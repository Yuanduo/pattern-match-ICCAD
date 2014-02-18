// oasis/trapezoid.cc -- support for TRAPEZOID and CTRAPEZOID records
//
// last modified:   19-Feb-2010  Fri  15:29
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cassert>
#include <climits>
#include <cstdlib>
#include "misc/arith.h"
#include "trapezoid.h"

namespace Oasis {

using namespace std;
using namespace SoftJin;


// We need some local data and functions for dealing with compressed
// trapezoids.  Put these in an unnamed namespace.

namespace {

// CTrapFunc -- specify a function of trapezoid width and height.
// In a compressed trapezoid, the X and Y coordinates of the vertices
// are computed as simple functions of the width and/or height.  That is
// why the trapezoids can be stored compactly in the file.  These
// enumerators specify all the functions that we need.

enum CTrapFunc {
    Z,          // value = 0
    W,          // value = width
    H,          // value = height
    W2,         // value = 2*width
    H2,         // value = 2*height
    mW,         // value = -width
    mH,         // value = -height
};



// CTrapInfo -- info for computing trapezoid values from width and/or height.
// OASIS defines 26 kinds of compressed trapezoids.  For each of these
// we need to compute the trapezoid orientation and the two deltas
// given the ctrapezoid type and its width and height.
//
// Instead of having a switch with 26 cases, we put all the data into a
// table with 26 rows.  The data in each row tell how to compute the
// value of the corresponding member of the RTrapezoid class.  In struct
// CTrapInfo all members are Uchar to save space; the following comment
// gives the actual type.
//
// For some ctrapezoids only one of width and height is given; the other
// dimension is computed from that.  In the table below, if the 'width'
// column is not W then the value is computed from the height.  Similarly
// if the height value is not H it is computed from the width.
//
// The row numbers in this table correspond to the ctrapezoid numbers in
// Figure 8 of the spec.  Types 16-23 are triangles.


struct CTrapInfo {
    Uchar   orient;     // Trapezoid::Orientation
    Uchar   width;      // CTrapFunc
    Uchar   height;     // CTrapFunc
    Uchar   delta_a;    // CTrapFunc
    Uchar   delta_b;    // CTrapFunc
};


const CTrapInfo  ctrapInfo[] =
{
//    orient                   wd   ht  d_a   d_b

    { Trapezoid::Horizontal,   W,   H,    Z,   mH  },  // 0
    { Trapezoid::Horizontal,   W,   H,    Z,    H  },  // 1
    { Trapezoid::Horizontal,   W,   H,    H,    Z  },  // 2
    { Trapezoid::Horizontal,   W,   H,   mH,    Z  },  // 3

    { Trapezoid::Horizontal,   W,   H,    H,   mH  },  // 4
    { Trapezoid::Horizontal,   W,   H,   mH,    H  },  // 5
    { Trapezoid::Horizontal,   W,   H,    H,    H  },  // 6
    { Trapezoid::Horizontal,   W,   H,   mH,   mH  },  // 7

    { Trapezoid::Vertical,     W,   H,    Z,    W  },  // 8
    { Trapezoid::Vertical,     W,   H,    Z,   mW  },  // 9
    { Trapezoid::Vertical,     W,   H,   mW,    Z  },  // 10
    { Trapezoid::Vertical,     W,   H,    W,    Z  },  // 11

    { Trapezoid::Vertical,     W,   H,   mW,    W  },  // 12
    { Trapezoid::Vertical,     W,   H,    W,   mW  },  // 13
    { Trapezoid::Vertical,     W,   H,   mW,   mW  },  // 14
    { Trapezoid::Vertical,     W,   H,    W,    W  },  // 15

    { Trapezoid::Horizontal,   W,   W,    Z,   mW  },  // 16
    { Trapezoid::Horizontal,   W,   W,    Z,    W  },  // 17
    { Trapezoid::Horizontal,   W,   W,    W,    Z  },  // 18
    { Trapezoid::Horizontal,   W,   W,   mW,    Z  },  // 19

    { Trapezoid::Horizontal,   H2,  H,    H,   mH  },  // 20
    { Trapezoid::Horizontal,   H2,  H,   mH,    H  },  // 21
    { Trapezoid::Vertical,     W,   W2,  mW,    W  },  // 22
    { Trapezoid::Vertical,     W,   W2,   W,   mW  },  // 23

    { Trapezoid::Horizontal,   W,   H,    Z,    Z  },  // 24
    { Trapezoid::Horizontal,   W,   W,    Z,    Z  },  // 25
};



// EvalCTrapFunc -- evaluate a CTrapFunc for the given arguments
//   func       the function specifier, a CTrapFunc.  Declared Uint
//              instead because everything is Uchar in the table above.
//
//   width      width of trapezoid.  May be undefined if the
//              function's value depends only on the height.
//
//   height     height of trapezoid.  May be undefined if the
//              function's value depends only on the width.
//
// Returns the value of the function func applied to width and height.

long
EvalCTrapFunc (/*CTrapFunc*/ Uint func, long width, long height)
{
    long  val;
    switch (func) {
        case Z:   val = 0;                              break;
        case W:   val = width;                          break;
        case H:   val = height;                         break;
        case W2:  val = CheckedPlus(width, width);      break;
        case H2:  val = CheckedPlus(height, height);    break;
        case mH:  val = -height;                        break;
        case mW:  val = -width;                         break;
        default:  val = 0;  /* make gcc happy */        break;
    }
    return val;
}


} // unnamed namespace



// Trapezoid constructor for uncompressed trapezoids.
// There is another constructor below for compressed trapezoids.
//
//   orient     trapezoid orientation.
//              Horizontal => two sides are parallel to the X axis
//              Vertical   => two sides are parallel to the Y axis
//              For a Manhattan rectangle, pick either one.
//
//   width      width  of trapezoid's bounding box.  Must be >= 0.
//   height     height of trapezoid's bounding box.  Must be >= 0.
//
//   delta_a    for horizontal trapezoids, NW.x - SW.x
//              i.e., the (signed) difference between the X coordinates
//              of the NW and SW vertices;
//              for vertical trapezoids, SW.y - SE.y
//
//   delta_b    for horizontal trapezoids, NE.x - SE.x
//              for vertical trapezoids, NW.y - NE.y
//
// Neither delta_a nor delta_b must be LONG_MIN.
// Throws BadTrapezoidError if the edges of the trapezoid cross.


Trapezoid::Trapezoid (Orientation orient,
                      long width, long height,
                      long delta_a, long delta_b)
    /* throw (BadTrapezoidError) */
{
    assert (width >= 0  &&  height >= 0);

    // We don't want to worry about overflows when negating.
    assert (delta_a != LONG_MIN  &&  delta_b != LONG_MIN);

    ctrapType = Uncompressed;
    this->orient  = orient;
    this->width   = width;
    this->height  = height;
    this->delta_a = delta_a;
    this->delta_b = delta_b;

    verifyValidity();
}



// Trapezoid constructor for compressed trapezoids
//   ctrapType  type of compressed trapezoid.  Must be in 0..25.
//   width      width of trapezoid.  Unused for ctrapType values
//              20 and 21, when it is computed from the height.
//   height     height of trapezoid.  Unused for ctrapType values
//              16-19, 22, 23, 25, when it is computed from the width.
//
// Throws BadTrapezoidError if the edges of the trapezoid cross.
// Throws overflow_error if an integer overflow occurs while computing
// a dimension or position.

Trapezoid::Trapezoid (Uint ctrapType, long width, long height)
    /* throw (BadTrapezoidError, overflow_error) */
{
    assert (width >= 0  &&  height >= 0);
    assert (ctrapezoidTypeIsValid(ctrapType));

    const CTrapInfo*  info = &ctrapInfo[ctrapType];
    orient = info->orient;
    this->ctrapType = ctrapType;
    this->width  = EvalCTrapFunc(info->width,  width, height);
    this->height = EvalCTrapFunc(info->height, width, height);
    this->delta_a = EvalCTrapFunc(info->delta_a, this->width, this->height);
    this->delta_b = EvalCTrapFunc(info->delta_b, this->width, this->height);

    verifyValidity();
}



// verifyValidity -- verify that the trapezoid is valid.
// This implements the checks of paragraphs 28.9 and 29.8 in the spec.

void
Trapezoid::verifyValidity()
{
    long  span = (orient == Horizontal) ? width : height;
    if (abs(CheckedMinus(delta_a, delta_b)) > span
            ||  abs(delta_a) > span
            ||  abs(delta_b) > span)
        throw BadTrapezoidError();
}



// tryCompress -- see if trapezoid can be expressed as an OASIS ctrapezoid.
// If it succeeds, it sets the member ctrapType to the ctrapezoid type,
// a number in the range 0..25.  Returns true if the ctrapezoid type is
// already known or if it manages to determine it.

bool
Trapezoid::tryCompress()
{
    // For each type of compressed trapezoid, see if all the values
    // computed by that row of the ctrapInfo table match the values
    // stored in the trapezoid.  If they do, that's the ctrapType of
    // this trapezoid.  Try the types at the bottom of the table first
    // because they have more compact representations than the ones at
    // the top.
    //
    // Types 16-19, 24, and 25 are both horizontal and vertical.  The
    // loop below recognizes the compression type only if the
    // orientation is horizontal, because that is what appears in
    // ctrapInfo[].  If the trapezoid's type is vertical instead, the
    // loop recognizes types 16-19 as types 8-11, which take a little
    // more space in the OASIS file, and does not recognize types 24 and
    // 25 at all.  Hence check for those types using special-case code.

    if (ctrapType != Uncompressed)      // already compressed
        return true;

    if (orient == Vertical) {
        if (width == height  &&  ((delta_a == 0 && abs(delta_b) == width)
                               || (delta_b == 0 && abs(delta_a) == width) ))
        {
            if (delta_a != 0)
                ctrapType = (delta_a > 0 ? 19 : 17);
            else if (delta_b != 0)
                ctrapType = (delta_b > 0 ? 16 : 18);
            else
                ctrapType = 25;
            return true;
        }
        if (delta_a == 0  &&  delta_b == 0) {
            ctrapType = 24;
            return true;
        }
    }

    int  row = sizeof(ctrapInfo)/sizeof(ctrapInfo[0]);
    while (--row >= 0) {
        const CTrapInfo*  info = &ctrapInfo[row];
        if (info->orient == orient
                &&  EvalCTrapFunc(info->delta_a, width, height) == delta_a
                &&  EvalCTrapFunc(info->delta_b, width, height) == delta_b
                &&  (info->width == W
                     ||  EvalCTrapFunc(info->width,  width, height) == width)
                &&  (info->height == H
                     ||  EvalCTrapFunc(info->height, width, height) == height))
        {
            ctrapType = row;
            return true;
        }
    }

    return false;
}



// getVertices -- return the four vertices of the trapezoid
//   pt         pointer to space for four vertices
//
// The vertices are returned in the (positively-oriented) order
// SW, SE, NE, NW.  Coordinates are assigned so that the
// bounding box is always (0, 0, width, height).
// Some of the vertices may coincide.
//
// If the trapezoid's orientation is Horizontal, points 0 and 1 will
// have the same Y coordinate value, as will points 2 and 3.  If the
// orientation is Vertical, points 0 and 3 will have the same X
// coordinate value, as will points 1 and 2.

void
Trapezoid::getVertices (/*out*/ Delta pt[]) const
{
    /*-----------------------------------------------------------------------
    For each of horizontal and vertical orientations we need to
    distinguish four cases.  First, the horizontal cases.  The Y coordinates
    are 0 for the bottom two vertices and height for the top two.  The
    X coordinates are shown below, where w, da, and db stand for width,
    delta_a, and delta_b, respectively.

        delta_a >= 0     delta_a >= 0       delta_a <  0     delta_a < 0
        delta_b >= 0     delta_b <  0       delta_b >= 0     delta_b < 0

          da      w        da   w+db       0            w    0       w+db
          +-------+        +------+        +------------+    +--------+
         /       /        /        \        \          /      \        \
        /       /        /          \        \        /        \        \
       +-------+        +------------+        +------+          +--------+
       0     w-db       0            w       -da    w-db       -da       w
    -------------------------------------------------------------------------*/

    if (orient == Horizontal) {
        pt[0].y = pt[1].y = 0;
        pt[2].y = pt[3].y = height;

        if (delta_a >= 0) {
            pt[0].x = 0;
            pt[3].x = delta_a;
        } else {
            pt[0].x = -delta_a;
            pt[3].x = 0;
        }
        if (delta_b >= 0) {
            pt[1].x = width - delta_b;
            pt[2].x = width;
        } else {
            pt[1].x = width;
            pt[2].x = width + delta_b;
        }
    }
    else {                              // orient == Vertical
        pt[0].x = pt[3].x = 0;
        pt[1].x = pt[2].x = width;

        if (delta_a >= 0) {
            pt[0].y = delta_a;
            pt[1].y = 0;
        } else {
            pt[0].y = 0;
            pt[1].y = -delta_a;
        }
        if (delta_b >= 0) {
            pt[2].y = height - delta_b;
            pt[3].y = height;
        } else {
            pt[2].y = height;
            pt[3].y = height + delta_b;
        }
    }
    /*----------------------------------------------------------------------
      For the vertically-oriented trapezoids, the X coordinate is 0 for
      the left two vertices and width for the right two vertices.  The
      Y coordinates are shown below for each case.

       delta_a >= 0    delta_a >= 0     delta_a <  0     delta_a < 0
       delta_b >= 0    delta_b <  0     delta_b >= 0     delta_b < 0

        h +                   + h         h +                   + h
          |\                 /|             |\                 /|
          | \               / |             | \               / |
          |  \        h+db +  |             |  + h-db        /  |
       da +   + h-db       |  |             |  |       h+db +   + -da
           \  |         da +  |             |  + -da        |  /
            \ |             \ |             | /             | /
             \|              \|             |/              |/
              + 0             + 0         0 +             0 +
    ------------------------------------------------------------------------*/
}



// For some types of compressed trapezoid, the OASIS file contains only
// one of width and height; the other size is derived from that one.
// For each ctrapezoid-type, OasisParser needs to know whether to read
// width or height or both.  needWidth() and needHeight() tell it.


/*static*/ bool
Trapezoid::needWidth (Uint ctrapType)
{
    assert (ctrapezoidTypeIsValid(ctrapType));
    return (ctrapInfo[ctrapType].width == W);
}


/*static*/ bool
Trapezoid::needHeight (Uint ctrapType)
{
    assert (ctrapezoidTypeIsValid(ctrapType));
    return (ctrapInfo[ctrapType].height == H);
}


}  // namespace Oasis
