// gdsii/rectypes.cc -- GDSII record types and data types
//
// last modified:   07-Mar-2010  Sun  21:34
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include "glimits.h"
#include "rectypes.h"


namespace Gdsii {

using namespace GLimits;


// RLMIN/RLMAX -- min/max length of REFLIBS record.
// The GDSII Stream spec is not clear about whether a REFLIBS record can
// contain 15 or 2+15 libraries.  Compute the max length of a REFLIBS
// record from the constants in GLimits so that we need only change one
// place if we change our minds.  The min length is just for symmetry.

enum {
    RLMIN = ReflibNameLength * MinReflibs,
    RLMAX = ReflibNameLength * MaxReflibs,
};


// Other min/max record lengths also depend on the values in GLimits,
// but using those constants here is not worth the trouble.  There is no
// need to make a mess of this file to accommodate changes in the GDSII
// spec because for all practical purposes the spec is frozen.
//
// The min length for LIBNAME should be 2, not 0, but some GDSII files
// contain empty LIBNAME records.


/*static*/ const GdsRecordTypeInfo  GdsRecordTypeInfo::recTypeInfo[] =
{
//   type  valid         datatype  size minlen maxlen    name

    {  0,   true,     GDATA_SHORT,    2,    2,      2,  "HEADER"         },
    {  1,   true,     GDATA_SHORT,    2,   24,     24,  "BGNLIB"         },
    {  2,   true,    GDATA_STRING,    0,    0,  65530,  "LIBNAME"        },
    {  3,   true,    GDATA_DOUBLE,    8,   16,     16,  "UNITS"          },
    {  4,   true,      GDATA_NONE,    0,    0,      0,  "ENDLIB"         },
    {  5,   true,     GDATA_SHORT,    2,   24,     24,  "BGNSTR"         },

    {  6,   true,    GDATA_STRING,    0,    2,  65530,  "STRNAME"        },
    {  7,   true,      GDATA_NONE,    0,    0,      0,  "ENDSTR"         },
    {  8,   true,      GDATA_NONE,    0,    0,      0,  "BOUNDARY"       },
    {  9,   true,      GDATA_NONE,    0,    0,      0,  "PATH"           },
    { 10,   true,      GDATA_NONE,    0,    0,      0,  "SREF"           },

    { 11,   true,      GDATA_NONE,    0,    0,      0,  "AREF"           },
    { 12,   true,      GDATA_NONE,    0,    0,      0,  "TEXT"           },
    { 13,   true,     GDATA_SHORT,    2,    2,      2,  "LAYER"          },
    { 14,   true,     GDATA_SHORT,    2,    2,      2,  "DATATYPE"       },
    { 15,   true,       GDATA_INT,    4,    4,      4,  "WIDTH"          },

    { 16,   true,       GDATA_INT,    4,    8,  65528,  "XY"             },
    { 17,   true,      GDATA_NONE,    0,    0,      0,  "ENDEL"          },
    { 18,   true,    GDATA_STRING,    0,    2,  65530,  "SNAME"          },
    { 19,   true,     GDATA_SHORT,    2,    4,      4,  "COLROW"         },
    { 20,   true,      GDATA_NONE,    0,    0,      0,  "TEXTNODE"       },

    { 21,   true,      GDATA_NONE,    0,    0,      0,  "NODE"           },
    { 22,   true,     GDATA_SHORT,    2,    2,      2,  "TEXTTYPE"       },
    { 23,   true,  GDATA_BITARRAY,    2,    2,      2,  "PRESENTATION"   },
    { 24,  false,               0,    0,    0,      0,  "SPACING"        },
    { 25,   true,    GDATA_STRING,    0,    0,  65530,  "STRING"         },

    { 26,   true,  GDATA_BITARRAY,    2,    2,      2,  "STRANS"         },
    { 27,   true,    GDATA_DOUBLE,    8,    8,      8,  "MAG"            },
    { 28,   true,    GDATA_DOUBLE,    8,    8,      8,  "ANGLE"          },
    { 29,  false,               0,    0,    0,      0,  "UINTEGER"       },
    { 30,  false,               0,    0,    0,      0,  "USTRING"        },

    { 31,   true,    GDATA_STRING,   44, RLMIN, RLMAX,  "REFLIBS"        },
    { 32,   true,    GDATA_STRING,   44,  176,    176,  "FONTS"          },
    { 33,   true,     GDATA_SHORT,    2,    2,      2,  "PATHTYPE"       },
    { 34,   true,     GDATA_SHORT,    2,    2,      2,  "GENERATIONS"    },
    { 35,   true,    GDATA_STRING,    0,    0,  65530,  "ATTRTABLE"      },

    { 36,  false,               0,    0,    0,      0,  "STYPTABLE"      },
    { 37,  false,               0,    0,    0,      0,  "STRTYPE"        },
    { 38,   true,  GDATA_BITARRAY,    2,    2,      2,  "ELFLAGS"        },
    { 39,  false,               0,    0,    0,      0,  "ELKEY"          },
    { 40,  false,               0,    0,    0,      0,  "LINKTYPE"       },

    { 41,  false,               0,    0,    0,      0,  "LINKKEYS"       },
    { 42,   true,     GDATA_SHORT,    2,    2,      2,  "NODETYPE"       },
    { 43,   true,     GDATA_SHORT,    2,    2,      2,  "PROPATTR"       },
    { 44,   true,    GDATA_STRING,    0,    0,  65530,  "PROPVALUE"      },
    { 45,   true,      GDATA_NONE,    0,    0,      0,  "BOX"            },

    { 46,   true,     GDATA_SHORT,    2,    2,      2,  "BOXTYPE"        },
    { 47,   true,       GDATA_INT,    4,    4,      4,  "PLEX"           },
    { 48,   true,       GDATA_INT,    4,    4,      4,  "BGNEXTN"        },
    { 49,   true,       GDATA_INT,    4,    4,      4,  "ENDEXTN"        },
    { 50,   true,     GDATA_SHORT,    2,    2,      2,  "TAPENUM"        },

    { 51,   true,     GDATA_SHORT,    2,   12,     12,  "TAPECODE"       },
    { 52,   true,  GDATA_BITARRAY,    2,    2,      2,  "STRCLASS"       },
    { 53,  false,               0,    0,    0,      0,  "RESERVED"       },
    { 54,   true,     GDATA_SHORT,    2,    2,      2,  "FORMAT"         },
    { 55,   true,    GDATA_STRING,    0,    2,  65530,  "MASK"           },

    { 56,   true,      GDATA_NONE,    0,    0,      0,  "ENDMASKS"       },
    { 57,   true,     GDATA_SHORT,    2,    2,      2,  "LIBDIRSIZE"     },
    { 58,   true,    GDATA_STRING,    0,    2,  65530,  "SRFNAME"        },
    { 59,   true,     GDATA_SHORT,    2,    6,    192,  "LIBSECUR"       },
    { 60,   true,      GDATA_NONE,    0,    0,      0,  "BORDER"         },

    { 61,   true,      GDATA_NONE,    0,    0,      0,  "SOFTFENCE"      },
    { 62,   true,      GDATA_NONE,    0,    0,      0,  "HARDFENCE"      },
    { 63,   true,      GDATA_NONE,    0,    0,      0,  "SOFTWIRE"       },
    { 64,   true,      GDATA_NONE,    0,    0,      0,  "HARDWIRE"       },
    { 65,   true,      GDATA_NONE,    0,    0,      0,  "PATHPORT"       },

    { 66,   true,      GDATA_NONE,    0,    0,      0,  "NODEPORT"       },
    { 67,   true,      GDATA_NONE,    0,    0,      0,  "USERCONSTRAINT" },
    { 68,   true,      GDATA_NONE,    0,    0,      0,  "SPACER_ERROR"   },
    { 69,   true,      GDATA_NONE,    0,    0,      0,  "CONTACT"        },
};


}  // namespace Gdsii
