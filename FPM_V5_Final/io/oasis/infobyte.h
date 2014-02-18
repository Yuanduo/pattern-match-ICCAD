// oasis/infobyte.h -- bits for the info-bytes in several records
//
// last modified:   26-Dec-2009  Sat  09:03
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// For each record we define the info-byte bits in a separate namespace.
// Any function that needs those bits uses a 'using namespace' directive
// to bring the appropriate names into scope.
//
// AsciiRecordReader needs additional bits not defined by OASIS to keep
// track of the info-byte field and mandatory fields while it parses
// element records.  We use the bits in 0xff00 (the second-lowest byte)
// for those.


#ifndef OASIS_INFOBYTE_H_INCLUDED
#define OASIS_INFOBYTE_H_INCLUDED

#include <cassert>
#include "misc/globals.h"


namespace Oasis {

using SoftJin::Uint;
using SoftJin::Ulong;


// Section 22:  PLACEMENT record
// `17' placement-info-byte [reference-number | cellname-string]
//      [x] [y] [repetition]
// placement-info-byte ::= CNXYRAAF
//
// `18' placement-info-byte [reference-number | cellname-string]
//      [magnification] [angle] [x] [y] [repetition]
// placement-info-byte ::= CNXYRMAF

namespace PlacementInfoByte {
    enum {
        CellExplicitBit = 0x80,
        RefnumBit       = 0x40,
        XBit            = 0x20,
        YBit            = 0x10,
        RepBit          = 0x08,
        MagBit          = 0x04,         // for record type 18
        AngleBit        = 0x02,         // for record type 18
        FlipBit         = 0x01,

        // Bitmask and values for the AA bits of record type 17
        AngleMask       = 0x06,
        AngleZero       = 0x0,
        Angle90         = 0x02,
        Angle180        = 0x04,
        Angle270        = 0x06,

        // Bits used by AsciiRecordReader
        InfoByteBit     = 0x100
    };

    // GetAngle -- get angle from info-byte for record type 18
    inline int
    GetAngle (int infoByte) {
        return (90 * ((infoByte & AngleMask) >> 1));
    }
}



// Section 24:  TEXT record
// `19' text-info-byte [reference-number | text-string]
//      [textlayer-number] [texttype-number] [x] [y] [repetition]
// text-info-byte ::= 0CNXYRTL

namespace TextInfoByte {
    enum {
        TextExplicitBit = 0x40,
        RefnumBit       = 0x20,
        XBit            = 0x10,
        YBit            = 0x08,
        RepBit          = 0x04,
        TexttypeBit     = 0x02,
        TextlayerBit    = 0x01,

        ValidInfoBits   = 0x7f,         // remaining bits must be zero

        // Bits used by AsciiRecordReader
        InfoByteBit     = 0x100
    };
}



// Section 25:  RECTANGLE record
// `20' rectangle-info-byte [layer-number] [datatype-number]
//      [width] [height] [x] [y] [repetition]
// rectangle-info-byte ::= SWHXYRDL
// S = 1 means the rectangle is a square.  H must be 0.

namespace RectangleInfoByte {
    enum {
        SquareBit       = 0x80,
        WidthBit        = 0x40,
        HeightBit       = 0x20,
        XBit            = 0x10,
        YBit            = 0x08,
        RepBit          = 0x04,
        DatatypeBit     = 0x02,
        LayerBit        = 0x01,

        // Bits used by AsciiRecordReader
        InfoByteBit     = 0x100
    };
}



// Section 26:  POLYGON Record
// `21' polygon-info-byte [layer-number] [datatype-number]
//      [point-list] [x] [y] [repetition]
// polygon-info-byte ::= 00PXYRDL

namespace PolygonInfoByte {
    enum {
        PointListBit    = 0x20,
        XBit            = 0x10,
        YBit            = 0x08,
        RepBit          = 0x04,
        DatatypeBit     = 0x02,
        LayerBit        = 0x01,

        ValidInfoBits   = 0x3f,

        // Bits used by AsciiRecordReader
        InfoByteBit     = 0x100
    };
}


// Section 27:  PATH record
// `22' path-info-byte [layer-number] [datatype-number] [half-width]
//      [ extension-scheme [start-extension] [end-extension] ]
//      [point-list] [x] [y] [repetition]
// path-info-byte   ::= EWPXYRDL
// extension-scheme ::= 0000SSEE

namespace PathInfoByte {
    enum {
        ExtensionBit    = 0x80,
        HalfwidthBit    = 0x40,
        PointListBit    = 0x20,
        XBit            = 0x10,
        YBit            = 0x08,
        RepBit          = 0x04,
        DatatypeBit     = 0x02,
        LayerBit        = 0x01,

        // Bits used by AsciiRecordReader
        InfoByteBit     = 0x100,
        StartExtnBit    = 0x200,
        EndExtnBit      = 0x400,
    };

    // Values for the 2-bit fields SS and EE in extension-scheme.

    enum {
        ReuseLastExtn   = 0,
        FlushExtn       = 1,
        HalfwidthExtn   = 2,
        ExplicitExtn    = 3
    };


    // Functions to get and set the SS and EE bits in extension-scheme

    inline bool
    ExtensionSchemeIsValid (Ulong extnScheme) {
        return ((extnScheme & ~Ulong(0xf)) == 0);
    }

    inline Uint
    GetStartScheme (Ulong extnScheme) {
        return ((extnScheme & 0x0c) >> 2);
    }

    inline Uint
    GetEndScheme (Ulong extnScheme) {
        return (extnScheme & 0x03);
    }

    inline void
    SetStartScheme (/*inout*/ Ulong* extnScheme, Uint ss) {
        assert (ss < 4);
        *extnScheme = (*extnScheme & ~0x0c) | (ss << 2);
    }

    inline void
    SetEndScheme (/*inout*/ Ulong* extnScheme, Uint ee) {
        assert (ee < 4);
        *extnScheme = (*extnScheme & ~0x03) | ee;
    }
}



// Section 28:  TRAPEZOID record
//
// `23' trap-info-byte [layer-number] [datatype-number]
//      [width] [height] delta-a delta-b [x] [y] [repetition]
//
// `24' trap-info-byte [layer-number] [datatype-number]
//      [width] [height] delta-a         [x] [y] [repetition]
//
// `25' trap-info-byte [layer-number] [datatype-number]
//      [width] [height]         delta-b [x] [y] [repetition]
//
// trap-info-byte ::= OWHXYRDL

namespace TrapezoidInfoByte {
    enum {
        VerticalBit     = 0x80, // 1 => vertical orientation; 0 => horizontal
        WidthBit        = 0x40,
        HeightBit       = 0x20,
        XBit            = 0x10,
        YBit            = 0x08,
        RepBit          = 0x04,
        DatatypeBit     = 0x02,
        LayerBit        = 0x01,

        // Bits used by AsciiRecordReader
        InfoByteBit     = 0x100,
        DeltaABit       = 0x200,
        DeltaBBit       = 0x400,
        HorizontalBit   = 0x800,
    };
}



// Section 29:  CTRAPEZOID record
// `26' ctrapezoid-info-byte [layer-number] [datatype-number]
//      [ctrapezoid-type] [width] [height] [x] [y] [repetition]
// ctrapezoid-info-byte ::= TWHXYRDL

namespace CTrapezoidInfoByte {
    enum {
        TrapTypeBit     = 0x80, // 1 => ctrapezoid-type present
        WidthBit        = 0x40,
        HeightBit       = 0x20,
        XBit            = 0x10,
        YBit            = 0x08,
        RepBit          = 0x04,
        DatatypeBit     = 0x02,
        LayerBit        = 0x01,

        // Bits used by AsciiRecordReader
        InfoByteBit     = 0x100
    };
}



// Section 30:  CIRCLE record
// `27' circle-info-byte [layer-number] [datatype-number]
//      [radius] [x] [y] [repetition]
// circle-info-byte ::= 00rXYRDL   (r is radius, R is repetition)

namespace CircleInfoByte {
    enum {
        RadiusBit       = 0x20,
        XBit            = 0x10,
        YBit            = 0x08,
        RepBit          = 0x04,
        DatatypeBit     = 0x02,
        LayerBit        = 0x01,

        ValidInfoBits   = 0x3f,

        // Bits used by AsciiRecordReader
        InfoByteBit     = 0x100
    };
}



// Section 31:  PROPERTY record
// `28' prop-info-byte [reference-number | propname-string]
//      [prop-value-count]  [ <property-value>* ]
// `29'
// prop-info-byte ::= UUUUVCNS

namespace PropertyInfoByte {
    enum {
        ValueCountMask  = 0xf0, // 0-14 => num values; 15 => use count field
        ReuseValueBit   = 0x08, // 1 => reuse last value list
        NameExplicitBit = 0x04, // 1 => name present; 0 => reuse last name
        RefnumBit       = 0x02, // 1 => propname ref, 0 => name string
        StandardBit     = 0x01, // 1 => standard property

        // Bits used by AsciiRecordReader
        InfoByteBit     = 0x100
    };

    inline int
    GetNumValues (int infoByte) {
        return ((infoByte & 0xf0) >> 4);
    }

    inline void
    SetNumValues (/*inout*/ int* infoByte, Uint numValues) {
        assert (numValues < 16);
        *infoByte = (*infoByte & ~0xf0) | (numValues << 4);
    }
}



// Section 34:  XGEOMETRY record
// `33' xgeometry-info-byte xgeometry-attribute [layer-number]
//      [datatype-number] xgeometry-string [x] [y] [repetition]
// xgeometry-info-byte ::= 000XYRDL

namespace XGeometryInfoByte {
    enum {
        XBit            = 0x10,
        YBit            = 0x08,
        RepBit          = 0x04,
        DatatypeBit     = 0x02,
        LayerBit        = 0x01,

        ValidInfoBits   = 0x1f,

        // Bits used by AsciiRecordReader
        InfoByteBit     = 0x100,
        AttributeBit    = 0x200,        // xgeometry-attribute
        GeomStringBit   = 0x400,        // xgeometry-string
    };
}


}  // namespace Oasis

#endif  // OASIS_INFOBYTE_H_INCLUDED
