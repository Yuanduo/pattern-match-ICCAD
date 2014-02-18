// oasis/oasis.cc -- types for OASIS constructs
//
// last modified:   30-Dec-2009  Wed  19:27
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <climits>
#include <cstdio>
#include "misc/arith.h"
#include "misc/utils.h"
#include "oasis.h"


namespace Oasis {

using namespace std;
using namespace SoftJin;


const char      OasisMagic[] = "%SEMI-OASIS\r\n";
    // 6.4: The magic string that begins each OASIS file.


// File-level standard properties.

const char  S_MAX_SIGNED_INTEGER_WIDTH[]   = "S_MAX_SIGNED_INTEGER_WIDTH";
const char  S_MAX_UNSIGNED_INTEGER_WIDTH[] = "S_MAX_UNSIGNED_INTEGER_WIDTH";
const char  S_MAX_STRING_LENGTH[]          = "S_MAX_STRING_LENGTH";
const char  S_POLYGON_MAX_VERTICES[]       = "S_POLYGON_MAX_VERTICES";
const char  S_PATH_MAX_VERTICES[]          = "S_PATH_MAX_VERTICES";
const char  S_TOP_CELL[]                   = "S_TOP_CELL";
const char  S_BOUNDING_BOXES_AVAILABLE[]   = "S_BOUNDING_BOXES_AVAILABLE";


// Cell-level standard properties.  Must appear after CELLNAME record.

const char  S_BOUNDING_BOX[] = "S_BOUNDING_BOX";
    // A2-2.2: A cell-level standard property.  All we do is verify that
    // each CellName has only one property with this name, as required
    // by 15.5.

const char  S_CELL_OFFSET[]  = "S_CELL_OFFSET";
    // A2-2.3: A cell-level standard property.  If its value is
    // available, OasisParser uses it to jump directly to the cell.
    // OasisCreator adds this property to each CELLNAME record it writes
    // if it wrote any CELL record with the same name.


// Element-level standard property.

const char  S_GDS_PROPERTY[] = "S_GDS_PROPERTY";


//======================================================================
//                              Oreal
//======================================================================

// assign -- assign an integer value to the Oreal
// Overloaded method.  The next one assigns an arbitrary rational.

Oreal&
Oreal::assign (long intval)
{
    // To maintain the class invariant, convert the number to
    // floating-point if it is LONG_MIN.

    if (intval == LONG_MIN)
        type = Float64;
    else {
        type = Rational;
        numer = intval;
        denom = 1;
    }
    value = static_cast<double>(intval);
    return *this;
}



// assign -- assign a rational value to the Oreal
// Precondition:  denom != 0

Oreal&
Oreal::assign (long numer, long denom)
{
    assert (denom != 0);

    // To maintain the class invariant, convert the number to
    // floating-point if either numerator or denominator is LONG_MIN.

    if (numer == LONG_MIN  ||  denom == LONG_MIN)
        type = Float64;
    else {
        type = Rational;
        // Ensure that the denominator is positive.  That is the other
        // class invariant.
        if (denom > 0) {
            this->numer = numer;
            this->denom = denom;
        } else {
            this->numer = -numer;
            this->denom = -denom;
        }
    }

    value = static_cast<double>(numer)/denom;
    return *this;
}



// isIdenticalTo -- see if this Oreal is identical to another.
// This compares both the representation and the value.  To compare the
// values alone, use operator==().  Note that assign() does not normalize
// the rational, so 1/2 is not identical to 2/4.

bool
Oreal::isIdenticalTo (const Oreal& x) const
{
    if (type != x.type)
        return false;

    if (type == Rational)
        return (numer == x.numer  &&  denom == x.denom);
    else
        return (value == x.value);
}



//======================================================================
//                              Delta
//======================================================================

// assign -- set this to an octangular displacement
// For the diagonal directions, the offset argument is the offset along
// the axis, as in the OASIS file.

Delta&
Delta::assign (Direction dirn, Ulong offset)
{
    switch (dirn) {
        case East:       x = offset;   y = 0;        break;
        case North:      x = 0;        y = offset;   break;
        case West:       x = -offset;  y = 0;        break;
        case South:      x = 0;        y = -offset;  break;

        case NorthEast:  x = offset;   y = offset;   break;
        case NorthWest:  x = -offset;  y = offset;   break;
        case SouthWest:  x = -offset;  y = -offset;  break;
        case SouthEast:  x = offset;   y = -offset;  break;
    }
    return *this;
}



// getDirection -- get the octangular direction of a 3-delta
// Precondition:
//   The delta must be representable as a 3-delta.  That is, either
//   one of the offsets is 0 or the magnitudes of the X and Y offsets
//   are the same.

Delta::Direction
Delta::getDirection() const
{
    assert (isOctangular());

    Direction  dirn;
    if (x == 0) {
        dirn = (y > 0 ? North : South);
    } else if (x > 0) {
        if (y == 0)
            dirn = East;
        else if (y > 0)
            dirn = NorthEast;
        else
            dirn = SouthEast;
    } else {
        if (y == 0)
            dirn = West;
        else if (y > 0)
            dirn = NorthWest;
        else
            dirn = SouthWest;
    }
    return dirn;
}



// operator+= -- add another delta to this, checking for overflow.
// Throws overflow_error if the addition would cause an overflow.

Delta&
Delta::operator+= (const Delta& delta) /* throw (overflow_error) */
{
    x = CheckedPlus(x, delta.x);
    y = CheckedPlus(y, delta.y);
    return *this;
}



Delta&
Delta::operator-= (const Delta& delta) /* throw (overflow_error) */
{
    x = CheckedMinus(x, delta.x);
    y = CheckedMinus(y, delta.y);
    return *this;
}



Delta&
Delta::operator*= (long mult) /* throw (overflow_error) */
{
    x = CheckedMult(x, mult);
    y = CheckedMult(y, mult);
    return *this;
}



Delta&
Delta::operator/= (long divisor)
{
    assert (divisor != 0);

    // Round the quotient in a roundabout way because Solaris 8 does not
    // have round().  The code ensures that division is symmetric about
    // the zero point.  That is,
    //     (-x)/divisor == - (x/divisor)
    //     (-y)/divisor == - (y/divisor)
    // This seems like a good idea.

    double  dx = static_cast<double>(x) / divisor;
    x = static_cast<long>(x >= 0 ? dx+0.5 : dx-0.5);

    double  dy = static_cast<double>(y) / divisor;
    y = static_cast<long>(y >= 0 ? dy+0.5 : dy-0.5);

    return *this;
}


//======================================================================
//                              Repetition
//======================================================================

void
Repetition::makeMatrix (Ulong xdimen, Ulong ydimen, long xspace, long yspace)
{
    // Ensure that the top-right corner of the matrix is representable.
    // [xy]space == 0 is allowed because the spec does not forbid zero
    // deltas.

    assert (xdimen >= 2  &&  ydimen >= 2);
    assert (xspace >= 0  &&  static_cast<Ulong>(xspace) <= LONG_MAX/(xdimen-1));
    assert (yspace >= 0  &&  static_cast<Ulong>(yspace) <= LONG_MAX/(ydimen-1));

    repType = Rep_Matrix;
    storType = StorMatrix;

    this->xdimen = xdimen;
    this->ydimen = ydimen;

    // Put the x-space in the first delta and the y-space in the second
    // delta.  This makes Rep_Matrix a special case of Rep_TiltedMatrix.

    deltas.clear();
    deltas.push_back(Delta(xspace, 0));
    deltas.push_back(Delta(0, yspace));
}



void
Repetition::makeUniformX (Ulong dimen, long xspace)
{
    assert (dimen >= 2);
    assert (xspace >= 0  &&  static_cast<Ulong>(xspace) <= LONG_MAX/(dimen-1));

    repType = Rep_UniformX;
    storType = StorUniformLine;

    this->dimen = dimen;
    deltas.clear();
    deltas.push_back(Delta(xspace, 0));
}



void
Repetition::makeUniformY (Ulong dimen, long yspace)
{
    assert (dimen >= 2);
    assert (yspace >= 0  &&  static_cast<Ulong>(yspace) <= LONG_MAX/(dimen-1));

    repType = Rep_UniformY;
    storType = StorUniformLine;

    this->dimen = dimen;
    deltas.clear();
    deltas.push_back(Delta(0, yspace));
}



// For all the StorArbitrary repetition types, the parameter 'dimen' is
// only an estimate of the number of points that will be added.  It is
// used to allocate space more efficiently. The actual number of points
// is the number of later calls to addVaryingXoffset(),
// addVaryingYoffset(), or addDelta().



void
Repetition::makeVaryingX (Ulong dimen)
{
    repType = Rep_VaryingX;
    storType = StorArbitrary;

    this->grid = 1;
    this->dimen = 0;
        // Note that this->dimen is set to 0, not dimen.
        // Class invariant for StorArbitrary:  dimen == deltas.size()
    deltas.reserve(dimen);
    deltas.clear();
}



void
Repetition::makeGridVaryingX (Ulong dimen, long grid)
{
    assert (grid >= 0);         // spec does not forbid zero grid

    repType = Rep_GridVaryingX;
    storType = StorArbitrary;

    this->grid = grid;
    this->dimen = 0;
    deltas.reserve(dimen);
    deltas.clear();
}



void
Repetition::makeVaryingY (Ulong dimen)
{
    repType = Rep_VaryingY;
    storType = StorArbitrary;

    this->grid = 1;
    this->dimen = 0;
    deltas.reserve(dimen);
    deltas.clear();
}



void
Repetition::makeGridVaryingY (Ulong dimen, long grid)
{
    assert (grid >= 0);

    repType = Rep_GridVaryingY;
    storType = StorArbitrary;

    this->grid = grid;
    this->dimen = 0;
    deltas.reserve(dimen);
    deltas.clear();
}



void
Repetition::makeTiltedMatrix (Ulong ndimen, Ulong mdimen,
                              const Delta& ndisp, const Delta& mdisp)
{
    assert (ndimen >= 2  &&  mdimen >= 2);

    // The X coordinate of the corner opposite the origin is
    //    ndisp.x * (ndimen-1) + mdisp.x * (mdimen-1)
    // We need to check that this fits in a long, but we have to do it
    // in two stages.  Similarly for the Y coordinate.

    assert (static_cast<Ulong>(abs(ndisp.x)) <= LONG_MAX/(ndimen-1));
    assert (static_cast<Ulong>(abs(mdisp.x))
                        <=  (LONG_MAX - (ndimen-1)*abs(ndisp.x)) / (mdimen-1));

    assert (static_cast<Ulong>(abs(ndisp.y)) <= LONG_MAX/(ndimen-1));
    assert (static_cast<Ulong>(abs(mdisp.y))
                        <=  (LONG_MAX - (ndimen-1)*abs(ndisp.y)) / (mdimen-1));

    repType = Rep_TiltedMatrix;
    storType = StorMatrix;

    this->ndimen = ndimen;
    this->mdimen = mdimen;

    deltas.clear();
    deltas.push_back(ndisp);
    deltas.push_back(mdisp);
}



void
Repetition::makeDiagonal (Ulong dimen, const Delta& disp)
{
    assert (dimen >= 2);

    repType = Rep_Diagonal;
    storType = StorUniformLine;

    this->dimen = dimen;
    deltas.clear();
    deltas.push_back(disp);
}



void
Repetition::makeArbitrary (Ulong dimen)
{
    repType = Rep_Arbitrary;
    storType = StorArbitrary;

    this->grid = 1;
    this->dimen = 0;
    deltas.clear();
    deltas.reserve(dimen);
}



void
Repetition::makeGridArbitrary (Ulong dimen, long grid)
{
    assert (grid >= 0);

    repType = Rep_GridArbitrary;
    storType = StorArbitrary;

    this->grid = grid;
    this->dimen = 0;
    deltas.clear();
    deltas.reserve(dimen);
}



bool
Repetition::operator== (const Repetition& rep) const
{
    if (repType != rep.repType)
        return false;

    switch (storType) {
        case StorNone:
            return true;

        case StorMatrix:
            // Note that xdimen is ndimen and ydimen is mdimen.
            return (xdimen == rep.xdimen  &&  ydimen == rep.ydimen
                    &&  deltas == rep.deltas);

        case StorUniformLine:
            return (dimen == rep.dimen  &&  deltas == rep.deltas);

        case StorArbitrary:
            return (grid == rep.grid  &&  deltas == rep.deltas);

        default:
            assert (false);
            return false;       // make gcc happy
    }
}



// begin() and end() support iteration through the points of a
// Repetition in the usual STL style when you are not interested in
// efficient special code for the special cases.
//
//     Repetition::iterator  iter = rep.begin();
//     Repetition::iterator  end  = rep.end();
//     for ( ;  iter != end;  ++iter) {
//         Delta pt = *iter;
//         ...
//     }
// Because end() is not inline, it is better to compute it before the
// loop than to compute it in the for-loop header.


Repetition::RepIter
Repetition::begin() const {
    return RepIter(*this);
}



Repetition::RepIter
Repetition::end() const
{
    RepIter  iter(*this);
    iter.i = xdimen;    // same as ndimen and dimen
    iter.j = 0;
    return iter;
}


//----------------------------------------------------------------------
//                              RepIter
//----------------------------------------------------------------------


Repetition::RepIter&
Repetition::RepIter::operator++()
{
    if (rep.storType == StorMatrix) {
        // This is for Rep_Matrix and Rep_TiltedMatrix.  Note that xdimen
        // is the same as ndimen and ydimen is the same as mdimen.  We
        // traverse the matrix in row-major order.
        //
        assert (i < rep.xdimen  &&  j < rep.ydimen);
        if (++j == rep.ydimen) {
            j = 0;
            ++i;
        }
    } else {
        // StorUniformLine and StorArbitrary
        assert (i < rep.dimen);
        ++i;
    }
    return *this;
}



Delta
Repetition::RepIter::operator*() const
{
    // We need not check for integer overflow in the StorMatrix and
    // StorUniformLine cases because Repetition's make*() functions have
    // verified that the coordinates of all points fit into a long.

    assert (rep.storType != StorNone);
    switch (rep.storType) {
        // See spec 7.6.10
        // deltas[0].x is nx-space, deltas[1].x is mx-space
        // deltas[0].y is ny-space, deltas[1].y is my-space
        //
        case StorMatrix:
            assert (i < rep.xdimen  &&  j < rep.ydimen);
            return Delta(i*rep.deltas[0].x + j*rep.deltas[1].x,
                         i*rep.deltas[0].y + j*rep.deltas[1].y);

        case StorUniformLine:
            assert (i < rep.dimen);
            return Delta(i*rep.deltas[0].x, i*rep.deltas[0].y);

        case StorArbitrary:
            assert (i < rep.dimen);
            return rep.deltas[i];

        default:
            assert (false);
            return Delta(0, 0);         // make gcc happy
    }
}



//======================================================================
//                              OFilePos
//======================================================================

// print -- print position in format suitable for error message
//   buf        where to print the position
//   bufsize    number of characters available in buf
// Returns the number of characters printed, excluding the terminating NUL.

Uint
OFilePos::print (/*out*/ char* buf, Uint bufsize) const
{
    Uint  n;
    llong  lloff = fileOffset;
    if (inCblock())
        n = SNprintf(buf, bufsize,
                     "uncompressed offset %lld in CBLOCK at offset %lld",
                     cblockOffset, lloff);
    else
        n = SNprintf(buf, bufsize, "offset %lld", lloff);

    // If buf is too small, SNprintf() returns the number of characters
    // that would have been written.  We want the actual number written.

    return (min(n, bufsize-1));
}



bool
operator== (const OFilePos& lhs, const OFilePos& rhs)
{
    return (lhs.fileOffset == rhs.fileOffset
            &&  lhs.cblockOffset == rhs.cblockOffset);
}



bool
operator< (const OFilePos& lhs, const OFilePos& rhs)
{
    if (lhs.fileOffset != rhs.fileOffset)
        return (lhs.fileOffset < rhs.fileOffset);
    return (lhs.cblockOffset < rhs.cblockOffset);
}


}  // namespace Oasis
