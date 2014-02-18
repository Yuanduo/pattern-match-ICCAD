// oasis/trapezoid.h -- support for TRAPEZOID and CTRAPEZOID records
//
// last modified:   01-Jan-2010  Fri  12:10
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef OASIS_TRAPEZOID_H_INCLUDED
#define OASIS_TRAPEZOID_H_INCLUDED

#include "port/compiler.h"
#include "oasis.h"


namespace Oasis {

using SoftJin::Uchar;
using SoftJin::Uint;


struct BadTrapezoidError { };
    // Exception thrown by Trapezoid constructor when its arguments are
    // invalid.


/*----------------------------------------------------------------------
    Trapezoid -- quadrilateral with two opposite sides parallel to an axis

    A Trapezoid is defined by its orientation, the width and height
    of its bounding box, and two parameters called delta_a and delta_b.
    We store the width and height in longs rather than Ulongs to avoid
    all the signed/unsigned hassles.

    The orientation is Horizontal if two sides are parallel to the X
    axis, and Vertical if two sides are parallel to the Y axis.  Width
    is measured along the X axis and height along the Y axis for both
    trapezoid orientations.

    Some kinds of trapezoids (e.g. rectangles) can be considered to
    have both orientations.  For such trapezoids orient can be either
    Horizontal or Vertical.  Applications must not depend on
    what the implementation currently sets it to.

    The parameters delta_a and delta_b are defined by the diagrams below.
    The names and definitions are taken from the OASIS spec (see
    paragraphs 28.6 and 28.7 and Figure 28-1 in the spec).

                                         + NW
            NW            NE             |\
             +------------+              | \
            /              \             |  \
           /                \            |   + NE   orient = Vertical
          /                  \           |   |      delta_a = SW.y - SE.y
         /                    \          |   |      delta_b = NW.y - NE.y
        +----------------------+         |   + SE
       SW                      SE        |  /
                                         | /
           orient = Horizontal           |/
           delta_a = NW.x - SW.x         + SW
           delta_b = NE.x - SE.x

    A side may have zero length, so a triangle with one side parallel to
    the Y axis may be considered to be a vertical trapezoid.  The
    trapezoid may be degenerate, i.e., reduce to a line segment or
    point.  (Spec 28.9: "The interpretation of zero-area trapezoids is
    application-dependent.")

    Trapezoid invariants:
        abs(delta_a) <= span
        abs(delta_b) <= span
        abs(delta_a - delta_b) <= span
    where
        span = (orient == Horizontal) ? width : height
    The last invariant says that the slant edges must not cross.
------------------------------------------------------------------------*/


class Trapezoid {
public:
    enum Orientation {
        Horizontal,     // sides are parallel to X axis
        Vertical        // sides are parallel to Y axis
    };

    enum { Uncompressed = -1 };
        // Used as the value of ctrapType for uncompressed trapezoids

private:
    Uchar       orient;         // Horizontal or Vertical
    short       ctrapType;      // either Uncompressed or 0..25
    long        width, height;
    long        delta_a, delta_b;

public:
                Trapezoid (Orientation orient, long width, long height,
                           long delta_a, long delta_b);
                    /* throw (BadTrapezoidError) */
                Trapezoid (Uint ctrapType, long width, long height);
                    /* throw (BadTrapezoidError, overflow_error) */
    bool        tryCompress();

    bool        isCompressed()    const { return (ctrapType >= 0); }
    int         getCompressType() const { return ctrapType;        }

    long        getWidth()   const { return width;   }
    long        getHeight()  const { return height;  }
    long        getDelta_A() const { return delta_a; }
    long        getDelta_B() const { return delta_b; }

    Orientation getOrientation()  const;
    void        getVertices (/*out*/ Delta pt[]) const;

    // Backward compatibility
    Delta       getVertex (Uint n) const SJ_DEPRECATED;

    static bool ctrapezoidTypeIsValid (Ulong ctrapType);
    static bool needWidth (Uint ctrapType);
    static bool needHeight (Uint ctrapType);

private:
    void        verifyValidity();
};



/*static*/ inline bool
Trapezoid::ctrapezoidTypeIsValid (Ulong ctrapType) {
    return (ctrapType <= 25);
}

inline Trapezoid::Orientation
Trapezoid::getOrientation() const {
    return static_cast<Orientation>(orient);
}



// getVertex -- return position of n'th vertex, 0 <= n < 4.
// Vertices are numbered starting with 0 for the bottom-left (southwest)
// vertex and increasing counter-clockwise.
//
// This function is provided only for backward compatibility.
// Applications should use getVertices() instead.

inline Delta
Trapezoid::getVertex (Uint n) const
{
    assert (n < 4);

    Delta  vertices[4];
    getVertices(vertices);
    return vertices[n];
}


}  // namespace Oasis

#endif  // OASIS_TRAPEZOID_H_INCLUDED
