// oasis/oasis.h -- types for OASIS constructs
//
// last modified:   30-Dec-2009  Wed  22:02
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// Classes defined here:
//
// Oreal        OASIS real number: integer, reciprocal, ratio, float, or double
// Delta        integer offsets or positions
// Repetition   OASIS repetition: a compact way to specify a set of Deltas
// PointList    OASIS point-list: a sequence of Deltas
// Validation   the validation scheme and signature in the END record
// OFilePos     specify position in an OASIS file, possibly inside a CBLOCK


#ifndef OASIS_OASIS_H_INCLUDED
#define OASIS_OASIS_H_INCLUDED

#include <algorithm>    // for swap()
#include <cassert>
#include <cstdlib>      // for overloaded abs()
#include <inttypes.h>
#include <limits>
#include <vector>

#include "misc/utils.h"


namespace Oasis {

using std::vector;
using SoftJin::llong;
using SoftJin::Ushort;
using SoftJin::Uint;
using SoftJin::Ulong;
using SoftJin::WarningHandler;


//======================================================================
//                              Constants
//======================================================================


extern const char  OasisMagic[];
const Uint         OasisMagicLength = 13;
    // 6.4: The magic string that begins each OASIS file
    // ("%SEMI-OASIS\r\n"), and its length (excluding the terminating NUL).


const off_t     StartRecordOffset = OasisMagicLength;
    // Immediately after the magic string is the START record.  We use
    // this name instead of OasisMagicLength as the argument to
    // OasisParser::seekTo() to make it clearer what we are doing.


const Uint      EndRecordSize = 256;
    // 14.2: END records are padded in the middle with a string of NULs
    // so that they are exactly this size (bytes).


const Ulong     DeflateCompression = 0;
    // 35.3: The only compression type for CBLOCK records supported by
    // version 1.0 of the OASIS spec.


// Names of standard properties.  Appendix 2 of the OASIS spec.

extern const char  S_MAX_SIGNED_INTEGER_WIDTH[];
extern const char  S_MAX_UNSIGNED_INTEGER_WIDTH[];
extern const char  S_MAX_STRING_LENGTH[];
extern const char  S_POLYGON_MAX_VERTICES[];
extern const char  S_PATH_MAX_VERTICES[];
extern const char  S_TOP_CELL[];
extern const char  S_BOUNDING_BOXES_AVAILABLE[];

extern const char  S_BOUNDING_BOX[];
extern const char  S_CELL_OFFSET[];

extern const char  S_GDS_PROPERTY[];


//======================================================================
//                              Oreal
//======================================================================

// Oreal -- OASIS real number: integer, rational, or floating-point.
// In OASIS, real numbers have several representations: as integers,
// reciprocals of integers, ratios, or floating-point numbers.  Oreal
// uses only two representations: rational and floating-point.  If the
// number is rational, Oreal keeps the numerator and denominator in
// addition to the approximate floating-point value.
//
// For floating-point numbers, Oreal remembers whether the number is a
// float or a double.  About the only use this has is for dumping an
// OASIS file in ASCII.
//
// For all rational types, OASIS stores the values of numerator and/or
// denominator as unsigned-integers, and uses the type of real to
// distinguish between positive and negative numbers.  We use (signed)
// long with the convention that the denominator is always positive.
// Because of this, about half the OASIS rationals (those with the high
// bit set in the unsigned-integer) are not representable as Oreal
// rationals.  We make this restriction to simplify the code.  The
// scanner converts unrepresentable rationals to type-7 (Float64) with a
// warning message.
//
// Invariants (assuming the instance is intialized):
//   type == Rational  =>  denom > 0
//      (i.e., the only numerator's sign matters)
//
//   type == Rational  =>  numer != LONG_MIN
//      (don't allow -2^31 or -2^63)


class Oreal {
public:
    // RealType -- representation of the real.
    // The values are specified in Table 7-3 of the spec.
    enum RealType {
        PosInteger     = 0,
        NegInteger     = 1,
        PosReciprocal  = 2,
        NegReciprocal  = 3,
        PosRatio       = 4,
        NegRatio       = 5,
        Float32        = 6,
        Float64        = 7,
    };

private:
    enum { Rational = 0 };      // should be different from Float32, Float64

    Uint        type;           // Rational, Float32, or Float64
    long        numer, denom;   // numerator, denominator if rational
    double      value;          // value of this real (always defined)

public:
                Oreal() { }

                Oreal (int val)  { assign(val); }
                Oreal (long val) { assign(val); }

                Oreal (long numer, long denom) {
                    assign(numer, denom);
                }
                Oreal (float val) {
                    type = Float32;
                    value = val;
                }
                Oreal (double val) {
                    type = Float64;
                    value = val;
                }

    // The compiler-generated copy constructor and copy assignment
    // operator are fine.

    Oreal&      operator= (int val)  { return assign(val); }
    Oreal&      operator= (long val) { return assign(val); }

    Oreal&      operator= (float val) {
                    type = Float32;
                    value = val;
                    return *this;
                }

    Oreal&      operator= (double val) {
                    type = Float64;
                    value = val;
                    return *this;
                }

    Oreal&      assign (long intval);
    Oreal&      assign (long numer, long denom);

    double      getValue()      const { return value; }
    bool        isRational()    const { return (type == Rational); }
    bool        isFloatSingle() const { return (type == Float32);  }
    bool        isFloatDouble() const { return (type == Float64);  }

    bool        isInteger() const {
                    return (type == Rational  &&  denom == 1 );
                }
    bool        isReciprocal() const {
                    return (type == Rational  &&  std::abs(numer) == 1);
                }
    long        getIntValue() const {
                    assert (isInteger());
                    return numer;
                }
    long        getNumerator() const {
                    assert (isRational());
                    return numer;
                }
    long        getDenominator() const {
                    assert (isRational());
                    return denom;
                }
    bool        isIdenticalTo (const Oreal& x) const;
};



// Equality checks for Oreals.  These compare only the values, not
// the representations.  Hence 1/2 == 0.5.  If you want to compare
// representations too, use Oreal::isIdenticalTo().


inline bool
operator== (const Oreal& x, const Oreal& y) {
    return (x.getValue() == y.getValue());
}


inline bool
operator!= (const Oreal& x, const Oreal& y) {
    return (! (x == y));
}


//======================================================================
//                              Delta
//======================================================================


// Delta -- signed offset or position

struct Delta {
    // Direction -- directions for 2-deltas, 3-deltas, and g-deltas.
    // 2-deltas use only the first four values (E|N|W|S), in bits 1..0.
    // 3-deltas store the direction in bits 2..0 and the single-integer
    // form of g-deltas store it in bits 3..1.
    // See paragraph 7.5.4 of the spec.

    enum Direction {
        East      = 0,
        North     = 1,
        West      = 2,
        South     = 3,
        NorthEast = 4,
        NorthWest = 5,
        SouthWest = 6,
        SouthEast = 7
    };

    long        x, y;

    //----------------------------------

                Delta() { }
                Delta (long x, long y) {
                    this->x = x;
                    this->y = y;
                }
                Delta (Direction dirn, Ulong offset) {
                    assign(dirn, offset);
                }

                // The compiler-generated copy constructor and
                // copy assignment operator are fine.

    Delta&      assign (Direction dirn, Ulong offset);
    Delta&      assign (long x, long y);

    Direction   getDirection() const;
    bool        isManhattan() const;
    bool        isOctangular() const;
    static bool isManhattan (Direction dirn);

    Delta&      operator+= (const Delta& delta); /* throw (overflow_error) */
    Delta&      operator-= (const Delta& delta); /* throw (overflow_error) */
    Delta&      operator*= (long mult);          /* throw (overflow_error) */
    Delta&      operator/= (long divisor);
};



inline bool
Delta::isManhattan() const {
    return (x == 0  ||  y == 0);
}


/*static*/ inline bool
Delta::isManhattan (Direction dirn) {
    return (dirn <= South);
}


inline bool
Delta::isOctangular() const {
    return (x == 0  ||  y == 0  ||  std::abs(x) == std::abs(y));
}



inline Delta&
Delta::assign (long x, long y)
{
    this->x = x;
    this->y = y;
    return *this;
}



inline Delta
operator+ (const Delta& da, const Delta& db) /* throw (overflow_error) */
{
    Delta  retval(da);
    retval += db;
    return retval;
}



inline Delta
operator- (const Delta& da, const Delta& db) /* throw (overflow_error) */
{
    Delta  retval(da);
    retval -= db;
    return retval;
}



inline Delta
operator* (const Delta& delta, long mult) /* throw (overflow_error) */
{
    Delta  retval(delta);
    retval *= mult;
    return retval;
}



inline Delta
operator/ (const Delta& delta, long divisor) /* throw (overflow_error) */
{
    Delta  retval(delta);
    retval /= divisor;
    return retval;
}



inline bool
operator== (const Delta& da, const Delta& db) {
    return (da.x == db.x  &&  da.y == db.y);
}


inline bool
operator!= (const Delta& da, const Delta& db) {
    return (! (da == db));
}


//======================================================================
//                              Repetition
//======================================================================


// RepetitionType -- how the repetition is specified
// The values are given by Table 7-6 of the spec.
// This enumeration is used both in the Repetition class below and in
// RawRepetition in records.h.

enum RepetitionType {
    Rep_ReusePrevious   = 0,
    Rep_Matrix          = 1,    // array aligned with the axes
    Rep_UniformX        = 2,    // uniformly spaced in a row
    Rep_UniformY        = 3,    // uniformly spaced in a column
    Rep_VaryingX        = 4,    // in a row with varying spaces between elems
    Rep_GridVaryingX    = 5,    // like VaryingX but elems are on grid points
    Rep_VaryingY        = 6,    // in a col with varying spaces between elems
    Rep_GridVaryingY    = 7,    // like VaryingY but elems are on grid points
    Rep_TiltedMatrix    = 8,    // like Matrix but may be tilted
    Rep_Diagonal        = 9,    // uniform spacing in line at any angle
    Rep_Arbitrary       = 10,   // elements can be anywhere
    Rep_GridArbitrary   = 11    // elements can be on any grid point
};



// Repetition -- specify set of positions compactly
// Spec section 7.6
//
// To simplify the code, we give up some of the compactness and merge
// the 12 types of repetition into 4 by treating some of the types
// as special cases of other types.  These 4 types are our storage
// types.
//
// StorNone
//      Used only for Rep_ReusePrevious
//
// StorMatrix
//      Used for Rep_Matrix and Rep_TiltedMatrix.  A Matrix is made a
//      special case of a TiltedMatrix by treating x-space as the
//      n-displacement (x-space, 0) and y-space as the m-displacement
//      (0, y-space).
//
//      deltas[0] is the n-displacement
//      deltas[1] is the m-displacement
//      n-dimen (x-dimen) is the number of points along the n-axis (x-axis)
//      m-dimen (y-dimen) is the number of points along the m-axis (y-axis)
//
// StorUniformLine
//      Used for Rep_UniformX, Rep_UniformY, and Rep_Diagonal.
//      Rep_UniformX is a special case of Rep_Diagonal for which
//      the displacement is (x-space, 0).  Similarly for Rep_UniformY
//      the displacement is (0, y-space).
//
//      deltas[0] is the displacement (between adjacent points)
//      dimen is the number of points on the line
//
// StorArbitrary
//      This is used for the remaining repetition types:
//          Rep_VaryingX,  Rep_GridVaryingX,
//          Rep_VaryingY,  Rep_GridVaryingY,
//          Rep_Arbitrary, Rep_GridArbitrary.
//      In each pair, the first is a special case of the second, with
//      grid = 1.  Rep_VaryingX is a special case of Rep_Arbitrary in
//      which each displacement has the form (xoffset, 0).  Similarly
//      in Rep_VaryingY each displacement has the form (0, yoffset).
//
//      deltas[0] is always (0,0)
//      deltas[j] is the position (offset) of the j'th point
//      Both deltas[j].x and deltas[j].y are multiples of grid.
//      dimen == deltas.size()  is the number of points
//
// Note that for the gridded repetition types the offsets must be
// multiplied by the grid before being added to the Repetition.
//
// All the space and offset arguments are declared to be longs although
// they are unsigned-integers in OASIS files.  We use signed values
// everywhere to avoid the dangers of mixing signed and unsigned
// arithmetic.  It is simpler to check for overflow when dealing only
// with signed numbers.
//
// XXX: For StorArbitrary it might be better to use deltas between
// adjacent points.
//
// WARNING:
// This class's interface will probably change as we get experience
// in using it.


class Repetition {
public:
    // StorageType -- how the repetition is stored.
    // The storage type tells which of the repetition fields are valid.

    enum StorageType {
        StorNone,               // Rep_ReusePrevious
        StorMatrix,             // Rep_Matrix, Rep_TiltedMatrix
        StorUniformLine,        // Rep_UniformX, Rep_UniformY, Rep_Diagonal
        StorArbitrary           // for the remaining Rep_* types
    };

private:
    class RepIter;

    Ushort      repType;        // RepetitionType
    Ushort      storType;       // StorageType

    union {
        Ulong   xdimen;         // defined for repType == Rep_Matrix
        Ulong   ndimen;         // defined for repType == Rep_TiltedMatrix
        Ulong   dimen;          // defined for storType == StorUniformLine
    };                          //         and storType == StorArbitrary
    union {
        Ulong   ydimen;         // defined for repType == Rep_Matrix
        Ulong   mdimen;         // defined for repType == Rep_TiltedMatrix
        long    grid;           // defined for storType == StorArbitrary
    };
    vector<Delta>  deltas;

public:
    Repetition()   { }          // uninitialized

    // The compiler-generated copy constructor, copy assignment operator,
    // and destructor are fine.

    //------------------------------------------------------------
    // Methods for constructing the repetition.  These are not actual
    // constructors because we want to keep reusing Repetition objects
    // in OasisParser.  The parameter 'dimen' for makeVaryingX(),
    // makeGridVaryingX(), makeVaryingY(), makeGridVaryingY(),
    // makeArbitrary(), and makeGridArbitrary() is advisory.  It is used
    // only to make storage allocation more efficient.

    void        makeReuse();
    void        makeMatrix (Ulong xdimen, Ulong ydimen,
                            long xspace, long yspace);
    void        makeUniformX (Ulong dimen, long xspace);
    void        makeUniformY (Ulong dimen, long yspace);

    void        makeVaryingX (Ulong dimen);
    void        makeGridVaryingX (Ulong dimen, long grid);
    void        addVaryingXoffset (long offset);

    void        makeVaryingY (Ulong dimen);
    void        makeGridVaryingY (Ulong dimen, long grid);
    void        addVaryingYoffset (long offset);

    void        makeTiltedMatrix (Ulong ndimen, Ulong mdimen,
                                  const Delta& ndisp, const Delta& mdisp);
    void        makeDiagonal (Ulong dimen, const Delta& disp);

    void        makeArbitrary (Ulong dimen);
    void        makeGridArbitrary (Ulong dimen, long grid);
    void        addDelta (const Delta& disp);

    //------------------------------------------------------------
    // Accessors

    RepetitionType  getType() const;

    Ulong       getDimen() const;
    long        getGrid() const;

    Ulong       getMatrixXdimen() const;
    Ulong       getMatrixYdimen() const;
    long        getMatrixXspace() const;
    long        getMatrixYspace() const;

    Ulong       getMatrixNdimen() const;
    Ulong       getMatrixMdimen() const;
    Delta       getMatrixNdelta() const;
    Delta       getMatrixMdelta() const;

    long        getUniformXspace() const;
    long        getUniformYspace() const;
    Delta       getDiagonalDelta() const;

    long        getVaryingXoffset (Ulong n) const;
    long        getVaryingYoffset (Ulong n) const;

    Delta       getDelta (Ulong n) const;

    //------------------------------------------------------------

    bool        operator== (const Repetition& rep) const;
    bool        operator!= (const Repetition& rep) const {
                    return (! (*this == rep));
                }

    //------------------------------------------------------------
    // Iteration

    // Logically a Repetition is a set of Deltas.  These members provide
    // a way to iterate over the set.  The iterator does not allow
    // modification of the Repetition; hence it is the same as
    // const_iterator.

    typedef Delta       value_type;
    typedef RepIter     iterator;
    typedef RepIter     const_iterator;

    iterator    begin() const;
    iterator    end() const;
};



inline void
Repetition::makeReuse() {
    repType = Rep_ReusePrevious;
    storType = StorNone;
}



// To construct a Rep_VaryingX, call makeVaryingX() to initialize the
// Repetition and then call addVaryingXoffset() to add each offset.
// The first offset added must be 0.  To construct a Rep_GridVaryingX,
// start with makeGridVaryingX() and add only offsets that are multiples
// of the grid.  Similarly for Rep_VaryingY and Rep_GridVaryingY.


inline void
Repetition::addVaryingXoffset (long offset)
{
    assert (repType == Rep_VaryingX  ||  repType == Rep_GridVaryingX);

    // Because x-spaces are unsigned-integers in the file, their sums
    // (the offsets) must be non-negative.

    assert (offset >= 0);

    // Because offsets must be multiplied by the grid before being added
    // here, the arg must be a multiple of the grid.
    // grid is 1 for Rep_VaryingX.

    assert (offset == (offset/grid) * grid);

    // Because x-spaces are unsigned, no offset can be to the
    // left of the previous one.

    assert (deltas.size() == 0  ||  offset >= deltas.back().x);

    deltas.push_back(Delta(offset, 0));
    ++dimen;
}



inline void
Repetition::addVaryingYoffset (long offset)
{
    assert (repType == Rep_VaryingY  ||  repType == Rep_GridVaryingY);
    assert (offset >= 0  &&  offset == (offset/grid) * grid);
    assert (deltas.size() == 0  ||  offset >= deltas.back().y);

    deltas.push_back(Delta(0, offset));
    ++dimen;
}



// To construct a Rep_Arbitrary initialize the Repetition with
// makeArbitrary() and then call addDelta() for each delta.  The first
// delta must be (0,0).


inline void
Repetition::addDelta (const Delta& disp)
{
    assert (repType == Rep_Arbitrary  ||  repType == Rep_GridArbitrary);
    deltas.push_back(disp);
    ++dimen;
}


//----------------------------------------------------------------------
// Accessors


inline RepetitionType
Repetition::getType() const {
    return (static_cast<RepetitionType>(repType));
}


inline Ulong
Repetition::getDimen() const {
    assert (storType == StorUniformLine  ||  storType == StorArbitrary);
    return dimen;
}


inline long
Repetition::getGrid() const {
    assert (storType == StorArbitrary);
    return grid;
}


inline Ulong
Repetition::getMatrixXdimen() const {
    assert (repType == Rep_Matrix);
    return xdimen;
}


inline Ulong
Repetition::getMatrixYdimen() const {
    assert (repType == Rep_Matrix);
    return ydimen;
}


inline long
Repetition::getMatrixXspace() const {
    assert (repType == Rep_Matrix);
    return deltas[0].x;
}


inline long
Repetition::getMatrixYspace() const {
    assert (repType == Rep_Matrix);
    return deltas[1].y;
}


inline Ulong
Repetition::getMatrixNdimen() const {
    assert (repType == Rep_TiltedMatrix);
    return ndimen;
}


inline Ulong
Repetition::getMatrixMdimen() const {
    assert (repType == Rep_TiltedMatrix);
    return mdimen;
}


inline Delta
Repetition::getMatrixNdelta() const {
    assert (repType == Rep_TiltedMatrix);
    return deltas[0];
}


inline Delta
Repetition::getMatrixMdelta() const {
    assert (repType == Rep_TiltedMatrix);
    return deltas[1];
}


inline long
Repetition::getUniformXspace() const {
    assert (storType == StorUniformLine);
    return deltas[0].x;
}


inline long
Repetition::getUniformYspace() const {
    assert (storType == StorUniformLine);
    return deltas[0].y;
}


inline Delta
Repetition::getDiagonalDelta() const {
    assert (repType == Rep_Diagonal);
    return deltas[0];
}


inline long
Repetition::getVaryingXoffset (Ulong n) const {
    assert (repType == Rep_VaryingX  ||  repType == Rep_GridVaryingX);
    return deltas[n].x;
}


inline long
Repetition::getVaryingYoffset (Ulong n) const {
    assert (repType == Rep_VaryingY  ||  repType == Rep_GridVaryingY);
    return deltas[n].y;
}


inline Delta
Repetition::getDelta (Ulong n) const {
    assert (repType == Rep_Arbitrary  ||  repType == Rep_GridArbitrary);
    return deltas[n];
}


//----------------------------------------------------------------------
//                              RepIter
//----------------------------------------------------------------------

// RepIter -- iterate through the points in a Repetition.
// Note that operator* returns a value, not a reference.  We cannot
// return a reference because the StorMatrix and StorUniformLine kinds
// of repetitions do not store the points explicitly.  That is why
// Repetition::iterator is the same as Repetition::const_iterator.
//
// The meaning of RepIter's members i and j depend on the Repetition's
// storage type.
//
// StorNone
//      Illegal
// StorMatrix
//      i is the index for the x|n dimension
//      j is the index for the y|m dimension
// StorUniformLine
//      i is the index for sole dimension and j is unused (always 0)
// StorArbitrary
//      i is an index into Repetition::deltas and j is unused (always 0)


class Repetition::RepIter {
    friend class Repetition;

    const Repetition&  rep;
    Ulong       i, j;

public:
    // The compiler-generated copy constructor and copy assignment operator
    // are fine.
    explicit    RepIter (const Repetition&);
    RepIter&    operator++();
    RepIter     operator++ (int);
    Delta       operator*() const;
    bool        operator== (const RepIter&) const;
    bool        operator!= (const RepIter&) const;
};



inline
Repetition::RepIter::RepIter (const Repetition& r) : rep(r) {
    assert (r.repType != Rep_ReusePrevious);
    i = j = 0;
}


inline Repetition::RepIter
Repetition::RepIter::operator++ (int)
{
    RepIter oldval(*this);
    ++*this;
    return oldval;
}


inline bool
Repetition::RepIter::operator== (const RepIter& iter) const {
    return (&rep == &iter.rep  &&  i == iter.i  &&  j == iter.j);
}


inline bool
Repetition::RepIter::operator!= (const RepIter& iter) const {
    return (! (*this == iter));
}



//======================================================================
//                              PointList
//======================================================================

// PointList -- list of points for polygon or path.
// This is a model of Reversible Container, not Random Access Container,
// though we store the points in a vector.  Random access does not seem
// to be needed, and it's better to retain the flexibility to replace
// the vector by a list.
//
// There are two differences between this class and the point-lists in
// an OASIS file.
//   - The file stores deltas between points (or double-deltas for type 5);
//     here we store all points as offsets from the initial point.
//   - In the file the initial point is implicit; here we store it
//     explicitly.  Thus points[0] is always (0,0).
//
// The contents of the member `points' does not depend on the list type.
// The type is stored only for the users of this class.
//
// XXX: Storing points as offsets from the first might not gain us much.
// Might be better to use deltas between adjacent points.
//
// The comments above apply to the 'normal' use of PointList, by
// OasisParser, OasisCreator, and application code.  The record-level
// classes use PointList differently.  See the comment before
// OasisRecordReader::readPointList() in rec-reader.cc


class PointList {
public:
    // ListType -- types of point list.
    // The enumerator values are given by Table 7-7 of the spec.

    enum ListType {
        ManhattanHorizFirst  = 0,
        ManhattanVertFirst   = 1,
        Manhattan            = 2,
        Octangular           = 3,
        AllAngle             = 4,
        AllAngleDoubleDelta  = 5,

        MaxListType          = 5
    };

private:
    ListType       type;
    vector<Delta>  points;

public:
                PointList() { }         // uninitialized
    explicit    PointList (ListType type) {
                    this->type = type;
                }

    // The compiler-generated copy constructor, copy assignment operator,
    // and destructor are fine.

    void        init (ListType type) {
                    this->type = type;
                    points.clear();
                }
    void        addPoint (const Delta& pt) {
                    points.push_back(pt);
                }

    size_t      numPoints() const { return points.size(); }
    ListType    getType()   const { return type;          }

    Delta       front() const {
                    assert (! points.empty());
                    return points.front();
                }
    Delta       back() const {
                    assert (! points.empty());
                    return points.back();
                }
    static bool typeIsValid (Ulong type) {
                    return (type <= MaxListType);
                }

    // Container requirements

    typedef vector<Delta>::value_type           value_type;
    typedef vector<Delta>::pointer              pointer;
    typedef vector<Delta>::const_pointer        const_pointer;
    typedef vector<Delta>::reference            reference;
    typedef vector<Delta>::const_reference      const_reference;
    typedef vector<Delta>::size_type            size_type;
    typedef vector<Delta>::difference_type      difference_type;
    typedef vector<Delta>::iterator             iterator;
    typedef vector<Delta>::const_iterator       const_iterator;

    iterator            begin()       { return points.begin(); }
    const_iterator      begin() const { return points.begin(); }
    iterator            end()         { return points.end();   }
    const_iterator      end()   const { return points.end();   }

    size_type   size()     const { return points.size();     }
    size_type   max_size() const { return points.max_size(); }
    bool        empty()    const { return points.empty();    }
    void        swap (PointList&);

    // Forward Container requirements

    bool        operator== (const PointList& ptlist) const;
    bool        operator!= (const PointList& ptlist) const;

    // Reversible Container requirements

    typedef vector<Delta>::reverse_iterator        reverse_iterator;
    typedef vector<Delta>::const_reverse_iterator  const_reverse_iterator;

    reverse_iterator            rbegin()       { return points.rbegin(); }
    const_reverse_iterator      rbegin() const { return points.rbegin(); }
    reverse_iterator            rend()         { return points.rend();   }
    const_reverse_iterator      rend()   const { return points.rend();   }
};



inline void
PointList::swap (PointList& ptlist) {
    std::swap(type, ptlist.type);
    points.swap(ptlist.points);
}


inline bool
PointList::operator== (const PointList& ptlist) const {
    return (type == ptlist.type  &&  points == ptlist.points);
}


inline bool
PointList::operator!= (const PointList& ptlist) const {
    return (! (*this == ptlist));
}



//======================================================================
//                              Validation
//======================================================================

// Validation -- validation data in END record

struct Validation {
    // Table 14-1:  END Record Validation Schemes
    enum Scheme {
        None       = 0,
        CRC32      = 1,
        Checksum32 = 2
    };

public:
    Scheme      scheme;
    uint32_t    signature;

public:
    Validation() { }

    Validation (Scheme scheme, uint32_t signature) {
        this->scheme = scheme;
        this->signature = signature;
    }

    const char*
    schemeName() const {
        const char *const  names[] = { "none", "crc32", "checksum32" };
        return names[scheme];
    }
};


//======================================================================
//                              OFilePos
//======================================================================

// OFilePos -- position in OASIS file
//
// Because of the CBLOCKs, a simple off_t does not completely specify
// a position in a file.  If an error occurs within a CBLOCK, we would
// like to say where in the CBLOCK it occurred.  Hence this struct.
//
// Note that OFilePos does not contain enough information to let the
// scanner jump to a position within a CBLOCK.  For that we would also
// need the other information in the CBLOCK record: the compression
// type, uncompressed size, and compressed size.
//
// fileOffset           off_t
//
//      The file offset of the position, or the file offset of the
//      CBLOCK's first byte if in a CBLOCK.  -1 means undefined.
//
// cblockOffset         llong
//
//      The offset within the current CBLOCK, or -1 if not in a CBLOCK.
//      Note that the position in the actual (compressed) CBLOCK is not
//      defined.  We only know how many uncompressed bytes we read
//      before we got to this position.
//
// print() prints the struct's contents to an output buffer in a form
// suitable for an error message.

struct OFilePos {
    off_t       fileOffset;
    llong       cblockOffset;           // -1 if not in CBLOCK

public:
                OFilePos() { }
                OFilePos (off_t fileOffset, llong cblockOffset = -1);

    void        init (off_t fileOffset, llong cblockOffset = -1);
    bool        inCblock() const;
    Uint        print (/*out*/ char* buf, Uint bufsize) const;
};



inline
OFilePos::OFilePos (off_t fileOffset, llong cblockOffset) {
    assert (cblockOffset >= -1);
    this->fileOffset = fileOffset;
    this->cblockOffset = cblockOffset;
}


inline void
OFilePos::init (off_t fileOffset, llong cblockOffset) {
    assert (cblockOffset >= -1);
    this->fileOffset = fileOffset;
    this->cblockOffset = cblockOffset;
}


inline bool
OFilePos::inCblock() const {
    return (cblockOffset >= 0);
}



// OasisDict::containsPos() needs to compare file positions.

bool    operator== (const OFilePos& lhs, const OFilePos& rhs);
bool    operator<  (const OFilePos& lhs, const OFilePos& rhs);


inline bool
operator!= (const OFilePos& lhs, const OFilePos& rhs) {
    return (! (lhs == rhs));
}

inline bool
operator> (const OFilePos& lhs, const OFilePos& rhs) {
    return (rhs < lhs);
}

inline bool
operator<= (const OFilePos& lhs, const OFilePos& rhs) {
    return (! (rhs < lhs));
}

inline bool
operator>= (const OFilePos& lhs, const OFilePos& rhs) {
    return (! (lhs < rhs));
}


}  // namespace Oasis

#endif  // OASIS_OASIS_H_INCLUDED
