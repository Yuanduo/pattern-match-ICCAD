// oasis/asc-recwriter.h -- write OASIS records in ASCII
//
// last modified:   02-Jan-2010  Sat  17:10
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef OASIS_ASC_RECWRITER_H_INCLUDED
#define OASIS_ASC_RECWRITER_H_INCLUDED

#include <string>
#include "asc-writer.h"
#include "keywords.h"
#include "names.h"
#include "oasis.h"
#include "records.h"


namespace Oasis {

using std::string;
using SoftJin::llong;
using SoftJin::Uint;
using SoftJin::Ullong;
using SoftJin::Ulong;


// AsciiRecordWriter -- write OASIS records in ASCII
//
// showOffsets          bool
//
//      Whether each record's offset in the OASIS file should be printed.
//      The caller specifies this.  The record offset appears in square
//      brackets at the beginning of the line, e.g., [0000123] for normal
//      offsets or [1234:000567] for offsets in a cblock.
//
// breakAtRecord        bool[RIDX_MaxID+1]
//
//      Specifies for each record type whether a blank line should be
//      left before a record of that type is printed.  The blank line
//      makes the ASCII file easier to read.  The constructor sets this
//      to true for CELL and NAME_TABLE records, false for the others.
//
// implicitRefnums      Ulong[RID_PROPSTRING+1]
//
//      Implicitly-assigned reference numbers for the record types
//      RID_CELLNAME, RID_TEXTSTRING, RID_PROPNAME, and RID_PROPSTRING.
//      When we print one of these records we also print the value of
//      the corresponding refnum and then increment the refnum.
//      This array is indexed by the record-ID, and so only those four
//      elements are used.


class AsciiRecordWriter {
    static const size_t   RefnumArraySize = RID_PROPSTRING + 1;

    AsciiWriter writer;
    bool        showOffsets;
    bool        breakAtRecord[RIDX_MaxID+1];
    Ulong       implicitRefnums[RefnumArraySize];

public:
    explicit    AsciiRecordWriter (const char* fname, bool foldLongLines);
    void        beginFile (bool showOffsets);
    void        endFile();
    void        writeRecord (OasisRecord* orecp);

private:
    void        writeKeyword (AsciiKeyword keyword);
    void        writeUInt (Ulong val);
    void        writeUInt (AsciiKeyword keyword, Ulong val);
    void        writeSInt (long val);
    void        writeSInt (AsciiKeyword keyword, long val);
    void        writeUInt64 (Ullong val);
    void        writeSInt64 (llong val);
    void        writeReal (const Oreal& val);
    void        writeReal (AsciiKeyword keyword, const Oreal& val);
    void        writeString (const string& val);
    void        writeString (AsciiKeyword keyword, const string& val);
    void        writeInfoByte (int infoByte);

    void        writeStartRecord (StartRecord* recp);
    void        writeEndRecord (const EndRecord* recp);
    void        writeTableOffsets (const TableOffsets& offsets);
    void        writeTableInfo (const TableOffsets::TableInfo& tinfo);

    void        writeNameRecord (const NameRecord* recp);
    void        writeNameRecordWithRefnum (const NameRecord* recp);
    void        writeLayerNameRecord (const LayerNameRecord* recp);
    void        writeInterval (const LayerNameRecord::RawInterval& ival);

    void        writeCellRecord (const CellRecord* recp);
    void        writePlacementRecord (const PlacementRecord* recp);
    void        writeTextRecord (const TextRecord* recp);
    void        writeRectangleRecord (const RectangleRecord* recp);
    void        writePolygonRecord (const PolygonRecord* recp);
    void        writePathRecord (const PathRecord* recp);
    void        writeExtnField (AsciiKeyword keyword, Uint scheme, long extn);
    void        writeTrapezoidRecord (const TrapezoidRecord* recp);
    void        writeCTrapezoidRecord (const CTrapezoidRecord* recp);
    void        writeCircleRecord (const CircleRecord* recp);

    void        writePropertyRecord (const PropertyRecord* recp);
    void        writePropertyValue (const PropValue* propval);

    void        writeXNameRecord (const XNameRecord* recp);
    void        writeXElementRecord (const XElementRecord* recp);
    void        writeXGeometryRecord (const XGeometryRecord* recp);
    void        writeCblockRecord (const CblockRecord* recp);

    void        writeRepetition (const RawRepetition& rawrep);
    void        writePointList (const PointList& ptlist);

private:
                AsciiRecordWriter (const AsciiRecordWriter&);   // forbidden
    void        operator= (const AsciiRecordWriter&);           // forbidden
};


} // namespace Oasis

#endif  // OASIS_ASC_RECWRITER_H_INCLUDED
