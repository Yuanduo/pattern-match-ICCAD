// misc/geometry.h -- primitives for 2-dimensional geometry
//
// last modified:   02-Mar-2005  Wed  14:35

#ifndef MISC_GEOMETRY_H_INCLUDED
#define MISC_GEOMETRY_H_INCLUDED

#include <algorithm>            // for min(), max()
#include <cassert>
#include <limits>
#include <boost/static_assert.hpp>
#include "arith.h"


namespace SoftJin {

using std::numeric_limits;


//======================================================================
//                              Point2d
//======================================================================

// Point2d -- point in two dimensions
//
// The coordinates of a point pt may be accessed either as pt.x and pt.y
// or as pt.coord(0) and pt.coord(1).  The latter is more convenient for
// code that works with either dimension.  The data members are public
// because no abstraction is provided, and pt.x is less cluttered than
// pt.x().
//
// The template argument must be a signed type.  If it is integral, do
// not assign the most negative value (-2^31 or -2^63) to either member.


template <typename NumberT>
struct Point2d {
    typedef NumberT  CoordType;

    union {
        struct {
            NumberT x;
            NumberT y;
        };
        NumberT  coords[2];
    };

public:
    Point2d() { }

    Point2d (NumberT x, NumberT y) {
        this->x = x;
        this->y = y;
    }

    Point2d (const Point2d& pt) {
        x = pt.x;
        y = pt.y;
    }

    const Point2d&
    operator= (const Point2d& pt) {
        x = pt.x;
        y = pt.y;
        return *this;
    }

    const Point2d&
    assign (NumberT x, NumberT y) {
        this->x = x;
        this->y = y;
        return *this;
    }

    // Template versions of copy construction and copy assignment.  These
    // may be used e.g. to copy a Point2d<int> to a Point2d<long>.
    // The caller must ensure that the copy will not cause an overflow.
    //
    // XXX: what if NumberT is integral and T is double?  Round instead of
    // truncating?

    template <typename T>
    Point2d (const Point2d<T>& pt) {
        x = static_cast<NumberT>(pt.x);
        y = static_cast<NumberT>(pt.y);
    }

    template <typename T>
    const Point2d&
    operator= (const Point2d<T>& pt) {
        x = static_cast<NumberT>(pt.x);
        y = static_cast<NumberT>(pt.y);
        return *this;
    }

    // Although the data member 'coords' is public, it is cleaner to
    // access it using these methods.

    NumberT     coord (Uint dimen) const { return coords[dimen]; }
    NumberT&    coord (Uint dimen)       { return coords[dimen]; }

    const Point2d&  operator+= (const Point2d& pt);
    const Point2d&  operator-= (const Point2d& pt);
    
private:
    BOOST_STATIC_ASSERT (numeric_limits<NumberT>::is_specialized
                     &&  numeric_limits<NumberT>::is_signed);
};



template <typename NumberT>
inline bool
operator== (const Point2d<NumberT>& lhs, const Point2d<NumberT>& rhs) {
    return (lhs.x == rhs.x  &&  lhs.y == rhs.y);
}


template <typename NumberT>
inline bool
operator!= (const Point2d<NumberT>& lhs, const Point2d<NumberT>& rhs) {
    return (! (lhs == rhs));
}


//----------------------------------------------------------------------
// Minor abuse of notation: Add and subtract points as if they were vectors.


template <typename NumberT>
inline const Point2d<NumberT>&
Point2d<NumberT>::operator+= (const Point2d<NumberT>& pt)
{
    x = CheckedPlus(x, pt.x);
    y = CheckedPlus(y, pt.y);
    return *this;
}



template <typename NumberT>
inline const Point2d<NumberT>&
Point2d<NumberT>::operator-= (const Point2d<NumberT>& pt)
{
    x = CheckedMinus(x, pt.x);
    y = CheckedMinus(y, pt.y);
    return *this;
}



template <typename NumberT>
inline const Point2d<NumberT>
operator+ (const Point2d<NumberT>& lhs, const Point2d<NumberT>& rhs)
{
    Point2d<NumberT>  result = lhs;
    result += rhs;
    return result;
}



template <typename NumberT>
inline const Point2d<NumberT>
operator- (const Point2d<NumberT>& lhs, const Point2d<NumberT>& rhs)
{
    Point2d<NumberT>  result = lhs;
    result -= rhs;
    return result;
}



template <typename NumberT>
inline const Point2d<NumberT>
operator- (const Point2d<NumberT>& pt)
{
    return (Point2d<NumberT>(-pt.x, -pt.y));
}



//======================================================================
//                              Box
//======================================================================

// Box -- axis-aligned rectangle for 2-D bounding boxes

template <typename NumberT>
class Box {
public:
    typedef NumberT  CoordType;

private:
    NumberT     xmin, xmax, ymin, ymax;

public:
    Box() { }
    Box (const Point2d<NumberT>& pa, const Point2d<NumberT>& pb);
    Box (NumberT xmin, NumberT xmax, NumberT ymin, NumberT ymax);

    template <typename PointIter>
    Box (PointIter iter, PointIter end);

    void        init (const Point2d<NumberT>& pa, const Point2d<NumberT>& pb);
    void        init (NumberT xmin, NumberT xmax, NumberT ymin, NumberT ymax);

    template <typename PointIter>
    void        init (PointIter iter, PointIter end);

    void        makeEmpty();
    bool        empty() const;

    NumberT     getXmin() const  { return xmin; }
    NumberT     getXmax() const  { return xmax; }
    NumberT     getYmin() const  { return ymin; }
    NumberT     getYmax() const  { return ymax; }

    const Box&  operator|= (const Box<NumberT>& box);
    const Box&  operator&= (const Box<NumberT>& box);
    const Box&  operator+= (const Point2d<NumberT>& offset);
    const Box&  operator-= (const Point2d<NumberT>& offset);

    bool        overlaps (const Box<NumberT>& box) const;
    bool        contains (const Box<NumberT>& box) const;
    bool        containedIn (const Box<NumberT>& box) const;
    bool        contains (const Point2d<NumberT>& pt) const;

    static const Box<NumberT>  universe();

private:
    BOOST_STATIC_ASSERT (numeric_limits<NumberT>::is_specialized
                     &&  numeric_limits<NumberT>::is_signed);
};



// init -- intialize box given the coordinates of its four sides.
// Note that it is okay if xmin >= xmax or ymin >= max.
// The box then becomes empty (see below).

template <typename NumberT>
inline void
Box<NumberT>::init (NumberT xmin, NumberT xmax, NumberT ymin, NumberT ymax)
{
    this->xmin = xmin;
    this->xmax = xmax;
    this->ymin = ymin;
    this->ymax = ymax;
}



// init -- initialize box given diagonally opposite vertices

template <typename NumberT>
void
Box<NumberT>::init (const Point2d<NumberT>& pa, const Point2d<NumberT>& pb)
{
    xmin = std::min(pa.x, pb.x);
    xmax = std::max(pa.x, pb.x);
    ymin = std::min(pa.y, pb.y);
    ymax = std::max(pa.y, pb.y);
}



// init -- set this to the smallest box that covers a set of points.
// The two iterator arguments identify a range of values in the usual
// STL way.  The iterator's value_type must be Point2d<NumberT>.

template <typename NumberT>
template <typename PointIter>
void
Box<NumberT>::init (PointIter iter, PointIter end)
{
    assert (iter != end);
    xmin = xmax = iter->x;
    ymin = ymax = iter->y;

    for (++iter;  iter != end;  ++iter) {
        Point2d<NumberT>  pt(*iter);
        if (pt.x < xmin)
            xmin = pt.x;
        else if (pt.x > xmax)
            xmax = pt.x;

        if (pt.y < ymin)
            ymin = pt.y;
        else if (pt.y > ymax)
            ymax = pt.y;
    }
}


//----------------------------------------------------------------------
// Each constructor just invokes the corresponding init().


template <typename NumberT>
inline
Box<NumberT>::Box (NumberT xmin, NumberT xmax, NumberT ymin, NumberT ymax)
{
    init(xmin, xmax, ymin, ymax);
}



template <typename NumberT>
inline
Box<NumberT>::Box (const Point2d<NumberT>& pa, const Point2d<NumberT>& pb)
{
    init(pa, pb);
}



template <typename NumberT>
template <typename PointIter>
inline
Box<NumberT>::Box (PointIter iter, PointIter end) {
    init(iter, end);
}


//----------------------------------------------------------------------
// A box is empty if its width or height is <= 0.  An empty box does not
// contain or overlap any box.


template <typename NumberT>
inline void
Box<NumberT>::makeEmpty()
{
    xmin = xmax = ymin = ymax = 0;
}



template <typename NumberT>
inline bool
Box<NumberT>::empty() const
{
    return (xmin >= xmax  ||  ymin >= ymax);
}


//----------------------------------------------------------------------
// Union and intersection of boxes.


template <typename NumberT>
const Box<NumberT>&
Box<NumberT>::operator|= (const Box<NumberT>& box)
{
    if (box.empty())
        return;
    if (empty()) {
        *this = box;
        return;
    }

    if (box.xmin < xmin)  xmin = box.xmin;
    if (box.xmax > xmax)  xmax = box.xmax;
    if (box.ymin < ymin)  ymin = box.ymin;
    if (box.ymax > ymax)  ymax = box.ymax;
    return *this;
}



template <typename NumberT>
const Box<NumberT>&
Box<NumberT>::operator&= (const Box<NumberT>& box)
{
    if (empty())
        return;
    if (box.empty()) {
        makeEmpty();
        return;
    }

    if (box.xmin > xmin)  xmin = box.xmin;
    if (box.xmax < xmax)  xmax = box.xmax;
    if (box.ymin > ymin)  ymin = box.ymin;
    if (box.ymax < ymax)  ymax = box.ymax;
    return *this;
}



template <typename NumberT>
const Box<NumberT>
operator| (const Box<NumberT>& lhs, const Box<NumberT>& rhs)
{
    Box<NumberT> result = lhs;
    result |= rhs;
    return result;
}



template <typename NumberT>
const Box<NumberT>
operator& (const Box<NumberT>& lhs, const Box<NumberT>& rhs)
{
    Box<NumberT> result = lhs;
    result &= rhs;
    return result;
}


//----------------------------------------------------------------------
// Translate boxes


template <typename NumberT>
const Box<NumberT>&
Box<NumberT>::operator+= (const Point2d<NumberT>& offset)
{
    xmin = CheckedPlus(xmin, offset.x);
    xmax = CheckedPlus(xmax, offset.x);
    ymin = CheckedPlus(ymin, offset.y);
    ymax = CheckedPlus(ymax, offset.y);
    return *this;
}



template <typename NumberT>
const Box<NumberT>&
Box<NumberT>::operator-= (const Point2d<NumberT>& offset)
{
    xmin = CheckedMinus(xmin, offset.x);
    xmax = CheckedMinus(xmax, offset.x);
    ymin = CheckedMinus(ymin, offset.y);
    ymax = CheckedMinus(ymax, offset.y);
    return *this;
}



template <typename NumberT>
const Box<NumberT>
operator+ (const Box<NumberT>& box, const Point2d<NumberT>& offset)
{
    Box<NumberT> result = box;
    result += offset;
    return result;
}



template <typename NumberT>
const Box<NumberT>
operator- (const Box<NumberT>& box, const Point2d<NumberT>& offset)
{
    Box<NumberT> result = box;
    result -= offset;
    return result;
}


//----------------------------------------------------------------------


// overlaps -- true if the intersection of the two boxes is non-empty

template <typename NumberT>
bool
Box<NumberT>::overlaps (const Box<NumberT>& box) const
{
    return (! box.empty()
            &&  xmin < box.xmax  &&  xmax > box.xmin
            &&  ymin < box.ymax  &&  ymax > box.ymin);
}



// contains -- true if (*this | box) is the same as *this
// XXX: It is not clear how we should treat empty boxes.  contains()
// returns true if both boxes are empty.

template <typename NumberT>
bool
Box<NumberT>::contains (const Box<NumberT>& box) const
{
    if (box.empty())
        return true;
    return (xmin <= box.xmin  &&  xmax >= box.xmax
        &&  ymin <= box.ymin  &&  ymax >= box.ymax);
}


template <typename NumberT>
inline bool
Box<NumberT>::containedIn (const Box<NumberT>& box) const
{
    return (box.contains(*this));
}



// contains -- true if point lies inside or on boundary of box.
// XXX: Here too empty boxes are tricky.

template <typename NumberT>
inline bool
Box<NumberT>::contains (const Point2d<NumberT>& pt) const
{
    return (xmin <= pt.x  &&  xmax >= pt.x
        &&  ymin <= pt.y  &&  ymax >= pt.y);
}


//----------------------------------------------------------------------
// Compare boxes.
// XXX: Should we consider all empty boxes to be equal?


template <typename NumberT>
bool
operator== (const Box<NumberT>& lhs, const Box<NumberT>& rhs)
{
    return (lhs.getXmin() == rhs.getXmin()
        &&  lhs.getXmax() == rhs.getXmax()
        &&  lhs.getYmin() == rhs.getYmin()
        &&  lhs.getYmax() == rhs.getYmax() );
}


template <typename NumberT>
inline bool
operator!= (const Box<NumberT>& lhs, const Box<NumberT>& rhs)
{
    return (! (lhs == rhs));
}


//----------------------------------------------------------------------


// universe -- return a box that covers the entire number space.
// For example, Box<int32_t>::universe() is
// Box<int32_t>(-2^31, 2^31 - 1, -2^31, 2^31 - 1).

template <typename NumberT>
/*static*/ const Box<NumberT>
Box<NumberT>::universe()
{
    // For integer types the range is min() .. max().  For float and
    // double the range is -max() .. max() because then
    // numeric_limits<T>::min() is the smallest positive value.
    //
    // XXX: Might want to use -max() even for integers, on the general
    // principle of avoiding -2^31 and -2^63 everywhere because of
    // the risk of overflow when negating.

    if (numeric_limits<NumberT>::is_integer) {
        const NumberT  RangeMin = numeric_limits<NumberT>::min();
        const NumberT  RangeMax = numeric_limits<NumberT>::max();
        return Box(RangeMin, RangeMax, RangeMin, RangeMax);
    } else {
        const NumberT  RangeMax = numeric_limits<NumberT>::max();
        return Box(-RangeMax, RangeMax, -RangeMax, RangeMax);
    }
}


}  // namespace SoftJin

#endif  // MISC_GEOMETRY_H_INCLUDED
