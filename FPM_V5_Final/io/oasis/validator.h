// oasis/validator.h -- compute validation signature of OASIS file
//
// last modified:   02-Sep-2004  Thu  23:32
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// This file defines an abstract strategy class Validator to compute the
// validation signature of an OASIS file, and two concrete strategy
// subclasses, Crc32Validator and Checksum32Validator.  OasisScanner and
// OasisWriter instantiate the appropriate concrete subclass to compute
// the signature.


#ifndef OASIS_VALIDATOR_H_INCLUDED
#define OASIS_VALIDATOR_H_INCLUDED

#include <inttypes.h>
#include "misc/globals.h"


namespace Oasis {

using SoftJin::Uint;
using SoftJin::Ulong;


// Validator -- strategy class to compute validation signature of OASIS file.
//
// add (const char* buf, Uint nbytes)
//
//      Adjusts the validation signature to include the nbytes
//      characters beginning at buf.  Its nbytes argument is Uint and
//      not size_t because that is what zlib's crc32() wants.
//
// uint32_t  getSignature()
//
//      Returns the current value of the validation signature.

class Validator {
public:
                        Validator();
    virtual             ~Validator();
    virtual void        add (const char* buf, Uint nbytes) = 0;
    virtual uint32_t    getSignature() = 0;
};



// Crc32Validator -- compute CRC32 of input
// crc is Ulong and not uint32_t because that is what zlib's crc32()
// wants.  (Actually crc32() uses the zlib typedef uLong.)
// getSignature() returns the lower 32 bits.

class Crc32Validator : public Validator {
    Ulong       crc;
public:
                        Crc32Validator();
    virtual             ~Crc32Validator();
    virtual void        add (const char* buf, Uint nbytes);
    virtual uint32_t    getSignature();
};



// Checksum32Validator -- compute 32-bit checksum of input

class Checksum32Validator : public Validator {
    uint32_t    checksum;
public:
                        Checksum32Validator();
    virtual             ~Checksum32Validator();
    virtual void        add (const char* buf, Uint nbytes);
    virtual uint32_t    getSignature();
};


}  // namespace Oasis

#endif  // OASIS_VALIDATOR_H_INCLUDED
