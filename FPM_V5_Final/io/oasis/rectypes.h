// oasis/rectypes.h -- record types and names
//
// last modified:   05-Sep-2004  Sun  07:10
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef OASIS_RECTYPES_H_INCLUDED
#define OASIS_RECTYPES_H_INCLUDED

#include <cassert>
#include "misc/globals.h"

namespace Oasis {

using SoftJin::Ulong;


// The record-IDs with an _R suffix are for <name> records that contain a
// reference-number.  The corresponding IDs without the _R suffix are
// for <name> records that use auto-numbering.

enum RecordID {
    RID_PAD                 =  0,
    RID_START               =  1,
    RID_END                 =  2,
    RID_CELLNAME            =  3,
    RID_CELLNAME_R          =  4,
    RID_TEXTSTRING          =  5,

    RID_TEXTSTRING_R        =  6,
    RID_PROPNAME            =  7,
    RID_PROPNAME_R          =  8,
    RID_PROPSTRING          =  9,
    RID_PROPSTRING_R        = 10,

    RID_LAYERNAME_GEOMETRY  = 11,
    RID_LAYERNAME_TEXT      = 12,
    RID_CELL_REF            = 13,
    RID_CELL_NAMED          = 14,
    RID_XYABSOLUTE          = 15,

    RID_XYRELATIVE          = 16,
    RID_PLACEMENT           = 17,
    RID_PLACEMENT_TRANSFORM = 18,
    RID_TEXT                = 19,
    RID_RECTANGLE           = 20,

    RID_POLYGON             = 21,
    RID_PATH                = 22,
    RID_TRAPEZOID           = 23,
    RID_TRAPEZOID_A         = 24,
    RID_TRAPEZOID_B         = 25,

    RID_CTRAPEZOID          = 26,
    RID_CIRCLE              = 27,
    RID_PROPERTY            = 28,
    RID_PROPERTY_REPEAT     = 29,
    RID_XNAME               = 30,

    RID_XNAME_R             = 31,
    RID_XELEMENT            = 32,
    RID_XGEOMETRY           = 33,
    RID_CBLOCK              = 34,

    RID_MaxID               = 34
};



// Define IDs for pseudo-records used only in the ASCII version of OASIS
// files.  The functions below and the table in rectypes.cc assume that
// the values of these enumerators follow the RID_* values with no gap.
//
// For every RID_* and RIDX_* enumerator there must be an entry in
// oasisRecordNames[] in rectypes.cc and a scanner rule in
// asc-scanner.l.


enum {
    RIDX_END_CBLOCK         = 35,
    RIDX_CELLNAME_TABLE     = 36,
    RIDX_TEXTSTRING_TABLE   = 37,
    RIDX_PROPNAME_TABLE     = 38,
    RIDX_PROPSTRING_TABLE   = 39,
    RIDX_LAYERNAME_TABLE    = 40,
    RIDX_XNAME_TABLE        = 41,

    RIDX_MaxID              = 41
};



// For each record type and pseudo-record type we have two names.  One
// is the name printed in error messages, and the other is the name
// printed in the ASCII file.  The ASCII record name is different
// because some record types come in variants that cannot be
// distinguished from the contents of the record, e.g., LAYERNAME for
// geometry and for text.  Each variant must be given a unique name for
// the ASCII file to be unambiguous.

struct OasisRecordNames {
    const char*  errName;       // name for error messages
    const char*  ascName;       // name for the ASCII file
};

extern const OasisRecordNames   oasisRecordNames[];


inline const char*
GetRecordErrName (Ulong recID) {
    assert (recID <= RIDX_MaxID);
    return oasisRecordNames[recID].errName;
}


inline const char*
GetRecordAscName (Ulong recID) {
    assert (recID <= RIDX_MaxID);
    return oasisRecordNames[recID].ascName;
}



// The scanner needs to check that a record-ID that appears in an OASIS
// file is valid.  Note that the RIDX_* values are not allowed.

inline bool
RecordTypeIsValid (Ulong recID) {
    return (recID <= RID_MaxID);
}


}  // namespace Oasis

#endif  // OASIS_RECTYPES_H_INCLUDED
