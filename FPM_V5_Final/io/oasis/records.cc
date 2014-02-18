// oasis/records.cc -- raw contents of OASIS records
//
// last modified:   15-Jan-2005  Sat  16:07
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include "records.h"

namespace Oasis {


OasisRecord::OasisRecord()  { }
OasisRecord::~OasisRecord() { }
StartRecord::StartRecord()  { }
StartRecord::~StartRecord() { }
EndRecord::EndRecord()  { }
EndRecord::~EndRecord() { }
NameRecord::NameRecord()  { }
NameRecord::~NameRecord() { }
LayerNameRecord::LayerNameRecord()  { }
LayerNameRecord::~LayerNameRecord() { }
CellRecord::CellRecord()  { }
CellRecord::~CellRecord() { }
PlacementRecord::PlacementRecord()  { }
PlacementRecord::~PlacementRecord() { }
TextRecord::TextRecord()  { }
TextRecord::~TextRecord() { }
RectangleRecord::RectangleRecord()  { }
RectangleRecord::~RectangleRecord() { }
PolygonRecord::PolygonRecord()  { }
PolygonRecord::~PolygonRecord() { }
PathRecord::PathRecord()  { }
PathRecord::~PathRecord() { }
TrapezoidRecord::TrapezoidRecord()  { }
TrapezoidRecord::~TrapezoidRecord() { }
CTrapezoidRecord::CTrapezoidRecord()  { }
CTrapezoidRecord::~CTrapezoidRecord() { }
CircleRecord::CircleRecord()  { }
CircleRecord::~CircleRecord() { }
PropertyRecord::PropertyRecord()  { }
PropertyRecord::~PropertyRecord() { }
XNameRecord::XNameRecord()  { }
XNameRecord::~XNameRecord() { }
XElementRecord::XElementRecord()  { }
XElementRecord::~XElementRecord() { }
XGeometryRecord::XGeometryRecord()  { }
XGeometryRecord::~XGeometryRecord() { }
CblockRecord::CblockRecord()  { }
CblockRecord::~CblockRecord() { }



//======================================================================
//                              RecordManager
//======================================================================

RecordManager::RecordManager()
{
    // Initialize all pointers to Null before allocating anything in
    // createRecords().  This ensures that all pointers are well-defined
    // in the destructor even if an exception is thrown in the middle of
    // the allocations.

    for (int j = 0;  j <= RIDX_MaxID;  ++j)
        records[j] = Null;
}



RecordManager::~RecordManager()
{
    for (int j = 0;  j <= RIDX_MaxID;  ++j)
        delete records[j];
}



// createRecords -- initialize the cache of records
// This is invoked by the constructors of OasisRecordReader and
// AsciiRecordReader.

void
RecordManager::createRecords()
{
    // If any of the invocations of the new operator throws an
    // exception, the RecordManager destructor will free all the
    // records created earlier.  That is why we have this class.

    // These three record types have no associated data.
    records[RID_PAD]        = new OasisRecord;
    records[RID_XYABSOLUTE] = new OasisRecord;
    records[RID_XYRELATIVE] = new OasisRecord;

    records[RID_START] = new StartRecord;
    records[RID_END]   = new EndRecord;

    // Four different kinds of names, each with just the name string
    // and (for the _R versions) a reference-number.
    records[RID_CELLNAME]       = new NameRecord;
    records[RID_CELLNAME_R]     = new NameRecord;
    records[RID_TEXTSTRING]     = new NameRecord;
    records[RID_TEXTSTRING_R]   = new NameRecord;
    records[RID_PROPNAME]       = new NameRecord;
    records[RID_PROPNAME_R]     = new NameRecord;
    records[RID_PROPSTRING]     = new NameRecord;
    records[RID_PROPSTRING_R]   = new NameRecord;

    records[RID_LAYERNAME_GEOMETRY] = new LayerNameRecord;
    records[RID_LAYERNAME_TEXT]     = new LayerNameRecord;

    records[RID_CELL_REF]   = new CellRecord;
    records[RID_CELL_NAMED] = new CellRecord;

    records[RID_PLACEMENT]           = new PlacementRecord;
    records[RID_PLACEMENT_TRANSFORM] = new PlacementRecord;

    records[RID_TEXT]        = new TextRecord;
    records[RID_RECTANGLE]   = new RectangleRecord;
    records[RID_POLYGON]     = new PolygonRecord;
    records[RID_PATH]        = new PathRecord;

    records[RID_TRAPEZOID]   = new TrapezoidRecord;
    records[RID_TRAPEZOID_A] = new TrapezoidRecord;
    records[RID_TRAPEZOID_B] = new TrapezoidRecord;

    records[RID_CTRAPEZOID]  = new CTrapezoidRecord;
    records[RID_CIRCLE]      = new CircleRecord;

    records[RID_PROPERTY]        = new PropertyRecord;
    records[RID_PROPERTY_REPEAT] = new PropertyRecord;

    records[RID_XNAME]     = new XNameRecord;
    records[RID_XNAME_R]   = new XNameRecord;

    records[RID_XELEMENT]  = new XElementRecord;
    records[RID_XGEOMETRY] = new XGeometryRecord;
    records[RID_CBLOCK]    = new CblockRecord;

    // Only AsciiRecordReader uses these pseudo-records.
    // OasisRecordReader does not use them because they may appear only
    // in the ASCII file.  None of them contain data.

    records[RIDX_END_CBLOCK]       = new OasisRecord;
    records[RIDX_CELLNAME_TABLE]   = new OasisRecord;
    records[RIDX_TEXTSTRING_TABLE] = new OasisRecord;
    records[RIDX_PROPNAME_TABLE]   = new OasisRecord;
    records[RIDX_PROPSTRING_TABLE] = new OasisRecord;
    records[RIDX_LAYERNAME_TABLE]  = new OasisRecord;
    records[RIDX_XNAME_TABLE]      = new OasisRecord;

    // Make sure all records are allocated
    for (int j = 0;  j <= RIDX_MaxID;  ++j)
        assert (records[j] != Null);
}


} // namespace Oasis
