// misc/arith.cc -- integer arithmetic with checks for overflow
//
// last modified:   28-Jul-2004  Wed  09:36
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <stdexcept>
#include "arith.h"
#include "utils.h"

namespace SoftJin {

using namespace std;


// IntegerOverflow -- throw overflow_error because of overflow in integer op.
//   op         the integer operation
//   x, y       the two operands of op
// To avoid using stringstreams we define four overloaded methods
// instead of a template method.

void
IntegerOverflow (int x, const char* op, int y)  /* throw (overflow_error) */
{
    char  buf[256];
    SNprintf(buf, sizeof buf, "integer overflow while computing %d %s %d",
             x, op, y);
    throw overflow_error(buf);
}



void
IntegerOverflow (Uint x, const char* op, Uint y)  /* throw (overflow_error) */
{
    char  buf[256];
    SNprintf(buf, sizeof buf, "integer overflow while computing %u %s %u",
             x, op, y);
    throw overflow_error(buf);
}



void
IntegerOverflow (long x, const char* op, long y)  /* throw (overflow_error) */
{
    char  buf[256];
    SNprintf(buf, sizeof buf, "integer overflow while computing %ld %s %ld",
             x, op, y);
    throw overflow_error(buf);
}



void
IntegerOverflow (Ulong x, const char* op, Ulong y) /* throw (overflow_error) */
{
    char  buf[256];
    SNprintf(buf, sizeof buf, "integer overflow while computing %lu %s %lu",
             x, op, y);
    throw overflow_error(buf);
}


} // namespace SoftJin
