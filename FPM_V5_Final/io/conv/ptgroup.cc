// conv/ptgroup.cc -- group sets of positions into OASIS repetitions
//
// last modified:   01-Jan-2010  Fri  21:18
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <algorithm>            // for min(), sort(), unique()
#include <cassert>
#include <climits>
#include "port/hash-table.h"
#include "misc/arith.h"
#include "ptgroup.h"

namespace GdsiiOasis {

using namespace std;
using namespace SoftJin;


const Uint  MinArrayPoints = 8;
const Uint  MinLinePoints  = 6;
    // The minimum number of points in an array or line for the points
    // to be included in a Rep_Matrix or Rep_Uniform[XY].  The minimum
    // sensible values are 6 for array and 4 for lines.
    //
    // The minimum values really depend on the overhead of creating a
    // separate element, which in turn depends on the number and size of
    // the element's properties.  Adding zlib's compression, however,
    // makes things unpredictable.  So for simplicity we ignore all that
    // and use the fixed thresholds above.


namespace {

// PointLess -- comparison predicate for ordering points
// Sorts points first by y (row number) and then by x (column) within a
// row.  makeSparseMatrix() wants the points in this order.

struct PointLess {
    bool operator() (const GdsPoint& a, const GdsPoint& b) const {
        return (a.y < b.y  ||  (a.y == b.y  &&  a.x < b.x));
    }
};



// GCD -- return the GCD of two numbers x and y
// x and y may be positive, negative, or zero.
// The return value is 0 if both x and y are 0; otherwise it is positive.

int
GCD (int x, int y)
{
    // Make both numbers positive.  Treat -2^31 as a special case
    // because - (-2^31) = -2^31 with 32-bit ints.

    if (x == INT_MIN  &&  y == INT_MIN)
        return INT_MIN;
    if (x < 0)
        x = (x == INT_MIN) ? -(INT_MIN/2) : -x;
    if (y < 0)
        y = (y == INT_MIN) ? -(INT_MIN/2) : -y;

    // Euclid's algorithm

    while (y != 0) {
        int  rem = x % y;
        x = y;
        y = rem;
    }
    return x;
}


} // unnamed namespace



// CoordInReach -- true if the delta between two coordinates won't overflow.
//
// We use this to decide which points to include in a Repetition.  For
// example, if we have two points (-2^31, 0) and (1, 0), we cannot put
// both points into a Repetition because the delta cannot be represented
// by the Oasis layer (on a 32-bit machine).


bool
CoordInReach (long from, long to)
{
    // Because the coordinates come from GDSII 32-bit ints, there is
    // no chance of overflow on a 64-bit system.

    if (sizeof(long) > 4)
        return true;

    if ((from > 0  &&  to < 0  &&  from > LONG_MAX + to)
            ||  (from < 0  &&  to > 0  &&  from < -LONG_MAX + to))
        return false;

    return true;
}


//----------------------------------------------------------------------
//                              MatrixElement
//----------------------------------------------------------------------

// A MatrixElement is an element of the sparse-matrix-like data
// structure we use to recognize equal-spaced arrays and lines of
// points.  Each element holds one point's coordinates and has pointers
// to two other elements.  'right' points to the next element on the
// right, i.e., with the same Y coordinate, and is Null for the
// rightmost element in each row.  'up' points to the next higher
// element in the same column and is Null for the top element in each
// column.
//
// When an element has been included in a Repetition, we mark it
// allocated (by setting alloc to true) to keep from including it again
// in a different Repetition.  If an element is unallocated, all the
// elements in its column and above it are also unallocated.  That is,
//
// (!elem->isAllocated() && elem->up != Null)  =>  !elem->up->isAllocated()
//
// This is because makeRepetition() traverses the matrix from bottom to
// top and allocates elements only above and to the right of the current
// element.  We use this invariant to omit the test for allocated
// elements in a couple of functions.


inline
PointGrouper::MatrixElement::MatrixElement (const GdsPoint& pt) {
    x = pt.x;
    y = pt.y;
    alloc = false;
    up = right = Null;
}


inline bool
PointGrouper::MatrixElement::isAllocated() const {
    return alloc;
}


inline void
PointGrouper::MatrixElement::markAllocated() {
    assert (! isAllocated());
    alloc = true;
}


//----------------------------------------------------------------------


// PointGrouper constructor
//   points     vector of positions of element
//
//   optLevel   optimization level.  How much effort we should expend to
//              make the OASIS file small.
//
//   deleteDuplicates
//              true => delete duplicate points; false => keep duplicates.
//              Currently PointGrouper does not bother to make arrays out
//              of duplicate points, even at optLevel=2.
//
// The points vector should not be used after being passed to this.
// It may be modified both here and in makeRepetition().
//
// Precondition:  ! points.empty()
//
// The current meaning of optLevel:
//
// 0    No optimization.  Dump all points using Rep_Arbitrary.
// 1    Sort points and compute grid.  Use Rep_GridArbitrary where possible.
// >1   Build sparse matrix, look for patterns in that, and try to use
//      Rep_Matrix, Rep_UniformX, Rep_UniformY.  For the miscellaneous
//      points, try to use RepVaryingX, RepVaryingY and the gridded variants.

PointGrouper::PointGrouper (vector<GdsPoint>& points, Uint optLevel,
                            bool deleteDuplicates)
  : points(points)
{
    // The vector of positions must be non-empty because every element
    // has at least one position.

    assert (! points.empty());

    this->optLevel = optLevel;
    numPoints = points.size();
    nextElem = matElems.end();  // the sparse matrix is initially empty
    nextPoint = points.begin(); // everything is still in the points vector

    // There is no repetition when there is only one point.
    if (numPoints == 1)
        return;

    // Sort the points first by row (Y coordinate) and then by the X
    // coordinate within a row.  This is mainly for makeSparseMatrix(),
    // which wants the points sorted that way.  But sorting is useful
    // even if we don't build the sparse matrix.  When the points are
    // sorted, regular patterns in the points appear in the OASIS file
    // as sequences of identical deltas, which zlib compresses well.  It
    // compresses them so well that when it is enabled there might not
    // be any reason to recognize arrays and lines.  The resulting file
    // tends have roughly the same size as when we just dump all points
    // into a Rep_GridArbitrary.
    //
    // If requested, delete duplicate points, which will be contiguous
    // after the sort.

    if (optLevel > 0  ||  deleteDuplicates) {
        sort(points.begin(), points.end(), PointLess());
        if (deleteDuplicates) {
            points.erase(unique(points.begin(),points.end()), points.end());
            numPoints = points.size();
        }
    }

    // For high optimization levels, move all points into the sparse
    // matrix.  We use that to search for regular patterns.  But do this
    // only if there are enough points to form a uniform line or array.
    // Otherwise we shall try directly for one of the varying-delta
    // repetitions.

    if (optLevel > 1  &&  numPoints >= min(MinArrayPoints, MinLinePoints))
        makeSparseMatrix();

    // Compute the GCD of all the coordinates of points left in the
    // vector 'points'.  We use that as the grid value for
    // Rep_GridArbitrary.  If makeSparseMatrix() was invoked above then
    // the vector will be empty and grid will remain 0.

    grid = 0;
    if (optLevel > 0) {
        vector<GdsPoint>::const_iterator  iter = points.begin(),
                                          end  = points.end();
        for ( ;  iter != end;  ++iter)
            grid = GCD(grid, GCD(iter->x, iter->y));
    }
}



PointGrouper::~PointGrouper() { }



// makeSparseMatrix -- build sparse matrix from points, to look for patterns
// Precondition:  The vector 'points' should be sorted.

void
PointGrouper::makeSparseMatrix()
{
    // Make a MatrixElement out of each unique point in the input
    // vector.  Duplicate points would complicate the matrix creation
    // code below, so store them temporarily in a separate vector
    // instead of making MatrixElements out of them.  Although it might
    // be possible to group some duplicates into arrays, doing that is
    // too much trouble.

    vector<GdsPoint>  duplicates;
    matElems.reserve(points.size());

    vector<GdsPoint>::const_iterator  ptIter = points.begin();
    vector<GdsPoint>::const_iterator  ptEnd  = points.end();
    GdsPoint  pt = *ptIter;
    matElems.push_back(MatrixElement(pt));
    while (++ptIter != ptEnd) {
        if (*ptIter == pt)
            duplicates.push_back(*ptIter);
        else {
            pt = *ptIter;
            matElems.push_back(MatrixElement(pt));
        }
    }

    // Now that we have converted all unique points into MatrixElements,
    // reuse 'points' to hold all the points left over after extracting
    // the arrays and uniform lines.  These include the duplicates, if any.

    points = duplicates;

    // Build the sparse matrix.  Because the vector is sorted, each
    // element is either to right of the preceding element and on the
    // same row, or somewhere above the preceding element.  So we can
    // set the 'up' and 'right' links by scanning the elements from
    // beginning to end.
    //
    // To make the 'up' links we need the last element seen for a given
    // column (X coordinate).  We use a HashMap for that.  Its key is
    // an X coordinate and its value points to the topmost MatrixElement
    // seen for that X value.

    HashMap<int, MatrixElement*>  colMap(numPoints);

    MatrixElement*  elem = &*matElems.begin();
    MatrixElement*  end  = &*matElems.end();
    MatrixElement*  next = elem + 1;
    for ( ;  elem != end;  ++elem, ++next) {
        if (next != end  &&  next->y == elem->y)
            elem->right = next;
        MatrixElement**  below = &colMap[elem->x];
        if (*below != Null)             // earlier element in this column?
            (*below)->up = elem;        // yes, so this elem is above that one
        *below = elem;                  // this is now topmost in column
    }

    // The first call to makeRepetition() should start searching for
    // patterns from the first element, and after the elements are done
    // it should start from the first point.

    nextElem = matElems.begin();
    nextPoint = points.begin();
}



// makeRepetition -- select subset of the points to make repetition.
//   pos        out: the position of the repetition's origin (i.e., the
//              element's position) is stored here.
//
// Returns a pointer to a Repetition to use, or Null if only one
// position is left.  The Repetition object pointed to is owned by this
// class and will be modified on the next call.
//
// Precondition:  ! empty()

const Repetition*
PointGrouper::makeRepetition (/*out*/ Delta* pos)
{
    assert (numPoints > 0);

    // We have at least one point left, but that point might be in the
    // sparse matrix or in the vector of miscellaneous points.  First
    // check the sparse matrix if we haven't finished with it.
    //
    // The strategy is to scan the sparse matrix row by row starting
    // from the bottom-left end.  For each point we look for an array
    // whose corner is that point or a uniformly-spaced horizontal or
    // vertical line beginning at that point.  If we find one, we mark
    // as being allocated all the points in the array/line, and return a
    // repetition for the points marked.  If the point is not part of an
    // array/line, we move it back into the vector 'points' to be
    // included in a varying-delta repetition at the end.

    if (nextElem != matElems.end()) {
        SparseMatrix::iterator  elem = nextElem,
                                end  = matElems.end();
        for ( ;  elem != end;  ++elem) {
            if (elem->isAllocated())
                continue;
            if (tryArray(&*elem)) {     // sets rep and numPoints on success
                pos->x = elem->x;
                pos->y = elem->y;
                nextElem = elem + 1;
                return &rep;
            }

            // The current point is not part of an array, so add it
            // to the list of miscellaneous points.  Adding the point
            // will not invalidate the iterator nextPoint because we
            // never add more points than were originally in the vector
            // 'points' when it was passed to the constructor.  We need
            // not mark the element because we shall never see it again.

            grid = GCD(grid, GCD(elem->x, elem->y));
            points.push_back(GdsPoint(elem->x, elem->y));
        }

        // We are done with the sparse matrix.
        nextElem = end;
    }

    // Nothing left in the sparse matrix, so start on contents of the
    // 'points' vector.  Whatever happens next, the position returned is
    // that of the first point left.

    pos->x = nextPoint->x;
    pos->y = nextPoint->y;

    // There is no repetition if only one point is left.

    if (numPoints == 1) {
        numPoints = 0;          // so no need to set nextPoint
        return Null;
    }

    // There are at least two points.  If _all_ are in a horizontal or
    // vertical line, use Rep_Varying[XY] or the gridded variant.
    // Grouping the points into more than one Rep_Varying[XY] is not
    // worth the effort.  The chances are that when compression is
    // enabled the resuting file will be about the same size as when
    // grouping all the points into a single Rep_Arbitrary.
    //
    // tryHorizontalRepetition() and tryVerticalRepetition set rep,
    // nextPoint, and numPoints.

    if (optLevel > 1) {
        if (tryHorizontalRepetition() || tryVerticalRepetition())
            return &rep;
    }

    // Otherwise make a Rep_GridArbitrary or Rep_Arbitrary repetition
    // out of as many of them as possible.
    //
    // Note that the grid we have computed is the GCD of the absolute
    // coordinates whereas the grid in the OASIS repetition is for the
    // deltas.  We might therefore not get the best value.  For example,
    // if the points are at (5,0), (15,0), and (25,0), we compute the
    // grid value 5.  We could use grid 10 instead because both deltas
    // are (10,0).
    //
    // XXX: Consider changing the Repetition interface to allow the grid
    // to be set after adding the points.  That would let us use the
    // best possible grid.
    //
    // Also note that the numPoints argument to makeGridArbitrary() and
    // makeArbitrary() is only advisory.  We may add fewer points.

    if (grid > 1)
        rep.makeGridArbitrary(numPoints, grid);
    else
        rep.makeArbitrary(numPoints);

    // Repetition requires not the actual coordinates of each point but
    // its offset with respect to the first point.  Add these offsets,
    // but stop at the first point where computing the delta would cause
    // integer overflow.  We must worry about two deltas for each point:
    //   - between each point and the first point, which is what
    //     Repetition stores; and
    //   - between each point and the previous one, which is what
    //     appears in the OASIS file.
    //
    // XXX: Consider changing Repetition to store deltas between
    // adjacent points, as in the file.

    Delta  start(nextPoint->x, nextPoint->y);
    Delta  prev(start);
    vector<GdsPoint>::iterator  iter = nextPoint,
                                end  = points.end();
    for ( ;  iter != end;  ++iter) {
        Delta  curr(iter->x, iter->y);
        if (!PointInReach(start, curr)  ||  !PointInReach(prev, curr))
            break;
        rep.addDelta(curr - start);
        prev = curr;
    }
    nextPoint = iter;
    numPoints = end - iter;
    return &rep;
}



// tryHorizontalRepetition -- make rep if _all_ points are in horizontal line.
// If all the remaining points in the vector 'points' are on the same
// row, tryHorizontalRepetition() puts as many points as possible into
// rep, sets numPoints and nextPoint, and returns true.  Otherwise it
// returns false.

bool
PointGrouper::tryHorizontalRepetition()
{
    // Verify that all Y coordinates are the same and X coordinates are
    // non-decreasing.  Duplicate points make the X coordinate test
    // necessary.  If all points are distinct, the sort in the
    // constructor is enough to ensure left-to-right order.

    vector<GdsPoint>::iterator  iter = nextPoint;
    vector<GdsPoint>::iterator  end  = points.end();
    GdsPoint  ptprev = *iter;
    while (++iter != end) {
        if (iter->y != ptprev.y  ||  iter->x < ptprev.x)
            return false;
        ptprev = *iter;
    }

    // Initialize the repetition and add as many points as possible.
    // This is more or less a copy of the code at the bottom of
    // makeRepetition(), but for the X coordinate only.

    // If there are two points, only one coordinate is written.
    // So using a grid won't save anything.

    if (grid > 1  &&  numPoints > 2)
        rep.makeGridVaryingX(numPoints, grid);
    else
        rep.makeVaryingX(numPoints);

    long  start = nextPoint->x;
    long  prev = start;
    for (iter = nextPoint, end = points.end();  iter != end;  ++iter) {
        long  curr = iter->x;
        if (!CoordInReach(start, curr)  ||  !CoordInReach(prev, curr))
            break;
        rep.addVaryingXoffset(curr - start);
        prev = curr;
    }
    nextPoint = iter;
    numPoints = end - iter;
    return true;
}



// tryVerticalRepetition -- make rep if _all_ points are in vertical line.
// This is the vertical analogue of tryHorizontalRepetition().

bool
PointGrouper::tryVerticalRepetition()
{
    vector<GdsPoint>::iterator  iter = nextPoint;
    vector<GdsPoint>::iterator  end  = points.end();
    GdsPoint  ptprev = *iter;
    while (++iter != end) {
        if (iter->x != ptprev.x  ||  iter->y < ptprev.y)
            return false;
        ptprev = *iter;
    }

    if (grid > 1  &&  numPoints > 2)
        rep.makeGridVaryingY(numPoints, grid);
    else
        rep.makeVaryingY(numPoints);

    long  start = nextPoint->y;
    long  prev = start;
    for (iter = nextPoint, end = points.end();  iter != end;  ++iter) {
        long  curr = iter->y;
        if (!CoordInReach(start, curr)  ||  !CoordInReach(prev, curr))
            break;
        rep.addVaryingYoffset(curr - start);
        prev = curr;
    }
    nextPoint = iter;
    numPoints = end - iter;
    return true;
}



// arrayBigEnough -- is array big enough to justify creating separate rep?
// lineLongEnough -- is line long enough to justify creating separate rep?
//   nelems     number of elements in array or line
//
// Helper functions for tryArray() below.  Both return true when the
// array or line uses up all remaining points, even if the count would
// be below the threshold.


inline bool
PointGrouper::arrayBigEnough (Uint nelems) {
    assert (nelems > 1);
    return (nelems >= MinArrayPoints  ||  nelems == numPoints);
}


inline bool
PointGrouper::lineLongEnough (Uint nelems) {
    assert (nelems > 1);
    return (nelems >= MinLinePoints  ||  nelems == numPoints);
}



// tryArray -- try to construct line/matrix Repetition anchored at given elem.
//   start      the starting element
//
// tryArray() looks for one of these patterns:
//   - a big-enough array of equal-spaced elements whose
//     bottom-left (southwest) corner is start; or
//   - a long-enough horizontal line of equal-spaced elements
//     whose leftmost one is start; or
//   - a long-enough vertical line of equal-spaced elements
//     whose bottom one is start.
//
// If there is, tryArray() initializes the class member rep with the
// appropriate Repetition parameters, marks all the elements included in
// rep, updates numPoints, and returns true.  Otherwise it returns false.
//
// The two methods above determine what is big and long enough.

bool
PointGrouper::tryArray (MatrixElement* start)
{
    Uint        ncols, nrows;

    // To keep things simple, we look only for arrays and lines aligned
    // with the axes.  We do not look for groups of elements that can be
    // described by Rep_Diagonal or Rep_TiltedMatrix.  We also examine
    // only the spacing between adjacent points, so we might not notice
    // an array if some other point comes in the middle.  For instance,
    // in both these sets of points the extra point prevents us from
    // recognizing the 2x3 array.
    //
    //     X       X       X                  X       X    X  X
    //
    //     X   X   X       X                  X       X       X


    // See if there are at least 3 equal-spaced points starting at start
    // in this row.  If there are, try to grow the line upwards into an
    // array.  That is, see if there is an array whose lowest line is
    // the one we first found.  If an array or line is found, mark the
    // elements in it as being allocated.

    ncols = tryHorizontalLine(start);
    if (ncols >= 3) {
        nrows = growArrayUp(start, ncols);
        if (nrows > 1  &&  arrayBigEnough(ncols*nrows)) {
            rep.makeMatrix(ncols, nrows,
                           start->right->x - start->x,
                           start->up->y - start->y);
            numPoints -= ncols * nrows;
            allocArray(start, ncols, nrows);
            return true;
        }

        // Either there is no array or it's not big enough.  If the
        // original line is long enough, make a repetition out of that.

        if (lineLongEnough(ncols)) {
            rep.makeUniformX(ncols, start->right->x - start->x);
            numPoints -= ncols;
            allocHorizontalLine(start, ncols);
            return true;
        }
    }

    // Now look for a vertical line of at least 3 points and try to grow
    // it rightwards.

    nrows = tryVerticalLine(start);
    if (nrows >= 3) {
        ncols = growArrayRight(start, nrows);
        if (ncols > 1  &&  arrayBigEnough(ncols*nrows)) {
            rep.makeMatrix(ncols, nrows,
                           start->right->x - start->x,
                           start->up->y - start->y);
            numPoints -= ncols * nrows;
            allocArray(start, ncols, nrows);
            return true;
        }
        if (lineLongEnough(nrows)) {
            rep.makeUniformY(nrows, start->up->y - start->y);
            numPoints -= nrows;
            allocVerticalLine(start, nrows);
            return true;
        }
    }

    return false;
}



// tryHorizontalLine -- look for horiz line of equal-spaced MatrixElements.
//   start      the start element, i.e., the leftmost element of the
//              potential line
// Returns the number of equal-spaced elements.  This number includes
// start, and so will be at least 1.

Uint
PointGrouper::tryHorizontalLine (const MatrixElement* start)
{
    // First verify that there is an unallocated element to the right of
    // start.

    if (start->right == Null  ||  start->right->isAllocated())
        return 1;

    // Get the distance between the two elements and verify that this
    // fits into an int.  If the distance is too large it will overflow,
    // and become negative on any reasonable processor.

    int  space = start->right->x - start->x;
    if (space < 0)
        return 1;

    // Now see how many more unallocated points to the right have the
    // same spacing.

    Uint  ncols = 2;
    const MatrixElement*  elem = start->right;
    while (elem->right != Null
            &&  ! elem->right->isAllocated()
            &&  elem->right->x - elem->x == space) {
        elem = elem->right;
        ++ncols;
    }
    return ncols;
}



// tryVerticalLine -- look for vertical line of equal-spaced MatrixElements.
//   start      the start element, i.e., the bottom element of the
//              potential line
// Returns the number of equal-spaced elements.  This number includes
// start, and so will be at least 1.
// This is the vertical analogue of tryHorizontalLine().

Uint
PointGrouper::tryVerticalLine (const MatrixElement* start)
{
    // Because of the MatrixElement invariant we need not check that
    // start->up is unallocated.  Similarly the while loop condition
    // later is slightly different from the corresponding condition in
    // tryHorizontalLine().

    if (start->up == Null)
        return 1;
    assert (! start->up->isAllocated());

    int  space = start->up->y - start->y;
    if (space < 0)
        return 1;

    Uint  nrows = 2;
    const MatrixElement*  elem = start->up;
    while (elem->up != Null  &&  elem->up->y - elem->y == space) {
        assert (! elem->up->isAllocated());
        elem = elem->up;
        ++nrows;
    }
    return nrows;
}



// growArrayUp -- make array by growing horiz line upwards as much as possible.
//   start      MatrixElement at left end of line.  If there is an array
//              this will be the southwest corner.
//   ncols      number of MatrixElements in the line
//
// Returns the number of rows in the array, 1 if no growth is possible.

Uint
PointGrouper::growArrayUp (const MatrixElement* start, Uint ncols)
{
    Uint  nrows = 1;      // the input line is one row

    // If we want to grow upwards, there must be something above.
    if (start->up == Null)
        return nrows;

    // The distance between the leftmost element of the line and the one
    // above it is the vertical spacing for the whole array.  If this
    // distance is too large and overflows, we cannot grow upwards.

    int  vspace = start->up->y - start->y;
    if (vspace < 0)
        return nrows;

    // The outer loop grows the array upward as much as possible using
    // vspace as the spacing between rows.  Each iteration grows the
    // array upward by one line.
    //
    // The inner loop verifies for each element in the current top line
    // that the element immediately above it is at the right spacing.
    // The element above is guaranteed to be unallocated if it exists.
    // Note that an extra element in one of the rows or columns will
    // stop the array's growth.

    for (;;) {
        const MatrixElement*  above = start->up;
        if (above == Null  ||  above->y - start->y != vspace)
            break;

        // 'elem' moves from left to right in the current row.  'above'
        // moves from left to right in the row above.  Verify at each
        // step that 'above' really is above 'elem'.

        const MatrixElement*  elem = start;
        for (Uint j = 0;  j < ncols;  ++j) {
            if (elem->up == Null  ||  elem->up != above)
                goto END_OUTER_LOOP;
            assert (! above->isAllocated());
            elem = elem->right;
            above = above->right;
        }

        // Grow array by one line
        start = start->up;
        ++nrows;
    }
  END_OUTER_LOOP: ;

    return nrows;
}



// growArrayRight -- make array by growing vert line right as much as possible.
//   start      MatrixElement at bottom end of line.  If there is an array
//              this will be the southwest corner.
//   nrows      number of MatrixElements in the line
//
// Returns the number of columns in the array, 1 if no growth is possible.
// This is the horizontal analogue of growArrayUp().

Uint
PointGrouper::growArrayRight (const MatrixElement* start, Uint nrows)
{
    Uint  ncols = 1;

    if (start->right == Null)
        return ncols;

    int  hspace = start->right->x - start->x;
    if (hspace < 0)
        return ncols;

    // We must check here that the elements to the right are unallocated
    // even though we need not do that in growArrayUp().  But it
    // suffices to check for only the bottom row because the elements
    // above an unallocated element will also be unallocated.

    for (;;) {
        const MatrixElement*  beside = start->right;
        if (beside == Null
                || beside->isAllocated()
                || beside->x - start->x != hspace)
            break;

        const MatrixElement*  elem = start;
        for (Uint j = 0;  j < nrows;  ++j) {
            if (elem->right == Null  ||  elem->right != beside)
                goto END_OUTER_LOOP;
            assert (! beside->isAllocated());
            elem = elem->up;
            beside = beside->up;
        }
        start = start->right;
        ++ncols;
    }
  END_OUTER_LOOP: ;

    return ncols;
}



// allocHorizontalLine -- mark allocated all MatrixElements in horizontal line
//   start      the leftmost MatrixElement in the line
//   ncols      number of MatrixElements in the line

void
PointGrouper::allocHorizontalLine (MatrixElement* start, Uint ncols)
{
    assert (ncols > 0);

    do {
        start->markAllocated();
        start = start->right;
    } while (--ncols != 0);
}



// allocVerticalLine -- mark allocated all MatrixElements in vertical line
//   start      bottom MatrixElement in the line
//   nrows      number of MatrixElements in the line

void
PointGrouper::allocVerticalLine (MatrixElement* start, Uint nrows)
{
    assert (nrows > 0);

    do {
        start->markAllocated();
        start = start->up;
    } while (--nrows != 0);
}



// allocArray   -- mark all MatrixElements in array
//   sw             southwest corner of array
//   ncols, nrows   number of columns and rows in array

void
PointGrouper::allocArray (MatrixElement* sw, Uint ncols, Uint nrows)
{
    assert (ncols > 0  &&  nrows > 0);

    do {
        allocHorizontalLine(sw, ncols);
        sw = sw->up;
    } while (--nrows != 0);
}


} // namespace GdsiiOasis
