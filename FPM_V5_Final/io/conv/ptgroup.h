// conv/ptgroup.h -- group sets of positions into OASIS repetitions
//
// last modified:   02-Mar-2006  Thu  11:31
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// When GdsToOasisConverter has to write out an element, it has a set of
// (x,y) positions at which the element appears.  The simple way to write
// the element is to create a Repetition object of type Rep_Arbitrary or
// Rep_GridArbitrary, add all the positions to the Repetition, and pass
// it to the OasisCreator method that writes the element.
//
// But if the set of positions falls into a regular pattern there might
// be a more compact representation.  For example, if there are 40
// points in an 8x5 array, we can use Rep_Matrix and specify all the
// positions using just 4 numbers.  What if there are 45 points of which
// 40 are in an 8x5 array and the remaining 5 are scattered around?
// Then we can write the element using two records, the first with a
// Rep_Matrix for the 8x5 array and the other with Rep_Arbitrary for the
// remaining 5 points.
//
// Writing two records like this will usually be more compact than
// writing a single record with Rep_Arbitrary for all 45 points because
// repeating an element record with identical field values is cheap in
// OASIS.  Typically the second record will have just the record-ID, the
// info-byte (with most bits set to 0 to indicate that the value should
// come from the modal variable), x, y, and the repetition specification.
// If the element has a single property, the second element record will
// have the single-byte PROPERTY_REPEAT record.
//
// The job of the class PointGrouper defined here is to take a set of
// positions and construct a set of Repetition objects from those
// positions in such a way that each position is specified by exactly
// one of the Repetition objects.  It should try to minimize the total
// space occupied by the resulting element record(s).
// XXX: It might be possible to relax the constraint "exactly one" to
// "at least one".  We'll play it safe for now.


#ifndef CONV_PTGROUP_H_INCLUDED
#define CONV_PTGROUP_H_INCLUDED

#include <vector>
#include "gdsii/builder.h"
#include "oasis/oasis.h"

namespace GdsiiOasis {

using std::vector;
using SoftJin::Uint;
using Gdsii::GdsPoint;
using Oasis::Delta;
using Oasis::Repetition;


// PointGrouper -- group sets of positions (of element) into OASIS Repetitions.
// It should be used like this:
//
//     PointGrouper  pg(element->positions, optLevel);
//     Delta  pos;
//     while (! pg.empty()) {
//         const Repetition*  rep = pg.makeRepetition(&pos);
//         creator.beginFooElement(..., pos.x, pos.y, ..., rep);
//     }
//
// Each call to makeRepetition() clubs together some subset of the
// positions into a Repetition.
//
// Implementation
//
// At high enough optimization levels PointGrouper tries to find
// regular patterns in the set of points by first moving them into a
// data structure somewhat like that for a sparse matrix.  Each element
// of the sparse matrix points to the next element on the same row and
// same column.
//
// matElems     vector<MatrixElement>
//
//      The sparse matrix.  Built only for high optimization levels.
//
// points       vector<GdsPoint>&
//
//      A reference to the vector passed to the constructor.  If we
//      build the sparse matrix we clear the vector and reuse it to hold
//      the miscellaneous points that are not part of an array.  After
//      extracting all the arrays from the sparse matrix we return all
//      the points in this in a Rep_Arbitrary, or perhaps Rep_VaryingX
//      or Rep_VaryingY if all the remaining points are in one line.
//
//      For low optimization levels we don't construct a sparse matrix
//      at all and leave all the points here.
//
// optLevel     Uint
//
//      Optimization level passed to the constructor.
//
// rep          Repetition
//
//      This is where we construct the value makeRepetition() returns.
//
// numPoints    Uint
//
//      The number of points remaining in the sparse matrix and 'points'
//      together.
//
// grid         int
//
//      The GCD of all the X and Y coordinates of points in the vector
//      'points'.  If this is >1, makeRepetition() creates a gridded
//      Repetition like Rep_GridArbitrary.  This member is set to 0
//      if optLevel is 0 and is unused if numPoints == 1.
//
// nextElem     vector<MatrixElement>::iterator
//
//      Iterator into matElems.  The next element to be processed by
//      makeRepetition().
//
// nextPoint    vector<GdsPoint>::iterator
//
//      Iterator into the vector 'points'.  The next point to be
//      processed by makeRepetition().  Because makeRepetition()
//      processes all the miscellaneous points after finishing the sparse
//      matrix, we have the invariant:
//          nextElem == matElems.end()  ||  nextPoint() == points.begin()


class PointGrouper {
    struct MatrixElement {
        int             x, y;
        bool            alloc;  // element used in Repetition
        MatrixElement*  right;  // next element on same row (y coordinate)
        MatrixElement*  up;     // next element on same column (x coordinate)

        explicit   MatrixElement (const GdsPoint& pt);
        void       markAllocated();
        bool       isAllocated() const;
    };

    typedef vector<MatrixElement>  SparseMatrix;

private:
    SparseMatrix      matElems; // points organized as sparse matrix
    vector<GdsPoint>& points;   // misc points for Rep_Arbitrary
    Uint        optLevel;       // optimization level
    Repetition  rep;            // makeRepetition() returns ptr to this
    Uint        numPoints;      // how many points remaining
    int         grid;           // GCD of coords in points vector

    SparseMatrix::iterator     nextElem;   // first element not examined
    vector<GdsPoint>::iterator nextPoint;  // first misc point not returned

public:
                PointGrouper (vector<GdsPoint>& points, Uint optLevel,
                              bool deleteDuplicates);
                ~PointGrouper();
    const Repetition*  makeRepetition (/*out*/ Delta* pos);

    bool        empty() const {
                    return (numPoints == 0);
                }
private:
    void        makeSparseMatrix();

    bool        tryHorizontalRepetition();
    bool        tryVerticalRepetition();

    bool        arrayBigEnough (Uint nelems);
    bool        lineLongEnough (Uint nelems);

    bool        tryArray (MatrixElement* start);
    Uint        tryHorizontalLine (const MatrixElement* start);
    Uint        tryVerticalLine (const MatrixElement* start);

    Uint        growArrayUp (const MatrixElement* start, Uint ncols);
    Uint        growArrayRight (const MatrixElement* start, Uint nrows);

    void        allocHorizontalLine (MatrixElement* start, Uint ncols);
    void        allocVerticalLine (MatrixElement* start, Uint nrows);
    void        allocArray (MatrixElement* sw, Uint ncols, Uint nrows);
private:
                PointGrouper (const PointGrouper&);     // forbidden
    void        operator= (const PointGrouper&);        // forbidden
};



bool    CoordInReach (long from, long to);


// PointInReach -- true if the delta between two points won't overflow

inline bool
PointInReach (const Delta& from, const Delta& to)
{
    // Because the coordinates come from GDSII 32-bit ints, there is
    // no chance of overflow on a 64-bit system.

    if (sizeof(long) > 4)
        return true;

    return (CoordInReach(from.x, to.x)  &&  CoordInReach(from.y, to.y));
}




}  // namespace GdsiiOasis

#endif  // CONV_PTGROUP_H_INCLUDED
