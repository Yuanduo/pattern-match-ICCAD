// oasis/rec-reader.h -- assemble tokens into records without interpretation
//
// last modified:   30-Dec-2009  Wed  06:19
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// This is the syntactic layer for input.  It lies between the lexical
// layer (OasisScanner) and the semantic layer (OasisParser).  All it
// does is assemble the tokens into records, with as few changes and
// validity checks as possible.
//
// records.h defines the record structs that this uses.

#ifndef OASIS_REC_READER_H_INCLUDED
#define OASIS_REC_READER_H_INCLUDED

#include <string>

#include "port/compiler.h"
#include "names.h"
#include "oasis.h"
#include "records.h"
#include "rectypes.h"
#include "scanner.h"


namespace Oasis {

using std::string;
using SoftJin::llong;
using SoftJin::Ullong;
using SoftJin::Ulong;


// OasisRecordReader -- assemble OASIS records and return them one by one.
// To use this class, position the scanner at the first record and then
// keep calling getNextRecord() until it returns the END record.  You
// will have to cast the return value to the appropriate subclass of
// OasisRecord (see records.h).
//
// scanner              OasisScanner&
//      Scanner used for reading tokens.  Note that this is a reference.
//      The upper layer must construct the scanner and pass a reference
//      to this class's constructor.  We do it this way because OasisParser
//      needs to invoke scanner methods directly, e.g., to jump around
//      the file or validate it.
//
// rm                   RecordManager
//      Cache of records of each type.  To avoid creating and destroying
//      a record on each call to getNextRecord(), we create an instance
//      of each of the 35 types of OASIS records and reuse the appropriate
//      instance in each call.
//
// currRecord           OasisRecord*
//      The record currently being assembled.  Set by getNextRecord()
//      and used only by abortReader() for the record-ID and position.
//
// offsetFlag           Ulong
//      The value of offset-flag in the START record.  If 0, the START
//      record contained the table-offsets, so the END record will not
//      have them.  Set by readStartRecord() and used by
//      readEndRecord().
//
// wantValidation       bool
//      Kludge to allow us to read OASIS files that have a mangled END
//      record.  If true, readEndRecord() reads the complete record,
//      including the validation scheme and signature.  If false,
//      readEndRecord() reads only the table-offsets (if present).  The
//      default value is true.

class OasisRecordReader {
    OasisScanner& scanner;
    RecordManager rm;
    OasisRecord*  currRecord;
    Ulong         offsetFlag;           // copied from START record
    bool          wantValidation;

public:
    explicit    OasisRecordReader (OasisScanner& scan);
                ~OasisRecordReader();
    void        setValidationWanted (bool wantValidation);

    OasisRecord*  getNextRecord();
    StartRecord*  getStartRecord();
    EndRecord*    getEndRecord();

private:
    long        readSInt();
    Ulong       readUInt();
    llong       readSInt64();
    Ullong      readUInt64();
    void        readString (/*out*/ string* pstr);
    void        readStartRecord (/*inout*/ StartRecord* recp);
    void        readEndRecord (/*inout*/ EndRecord* recp);
    void        readTableOffsets (/*out*/ TableOffsets* tables);
    void        readTableInfo (/*out*/ TableOffsets::TableInfo* tinfo);
    void        readNameRecord (/*inout*/ NameRecord* recp);
    void        readNameRecordWithRefnum (/*inout*/ NameRecord* recp);
    void        readLayerNameRecord (/*inout*/ LayerNameRecord* recp);
    void        readInterval (/*out*/ LayerNameRecord::RawInterval* ivalp);
    void        readCellRecord (/*inout*/ CellRecord* recp);
    void        readPlacementRecord (/*inout*/ PlacementRecord* recp);
    void        readTextRecord (/*inout*/ TextRecord* recp);
    void        readRectangleRecord (/*inout*/ RectangleRecord* recp);
    void        readPolygonRecord (/*inout*/ PolygonRecord* recp);
    void        readPathRecord (/*inout*/ PathRecord* recp);
    void        readTrapezoidRecord (/*inout*/ TrapezoidRecord* recp);
    void        readCTrapezoidRecord (/*inout*/ CTrapezoidRecord* recp);
    void        readCircleRecord (/*inout*/ CircleRecord* recp);
    void        readPropertyRecord (/*inout*/ PropertyRecord* recp);
    PropValue*  readPropertyValue();
    void        readXNameRecord (/*inout*/ XNameRecord* recp);
    void        readXElementRecord (/*inout*/ XElementRecord* recp);
    void        readXGeometryRecord (/*inout*/ XGeometryRecord* recp);
    void        readCblockRecord (/*inout*/ CblockRecord* recp);
    void        readRepetition (/*out*/ RawRepetition* rawrep);
    void        readPointList (/*out*/ PointList* ptlist);
    void        abortReader (const char* fmt, ...)
                                        SJ_PRINTF_ARGS(2,3) SJ_NORETURN;
private:
                OasisRecordReader (const OasisRecordReader&);   // forbidden
    void        operator= (const OasisRecordReader&);           // forbidden
};



// setValidationWanted -- should readEndRecord() read validation info?
//
// See the explanation for the data member wantValidation.  OasisParser
// calls this function to pass its own wantValidation option to
// OasisRecordReader.

inline void
OasisRecordReader::setValidationWanted (bool wantValidation) {
    this->wantValidation = wantValidation;
}


}  // namespace Oasis

#endif  // OASIS_REC_READER_H_INCLUDED
