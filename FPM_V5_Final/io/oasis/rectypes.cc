// oasis/rectypes.cc -- record types and names
//
// last modified:   31-Dec-2009  Thu  10:28
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include "rectypes.h"


namespace Oasis {


// The ascName of each record type is used for the ASCII version in
// OASIS files.  If you change any ascName, change the corresponding
// rule in asc-scanner.l too.


const struct OasisRecordNames  oasisRecordNames[] =
{
//    errName          ascName

    { "PAD",           "PAD"                },   //  0  RID_PAD
    { "START",         "START"              },   //  1  RID_START
    { "END",           "END"                },   //  2  RID_END
    { "CELLNAME",      "CELLNAME"           },   //  3  RID_CELLNAME
    { "CELLNAME",      "CELLNAME"           },   //  4  RID_CELLNAME_R
    { "TEXTSTRING",    "TEXTSTRING"         },   //  5  RID_TEXTSTRING

    { "TEXTSTRING",    "TEXTSTRING"         },   //  6  RID_TEXTSTRING_R
    { "PROPNAME",      "PROPNAME"           },   //  7  RID_PROPNAME
    { "PROPNAME",      "PROPNAME"           },   //  8  RID_PROPNAME_R
    { "PROPSTRING",    "PROPSTRING"         },   //  9  RID_PROPSTRING
    { "PROPSTRING",    "PROPSTRING"         },   // 10  RID_PROPSTRING_R

    { "LAYERNAME",     "LAYERNAME_GEOMETRY" },   // 11  RID_LAYERNAME_GEOMETRY
    { "LAYERNAME",     "LAYERNAME_TEXT"     },   // 12  RID_LAYERNAME_TEXT
    { "CELL",          "CELL"               },   // 13  RID_CELL_REF
    { "CELL",          "CELL"               },   // 14  RID_CELL_NAMED
    { "XYABSOLUTE",    "XYABSOLUTE"         },   // 15  RID_XYABSOLUTE

    { "XYRELATIVE",    "XYRELATIVE"         },   // 16  RID_XYRELATIVE
    { "PLACEMENT",     "PLACEMENT"          },   // 17  RID_PLACEMENT
    { "PLACEMENT",     "PLACEMENT_X"        },   // 18  RID_PLACEMENT_TRANSFORM
    { "TEXT",          "TEXT"               },   // 19  RID_TEXT
    { "RECTANGLE",     "RECTANGLE"          },   // 20  RID_RECTANGLE

    { "POLYGON",       "POLYGON"            },   // 21  RID_POLYGON
    { "PATH",          "PATH"               },   // 22  RID_PATH
    { "TRAPEZOID",     "TRAPEZOID"          },   // 23  RID_TRAPEZOID
    { "TRAPEZOID",     "TRAPEZOID_A"        },   // 24  RID_TRAPEZOID_A
    { "TRAPEZOID",     "TRAPEZOID_B"        },   // 25  RID_TRAPEZOID_B

    { "CTRAPEZOID",    "CTRAPEZOID"         },   // 26  RID_CTRAPEZOID
    { "CIRCLE",        "CIRCLE"             },   // 27  RID_CIRCLE
    { "PROPERTY",      "PROPERTY"           },   // 28  RID_PROPERTY
    { "PROPERTY",      "PROPERTY_REPEAT"    },   // 29  RID_PROPERTY_REPEAT
    { "XNAME",         "XNAME"              },   // 30  RID_XNAME

    { "XNAME",         "XNAME"              },   // 31  RID_XNAME_R
    { "XELEMENT",      "XELEMENT"           },   // 32  RID_XELEMENT
    { "XGEOMETRY",     "XGEOMETRY"          },   // 33  RID_XGEOMETRY
    { "CBLOCK",        "CBLOCK"             },   // 34  RID_CBLOCK

    { "END_CBLOCK",    "END_CBLOCK"         },   // 35  RIDX_END_CBLOCK
    { "NAME_TABLE",    "CELLNAME_TABLE"     },   // 36  RIDX_CELLNAME_TABLE
    { "NAME_TABLE",    "TEXTSTRING_TABLE"   },   // 37  RIDX_TEXTSTRING_TABLE
    { "NAME_TABLE",    "PROPNAME_TABLE"     },   // 38  RIDX_PROPNAME_TABLE
    { "NAME_TABLE",    "PROPSTRING_TABLE"   },   // 39  RIDX_PROPSTRING_TABLE
    { "NAME_TABLE",    "LAYERNAME_TABLE"    },   // 40  RIDX_LAYERNAME_TABLE
    { "NAME_TABLE",    "XNAME_TABLE"        },   // 41  RIDX_XNAME_TABLE
};


}  // namespace Oasis
