// misc/arith.h -- integer arithmetic with checks for overflow
//
// last modified:   16-Feb-2005  Wed  11:14
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef MISC_ARITH_H_INCLUDED
#define MISC_ARITH_H_INCLUDED

#include <limits>
#include "globals.h"

namespace SoftJin {


// These functions format an error message and throw overflow_error.
void  IntegerOverflow (int x,   const char* op, int y);
void  IntegerOverflow (long x,  const char* op, long y);
void  IntegerOverflow (Uint x,  const char* op, Uint y);
void  IntegerOverflow (Ulong x, const char* op, Ulong y);


// We do not allow the negative value -MaxVal - 1 (-2^31 or -2^63) even
// though it can be represented.  By forbidding it we avoid having to
// test for overflow when negating numbers.


template <typename IntType>
IntType
CheckedPlus (IntType x, IntType y)
{
    const IntType  MaxVal = std::numeric_limits<IntType>::max();

    if ((x > 0  &&  y > 0  &&  x > MaxVal - y)
            ||  (x < 0  &&  y < 0  &&  x < -MaxVal - y))
        IntegerOverflow(x, "+", y);
    return (x + y);
}



template <typename IntType>
IntType
CheckedMinus (IntType x, IntType y)
{
    const IntType  MaxVal = std::numeric_limits<IntType>::max();

    if ((x > 0  &&  y < 0  &&  x > MaxVal + y)
            ||  (x < 0  &&  y > 0  &&  x < -MaxVal + y))
        IntegerOverflow(x, "-", y);
    return (x - y);
}



template <typename IntType>
IntType
CheckedMult (IntType x, IntType y)
{
    const IntType  MaxVal = std::numeric_limits<IntType>::max();

    if (y != 0  &&  abs(x) > MaxVal/abs(y))
        IntegerOverflow(x, "*", y);
    return (x * y);
}



// There is no danger of overflow with doubles for the numbers we work
// with.  Disable overflow checks for them.

template <>
inline double
CheckedPlus<double> (double x, double y) {
    return (x + y);
}


template <>
inline double
CheckedMinus<double> (double x, double y) {
    return (x - y);
}


template <>
inline double
CheckedMult<double> (double x, double y) {
    return (x * y);
}


} // namespace SoftJin

#endif  // MISC_ARITH_H_INCLUDED
