// oasis/asc-conv.h -- convert between OASIS format and its ASCII version
//
// last modified:   24-Dec-2009  Thu  16:41
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef OASIS_ASC_CONV_H_INCLUDED
#define OASIS_ASC_CONV_H_INCLUDED

#include "misc/utils.h"         // for WarningHandler
#include "oasis.h"


namespace Oasis {

using SoftJin::WarningHandler;


// OasisToAsciiOptions -- options for the OASIS->ASCII conversion
//   showOffsets
//      true => print the file position of each record just before
//              the record
//
//   foldLongLines
//      true => insert newlines in records to try to keep lines from becoming
//              too long
//      false => print each record on one line

struct OasisToAsciiOptions {
    bool        showOffsets;
    bool        foldLongLines;

    OasisToAsciiOptions() {
        showOffsets = false;
        foldLongLines = true;
    }
};


void
ConvertOasisToAscii (const char*  infilename,
                     const char*  outfilename,
                     WarningHandler  warner,
                     const OasisToAsciiOptions&  options);



// AsciiToOasisOptions -- options for OASIS->ASCII conversion
//   valScheme
//      Validation scheme to use for the OASIS file.  The validation scheme
//      in the END record of the ASCII file is ignored.

struct AsciiToOasisOptions {
    Validation::Scheme  valScheme;

    explicit
    AsciiToOasisOptions() {
        valScheme = Validation::Checksum32;
    }
};


void
ConvertAsciiToOasis (const char*  infilename,
                     const char*  outfilename,
                     WarningHandler  warner,
                     const AsciiToOasisOptions&  options);


} // namespace Oasis

#endif  // OASIS_ASC_CONV_H_INCLUDED
