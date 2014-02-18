// oasis/keywords.h -- names of record fields in the ASCII file
//
// last modified:   04-Sep-2004  Sat  15:59
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// Some keywords appear as the values of fields:
//   - halfwidth     may appear as the value of start_extn or end_extn
//   - none, crc32, checksum32
//                   may appear in the END record as the type of the
//                   validation signature
//
// Some keywords have no value; their presence indicates that a bit
// in the info-byte should be set.
//   - flip         F bit in PLACEMENT records
//   - square       S bit in RECTANGLE records
//   - vertical     O bit in TRAPEZOID records
//   - horizontal   also for O bit in TRAPEZOID records: sets it to 0


#ifndef OASIS_KEYWORDS_H_INCLUDED
#define OASIS_KEYWORDS_H_INCLUDED

namespace Oasis {


// Keywords used to identify the fields in the ASCII version of OASIS
// files.  If you add any keywords here, add them to keywords.cc and
// asc-scanner.l too.

enum AsciiKeyword {
    KeyAngle,
    KeyAttribute,
    KeyChecksum32,
    KeyCRC32,
    KeyDatatype,
    KeyDelta_A,
    KeyDelta_B,
    KeyEndExtn,
    KeyFlip,
    KeyHalfwidth,
    KeyHeight,
    KeyHorizontal,
    KeyLayer,
    KeyMag,
    KeyName,
    KeyNone,
    KeyPointList,
    KeyRadius,
    KeyRefnum,
    KeyRepetition,
    KeySquare,
    KeyStandard,
    KeyStartExtn,
    KeyString,
    KeyTextlayer,
    KeyTexttype,
    KeyTrapType,
    KeyValues,
    KeyVertical,
    KeyWidth,
    KeyX,
    KeyY,
};


extern const char *const   asciiKeywordNames[];


inline const char*
GetKeywordName (AsciiKeyword kword) {
    return asciiKeywordNames[kword];
}


}

#endif  // OASIS_KEYWORDS_H_INCLUDED
