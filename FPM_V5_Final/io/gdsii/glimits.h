// gdsii/glimits.h -- limits defined in the GDSII Stream format
//
// last modified:   25-Jun-2006  Sun  10:20
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef GDSII_GLIMITS_H_INCLUDED
#define GDSII_GLIMITS_H_INCLUDED

#include "misc/globals.h"


namespace Gdsii {
    namespace GLimits {


// If you change any of the values here, you might have to make
// corresponding changes in rectypes.cc.  The max values for string
// lengths (MaxPropValueLength, MaxStructNameLength, MaxTextStringLength)
// must be even.
//
// Some of the constants here have two variants, e.g. MaxBoundaryPoints
// and MaxBoundaryPointsSpec.  According to the spec, a BOUNDARY may
// have at most 600 points, but some GDSII files have more.  So we set
// MaxBoundaryPoints to 8191 to allow the maximum that can fit into a
// 64KB record.  But when creating a GDSII file it is better to be
// conservative and limit the record to 600 points.  We define the
// symbol MaxBoundaryPointsSpec (= 600) for upper-level code that wants
// to know the recommended limit.
//
// Similarly for all the other ...Spec names.


enum {
    RecordHeaderLength = 4,     // bytes in GDSII Stream record header
    MaxRecordLength = 65534,    // max bytes in record, including header

    MaxAttrTableLength = 44,    // max length of pathname in ATTRTABLE record
    MaxFontNameLength = 44,     // max chars in font file name, FONT record

    MaxArefRows = 32767,        // max value for rows in COLROW
    MaxArefColumns = 32767,     // max value for columns in COLROW

    // These two values are unused; the parser does not check GENERATIONS.
    MinGenerations = 2,         // min value for GENERATIONS record
    MaxGenerations = 99,        // max value for GENERATIONS record

    MinPropAttr = 0,            // min value in PROPATTR
    MinPropAttrSpec = 1,        // min value in PROPATTR according to spec
    MaxPropAttr = 127,          // max value in PROPATTR
    MaxPropValueLength = 126,   // max length of string in PROPVALUE

    MinBoundaryPoints = 4,      // min points in XY for BOUNDARY
    MaxBoundaryPoints = 8191,   // max points in XY for BOUNDARY
    MaxBoundaryPointsSpec = 600,// max BOUNDARY points according to spec

    MinNodePoints = 1,          // min points in XY for NODE
    MaxNodePoints = 8191,       // max points in XY for NODE
    MaxNodePointsSpec = 50,     // max points in XY for NODE according to spec

    MinPathPoints = 2,          // min points in XY for PATH
    MaxPathPoints = 8191,       // max points in XY for PATH
    MaxPathPointsSpec = 200,    // max points in XY for PATH according to spec

    MinXYPoints = 1,            // min points in any XY
    MaxXYPoints = 8191,         // max points in any XY
    MaxXYPointsSpec = 600,      // max points in any XY according to spec

    MaxStructNameLength = 512,  // max chars in structure name (32 in spec)

    MaxTextStringLength = 512,  // max chars in STRING record

    MinReflibs = 2,             // min library names in REFLIBS records
    MaxReflibs = 17,            // max library names in REFLIBS records
    ReflibNameLength = 44,      // length of each library name in REFLIBS

    MaxAclEntries = 32,         // max triplets in LIBSECUR record

    // The spec says that values in the LAYER, DATATYPE, NODETYPE,
    // BOXTYPE, and TEXTTYPE records must be in the range 0..255, but
    // some files use higher values.  So GdsParser only checks that the
    // values are non-negative and GdsCreator asserts that the input
    // values are non-negative and at most 32767, the largest value that
    // can be stored in the 2-byte integer field used for these.

    MaxLayer        = 32767,
    MaxBoxtype      = 32767,
    MaxDatatype     = 32767,
    MaxNodetype     = 32767,
    MaxTexttype     = 32767,

    MaxLayerSpec    = 255,
    MaxBoxtypeSpec  = 255,
    MaxDatatypeSpec = 255,
    MaxNodetypeSpec = 255,
    MaxTexttypeSpec = 255,
};


    }   // namespace GLimits
}       // namespace Gdsii


#endif  // GDSII_GLIMITS_H_INCLUDED
