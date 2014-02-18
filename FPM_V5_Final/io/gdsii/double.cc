// gdsii/double.cc -- convert between doubles and GDSII Stream format reals
//
// last modified:   08-Jan-2010  Fri  11:44
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cfloat>
#include <cmath>
#include <cstring>
#include "double.h"


namespace Gdsii {

using namespace std;


// GdsRealToDouble -- convert 8-byte real in GDSII Stream to double.
//   str        pointer to GDSII Stream representation of 8-byte
//              floating point number

double
GdsRealToDouble (const Uchar* str)
{
    // GDSII Stream uses the IBM 370 representation of floating-point
    // numbers.  The bytes are stored in big-endian order.  The leftmost
    // bit of the first byte is the sign bit.  The remaining bits of the
    // first byte form the base-16 exponent biased by 64.
    // Note that 16**n == (2**4)**n == 2**(4*n)

    double  sign = (str[0] & 0x80) ? -1.0 : 1.0;
    int  exponent = 4 * ((str[0] & 0x7f) - 64);         // base-2 exponent

    // The remaining 7 bytes form the significand (mantissa) in big-endian
    // order.  It's a base-16 float, so there is no hidden (implicit) bit
    // as in the base-2 IEEE 754 format.  Instead of treating each nibble
    // as a base-16 digit we treat each byte as a base-256 digit.

    double  significand = 0;
    double  divisor = 256.0;
    for (int j = 1;  j < 8;  ++j) {
        significand += str[j]/divisor;
        divisor *= 256.0;
    }

    // XXX: Strictly speaking we should check errno for overflow/underflow
    // after calling ldexp().  But if `double' is IEEE 754 double
    // precision, as it will almost certainly be, there will be neither
    // overflow nor underflow in ldexp() since the IEEE format has a
    // larger range for exponents.

    return (sign * ldexp(significand, exponent));
}



// DoubleToGdsReal -- convert a double to GDSII Stream 8-byte format
//   val        value to convert
//   buf        out: points to 8-byte area where the GDSII representation
//              should be be stored
// Returns true if the conversion succeeds, false if the input value is
// outside the range representable in GDSII.

bool
DoubleToGdsReal (double val, /*out*/ Uchar* buf)
{
    int         exponent;       // base-2 exponent of val
    double      significand;    // mantissa of val, range [0.5, 1.0)

    // Get this special case out of the way.  Otherwise the (biased)
    // exponent in the output will be 64 instead of 0.

    if (val == 0.0) {
        // XXX: what about -0.0 ?  Copy sign bit?
        memset(buf, 0, 8);
        return true;
    }

    // Remember the sign and make the value positive.

    int  signBit = 0;
    if (val < 0) {
        signBit = 0x80;
        val = - val;
    }

    // Split val into a base-2 significand and exponent.
    significand = frexp(val, &exponent);

    // To convert the base-2 exponent to a base-16 exponent we must
    // first make sure the exponent is a multiple of 4.  If it isn't, we
    // increment the exponent and shift the significand right so that it
    // is.  Since significand began in the range [1/2, 1), it ends in
    // the range [1/16, 1) as required for GDSII.  The +8192 ensures
    // that the left argument of the % operator is positive.

    if (int excess = (exponent+8192) % 4) {
        exponent += 4 - excess;
        significand = ldexp(significand, excess - 4);
    }

    // Convert the exponent to base 16 and add the bias.  Since the range
    // of exponents for IEEE 754 double precision is [-1022, 1023], the
    // result might not be representable in GDSII Stream format, which
    // allocates only 7 bits for the base-16 exponent.
    //
    // Because the GDSII format has 56 bits of precision and IEEE 754
    // double has only 53 bits, the largest GDSII real number
    // (0x7fff ffff ffff ffff == 7.2370055773322622e+75) when converted
    // to double cannot be converted back.  That is, the GDSII number
    // 0.11111...(56 1-bits) * 16**63 is converted to the double
    // 1.0 * 16**63 == 0.0625 * 16**64 because of the last three 1-bits.
    // This number is out of the GDSII range; so when converting back to
    // GDSII we treat it specially.

    exponent = exponent/4 + 64;
    if (exponent < 0  ||  exponent > 127) {
        if (exponent == 128  &&  significand == 0.0625) {
            --exponent;
            significand = 1.0 - DBL_EPSILON;
        } else
            return false;
    }

    // Write the first byte (the sign bit and exponent) and then the
    // significand.  We treat each byte of the output as a base-256 digit.
    // The loop essentially shifts the significand 8 bits left in each
    // iteration and writes the leftmost 8 bits (the integer part) to the
    // output.  Note that when *buf is assigned, the value of significand
    // is in the range [0, 256.0); so the conversion to Uchar is
    // well-defined.

    *buf++ = signBit | exponent;
    for (int j = 0;  j < 7;  ++j) {
        significand *= 256.0;
        *buf++ = static_cast<Uchar>(significand);
        significand -= floor(significand);
    }

    return true;
}


} // namespace Gdsii
