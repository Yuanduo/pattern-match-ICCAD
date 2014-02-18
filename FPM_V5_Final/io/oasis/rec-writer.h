// oasis/rec-writer.h -- create OASIS file record by record
//
// last modified:   28-Dec-2009  Mon  19:48
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// This file defines OasisRecordWriter, the output analogue of
// OasisRecordReader.

#ifndef OASIS_REC_WRITER_H_INCLUDED
#define OASIS_REC_WRITER_H_INCLUDED

#include <string>
#include <sys/types.h>          // for off_t

#include "oasis.h"
#include "records.h"
#include "writer.h"


namespace Oasis {

using std::string;
using SoftJin::llong;
using SoftJin::Ullong;
using SoftJin::Ulong;


class OasisRecordWriter {
    OasisWriter         writer;

public:
    explicit    OasisRecordWriter (const char* fname);
                ~OasisRecordWriter();

    void        beginFile (Validation::Scheme valScheme);
    void        endFile();
    void        endCblock();
    void        writeRecord (const OasisRecord* orecp);
    off_t       currFileOffset() const;

private:
    void        writeSInt (long val);
    void        writeUInt (Ulong val);
    void        writeSInt64 (llong val);
    void        writeUInt64 (Ullong val);
    void        writeReal (const Oreal& val);
    void        writeString (const string& val);
    void        writeInfoByte (int infoByte);

    void        writeStartRecord (const StartRecord* recp);
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
                OasisRecordWriter (const OasisRecordWriter&);   // forbidden
    void        operator= (const OasisRecordWriter&);           // forbidden
};



// currFileOffset -- get offset at which the next record will be written
// Precondition: the writer is not in a cblock.

inline off_t
OasisRecordWriter::currFileOffset() const {
    return writer.currFileOffset();
}


} // namespace Oasis

#endif  // OASIS_REC_WRITER_H_INCLUDED
