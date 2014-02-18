// oasis/validator.cc -- compute validation signature of OASIS file
//
// last modified:   19-Jul-2004  Mon  23:34
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <zlib.h>
#include "validator.h"

namespace Oasis {

using SoftJin::Uchar;


Validator::Validator() { }
/*virtual*/ Validator::~Validator() { }


//----------------------------------------------------------------------


Crc32Validator::Crc32Validator() {
    crc = crc32(crc, Null, 0);
}


/*virtual*/ Crc32Validator::~Crc32Validator() { }


/*virtual*/ void
Crc32Validator::add (const char* buf, Uint nbytes) {
    crc = crc32(crc, reinterpret_cast<const Bytef*>(buf), nbytes);
}


/*virtual*/ uint32_t
Crc32Validator::getSignature() {
    return crc;
}


//----------------------------------------------------------------------



Checksum32Validator::Checksum32Validator() {
    checksum = 0;
}


/*virtual*/ Checksum32Validator::~Checksum32Validator() { }


/*virtual*/ void
Checksum32Validator::add (const char* buf, Uint nbytes)
{
    const Uchar*  ucp = reinterpret_cast<const Uchar*>(buf);
    const Uchar*  end = ucp + nbytes;
    for ( ;  ucp != end;  ++ucp)
        checksum += *ucp;
}


/*virtual*/ uint32_t
Checksum32Validator::getSignature() {
    return checksum;
}


}  // namespace Oasis
