// gdsii/asc-conv.h -- convert between GDSII Stream format and ASCII
//
// last modified:   26-Nov-2004  Fri  21:38
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef GDSII_ASC_CONV_H_INCLUDED
#define GDSII_ASC_CONV_H_INCLUDED

#include <sys/types.h>          // for off_t
#include "misc/gzfile.h"


namespace Gdsii {

using SoftJin::FileHandle;


// GdsToAsciiOptions -- settings for GDSII -> ASCII conversion process.
// The member gdsFileType says whether the input GDSII file is gzipped.
// If FileTypeGzip, it is gzipped.  If FileTypeNormal, it isn't.
// If FileTypeAuto, the file is gzipped iff its name ends with .gz.

struct GdsToAsciiOptions {
    bool        showOffsets;    // print file offset of each record
    bool        convertUnits;   // print XY coordinates in user units
    FileHandle::FileType  gdsFileType;  // is the GDSII file gzipped?

    GdsToAsciiOptions() {
        showOffsets = false;
        convertUnits = false;
        gdsFileType = FileHandle::FileTypeAuto;
    }

    GdsToAsciiOptions (bool offsets, bool convert, FileHandle::FileType ftype)
    {
        showOffsets = offsets;
        convertUnits = convert;
        gdsFileType = ftype;
    }
};



// AsciiToGdsOptions -- settings for ASCII -> GDSII conversion process.

struct AsciiToGdsOptions {
    FileHandle::FileType  gdsFileType;  // should the GDSII file be gzipped?

    AsciiToGdsOptions() {
        gdsFileType = FileHandle::FileTypeAuto;
    }

    AsciiToGdsOptions (FileHandle::FileType ftype) {
        gdsFileType = ftype;
    }
};



// All the functions below may throw runtime_error.

void    ConvertGdsToAscii (const char* infilename,
                           const char* outfilename,
                           const GdsToAsciiOptions& options);

void    ConvertGdsToAscii (const char* infilename,
                           const char* outfilename,
                           const GdsToAsciiOptions& options,
                           const char* sname,
                           off_t soffset = -1);

void    ConvertAsciiToGds (const char* infilename,
                           const char* outfilename,
                           const AsciiToGdsOptions& options);


}  // namespace Gdsii

#endif  // GDSII_ASC_CONV_H_INCLUDED
