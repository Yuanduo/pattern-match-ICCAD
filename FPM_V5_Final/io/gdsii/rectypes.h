// gdsii/rectypes.h -- GDSII record types and data types
//
// last modified:   22-Jan-2005  Sat  18:12
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef GDSII_RECTYPES_H_INCLUDED
#define GDSII_RECTYPES_H_INCLUDED

#include <cassert>
#include "misc/globals.h"


namespace Gdsii {

using SoftJin::Uchar;
using SoftJin::Ushort;
using SoftJin::Uint;


// Possible values for the record-type field in a GDSII Stream record.  Not
// all of these are legal.  The GDSII spec says that some record types are
// not used.

enum GdsRecordType {
    GRT_HEADER         = 0,
    GRT_BGNLIB         = 1,
    GRT_LIBNAME        = 2,
    GRT_UNITS          = 3,
    GRT_ENDLIB         = 4,
    GRT_BGNSTR         = 5,
    GRT_STRNAME        = 6,
    GRT_ENDSTR         = 7,
    GRT_BOUNDARY       = 8,
    GRT_PATH           = 9,
    GRT_SREF           = 10,

    GRT_AREF           = 11,
    GRT_TEXT           = 12,
    GRT_LAYER          = 13,
    GRT_DATATYPE       = 14,
    GRT_WIDTH          = 15,
    GRT_XY             = 16,
    GRT_ENDEL          = 17,
    GRT_SNAME          = 18,
    GRT_COLROW         = 19,
    GRT_TEXTNODE       = 20,

    GRT_NODE           = 21,
    GRT_TEXTTYPE       = 22,
    GRT_PRESENTATION   = 23,
    GRT_SPACING        = 24,
    GRT_STRING         = 25,
    GRT_STRANS         = 26,
    GRT_MAG            = 27,
    GRT_ANGLE          = 28,
    GRT_UINTEGER       = 29,
    GRT_USTRING        = 30,

    GRT_REFLIBS        = 31,
    GRT_FONTS          = 32,
    GRT_PATHTYPE       = 33,
    GRT_GENERATIONS    = 34,
    GRT_ATTRTABLE      = 35,
    GRT_STYPTABLE      = 36,
    GRT_STRTYPE        = 37,
    GRT_ELFLAGS        = 38,
    GRT_ELKEY          = 39,
    GRT_LINKTYPE       = 40,

    GRT_LINKKEYS       = 41,
    GRT_NODETYPE       = 42,
    GRT_PROPATTR       = 43,
    GRT_PROPVALUE      = 44,
    GRT_BOX            = 45,
    GRT_BOXTYPE        = 46,
    GRT_PLEX           = 47,
    GRT_BGNEXTN        = 48,
    GRT_ENDEXTN        = 49,
    GRT_TAPENUM        = 50,

    GRT_TAPECODE       = 51,
    GRT_STRCLASS       = 52,
    GRT_RESERVED       = 53,
    GRT_FORMAT         = 54,
    GRT_MASK           = 55,
    GRT_ENDMASKS       = 56,
    GRT_LIBDIRSIZE     = 57,
    GRT_SRFNAME        = 58,
    GRT_LIBSECUR       = 59,
    GRT_BORDER         = 60,

    GRT_SOFTFENCE      = 61,
    GRT_HARDFENCE      = 62,
    GRT_SOFTWIRE       = 63,
    GRT_HARDWIRE       = 64,
    GRT_PATHPORT       = 65,
    GRT_NODEPORT       = 66,
    GRT_USERCONSTRAINT = 67,
    GRT_SPACER_ERROR   = 68,
    GRT_CONTACT        = 69,

    GRT_MaxType        = 69
};



// Possible values for the data-type field in a GDSII Stream record.
// Type 4 (four-byte floating-point number) is defined by GDSII
// but not used.

enum GdsDataType {
    GDATA_NONE     = 0,
    GDATA_BITARRAY = 1,
    GDATA_SHORT    = 2,
    GDATA_INT      = 3,
//  GDATA_FLOAT    = 4,         // GDSII does not use 4-byte reals
    GDATA_DOUBLE   = 5,
    GDATA_STRING   = 6
};



// GdsRecordTypeInfo -- information about each GDSII record type.
// The .cc file defines an array of these structs called recTypeInfo.
// The information in this array is used, for instance,
//   - by the scanner, to check input records for sanity;
//   - by the GDS->ASCII converter to print the name of the record;
//   - by GdsRecord::nextString() to get the length of each string
//     in a record that contains fixed-length strings.
//
// The data members are all public because no abstraction is provided.
// It is safe enough because the array is const and so is the name field.
// The application typically uses the static get() method to get a
// pointer to the GdsRecordTypeInfo struct for a given recType.
//
// Data members
//
// recType      Record type, one of the GRT_XXX enumerators.
//              This is also the index of the struct in recTypeInfo[].
//
// valid        Whether this record type is valid.  The GDSII Stream spec
//              defines some record types but says that they are not used.
//              'valid' is false for those record types.  The scanner
//              aborts if any of them appear in the input file.  The
//              next 4 members are defined only when valid is true.
//
// datatype     Type of data contained in records of this record type,
//              one of the GDATA_XXX enumerators.
//
// itemSize     For records that contain fixed-length data, the
//              size in bytes of each datum.  This is 0 for records that
//              do not contain data (datatype is GDATA_NONE) and for
//              records that contain a single variable-length string.
//              For records that contain strings NUL-padded to some fixed
//              maximum length, this contains the fixed length.
//
// minLength    The minimum legal length of a record of this type.
// maxLength    The maximum legal length of a record of this type.
//
// name         Name of record type, used when printing error messages
//              and in the ASCII version of GDSII.
//
// The itemSizes for string records and some of the min/max lengths
// depend on constants in glimits.h.  If you update glimits.h, you might
// also have to update rectypes.cc.
//
// itemSize, minLength, and maxLength must all be even.


class GdsRecordTypeInfo {
public:
    Uchar       recType;        // one of the GRT_XXX enumerators
    Uchar       valid;          // 0 => this rec type is not used
    Uchar       datatype;       // one of the GDATA_XXX enumerators
    Uchar       itemSize;       // each data item in record has this size
    Ushort      minLength;      // min valid length of record body
    Ushort      maxLength;      // max valid length of record body
    const char* name;           // name corresponding to recType

public:
    GdsRecordType       getRecordType() const;
    GdsDataType         getDataType() const;

    static bool         recTypeIsValid (Uint recType);
    static const char*  getName (Uint recType);
    static const GdsRecordTypeInfo*  get (Uint recType);

private:
    static const GdsRecordTypeInfo   recTypeInfo[];
};



// getRecordType() and getDataType() are convenience functions that cast
// the recType and datatype fields into the appropriate enums.

inline GdsRecordType
GdsRecordTypeInfo::getRecordType() const {
    return (static_cast<GdsRecordType>(recType));
}


inline GdsDataType
GdsRecordTypeInfo::getDataType() const {
    return (static_cast<GdsDataType>(datatype));
}



// The scanner needs to check that a record type that appears in
// a GDSII file is valid.

/*static*/ inline bool
GdsRecordTypeInfo::recTypeIsValid (Uint recType) {
    return (recType <= GRT_MaxType  &&  recTypeInfo[recType].valid);
}



// getName -- get the name of a record type
// Unlike get() below, this function does not check the member 'valid'
// because the name field is defined for all recTypes.

/*static*/ inline const char*
GdsRecordTypeInfo::getName (Uint recType) {
    assert (recType <= GRT_MaxType);
    return (recTypeInfo[recType].name);
}



// get -- get the GdsRecordTypeInfo struct given the record type.
// The recType argument must be a valid record type.  Note that this
// implies that the member 'valid' must be true.  We have this
// precondition because there isn't much point in returning a pointer
// to a struct most of whose fields are undefined.

/*static*/ inline const GdsRecordTypeInfo*
GdsRecordTypeInfo::get (Uint recType) {
    assert (recTypeIsValid(recType));
    return (&recTypeInfo[recType]);
}


} // namespace Gdsii

#endif  // GDSII_RECTYPES_H_INCLUDED
