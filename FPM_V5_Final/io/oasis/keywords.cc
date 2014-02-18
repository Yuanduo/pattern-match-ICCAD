// oasis/keywords.cc -- names of record fields in the ASCII file
//
// last modified:   17-Aug-2004  Tue  16:26
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include "keywords.h"

namespace Oasis {


// Make sure that the field names here do not conflict with any ascName
// in rectypes.cc.  If you change or add a name, update asc-scanner.l to
// match.


const char *const  asciiKeywordNames[] = {
    "angle",            // KeyAngle
    "attribute",        // KeyAttribute
    "checksum32",       // KeyChecksum32
    "crc32",            // KeyCRC32
    "datatype",         // KeyDatatype
    "delta_a",          // KeyDelta_A
    "delta_b",          // KeyDelta_B
    "end_extn",         // KeyEndExtn
    "flip",             // KeyFlip
    "halfwidth",        // KeyHalfwidth
    "height",           // KeyHeight
    "horizontal",       // KeyHorizontal
    "layer",            // KeyLayer
    "mag",              // KeyMag
    "name",             // KeyName
    "none",             // KeyNone
    "ptlist",           // KeyPointList
    "radius",           // KeyRadius
    "refnum",           // KeyRefnum
    "rep",              // KeyRepetition
    "square",           // KeySquare
    "standard",         // KeyStandard
    "start_extn",       // KeyStartExtn
    "string",           // KeyString
    "textlayer",        // KeyTextlayer
    "texttype",         // KeyTexttype
    "ctraptype",        // KeyTrapType
    "values",           // KeyValues
    "vertical",         // KeyVertical
    "width",            // KeyWidth
    "x",                // KeyX
    "y",                // KeyY
};


} // namespace Oasis
