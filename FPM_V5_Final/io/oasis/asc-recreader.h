// oasis/asc-recreader.h -- assemble tokens from ASCII file into OASIS records
//
// last modified:   29-Dec-2009  Tue  22:38
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef OASIS_ASC_RECREADER_H_INCLUDED
#define OASIS_ASC_RECREADER_H_INCLUDED

#include <string>
#include "asc-scanner.h"
#include "keywords.h"
#include "records.h"


namespace Oasis {

using std::string;
using SoftJin::llong;
using SoftJin::Uint;
using SoftJin::Ullong;
using SoftJin::Ulong;


// AsciiRecordReader -- assemble tokens from ASCII file into OASIS records.
// This class is to ASCII files what OasisRecordReader is to OASIS
// files.  To use it, keep calling getNextRecord() until it returns the
// END record.  You will have to cast the return value to the
// appropriate subclass of OasisRecord (see records.h).
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
// filename             string
//      Pathname of file being parsed.  Used only for error messages.
//      OasisRecordReader does not open the file itself.  Instead its
//      constructor takes a reference to an OasisScanner from which it
//      gets the tokens.

class AsciiRecordReader {
    AsciiScanner  scanner;
    RecordManager rm;
    OasisRecord*  currRecord;
    Ulong         offsetFlag;           // copied from START record
    string        filename;

public:
    explicit    AsciiRecordReader (const char* fname);
                ~AsciiRecordReader();
    OasisRecord*  getNextRecord();

private:
    // Abbreviations

    AsciiScanner::TokenType  readToken();
    AsciiScanner::TokenType  peekToken();
    void        unreadToken();
    long        readSInt();
    Ulong       readUInt();
    llong       readSInt64();
    Ullong      readUInt64();
    Uint        readInfoByte();
    void        readString (/*out*/ string* pstr);

    // START and END records

    void        readStartRecord (/*inout*/ StartRecord* recp);
    void        readEndRecord (/*inout*/ EndRecord* recp);
    void        readTableOffsets (/*out*/ TableOffsets* tables);
    void        readTableInfo (/*out*/ TableOffsets::TableInfo* tinfo);

    // Name records

    void        readCellNameRecord (/*inout*/ NameRecord* recp);
    void        readTextStringRecord (/*inout*/ NameRecord* recp);
    void        readPropNameRecord (/*inout*/ NameRecord* recp);
    void        readPropStringRecord (/*inout*/ NameRecord* recp);
    void        readLayerNameRecord (/*inout*/ LayerNameRecord* recp);
    void        readInterval (/*out*/ LayerNameRecord::RawInterval* ivalp);

    void        readCellRecord (/*inout*/ CellRecord* recp);

    // Reading fields of element records

    void        readInfoByteField (/*inout*/ int* infoByte, int infoBit,
                                   /*out*/ int* field);

    void        readUIntField (AsciiKeyword  keyword,
                               /*inout*/ int* infoByte, int bit,
                               /*out*/ Ulong* field);

    void        readSIntField (AsciiKeyword  keyword,
                               /*inout*/ int* infoByte, int bit,
                               /*out*/ long* field);

    void        readRealField (AsciiKeyword  keyword,
                               /*inout*/ int* infoByte, int bit,
                               /*out*/ Oreal* field);

    void        readStringField (AsciiKeyword  keyword,
                                 /*inout*/ int* infoByte, int bit,
                                 /*out*/ string* field);

    void        readRepetitionField (/*inout*/ int* infoByte, int bit,
                                     /*out*/ RawRepetition* field);

    void        readPointListField (/*inout*/ int* infoByte, int bit,
                                    /*out*/ PointList* field);

    void        setInfoByteFlag (AsciiKeyword keyword,
                                 /*inout*/ int* infoBytep, int bit);

    // Element records.  For most elements we have a pair of functions.
    // The first reads the whole record.  It repeatedly calls the second,
    // which reads just one field.

    void        readPlacementRecord (/*inout*/ PlacementRecord* recp);
    void        readPlacementField (AsciiKeyword keyword,
                                    /*inout*/ int* infoByte,
                                    /*inout*/ PlacementRecord* recp);

    void        readTextRecord (/*inout*/ TextRecord* recp);
    void        readTextField (AsciiKeyword keyword,
                               /*inout*/ int* infoByte,
                               /*inout*/ TextRecord* recp);

    void        readRectangleRecord (/*inout*/ RectangleRecord* recp);
    void        readRectangleField (AsciiKeyword keyword,
                                    /*inout*/ int* infoByte,
                                    /*inout*/ RectangleRecord* recp);

    void        readPolygonRecord (/*inout*/ PolygonRecord* recp);
    void        readPolygonField (AsciiKeyword keyword,
                                  /*inout*/ int* infoByte,
                                  /*inout*/ PolygonRecord* recp);

    void        readPathRecord (/*inout*/ PathRecord* recp);
    void        readPathField (AsciiKeyword keyword,
                               /*inout*/ int* infoByte,
                               /*inout*/ PathRecord* recp);
    Uint        readExtnField (AsciiKeyword keyword,
                               /*inout*/ int* infoByte, int bit,
                               /*inout*/ long* extn);

    void        readTrapezoidRecord (/*inout*/ TrapezoidRecord* recp);
    void        readTrapezoidField (AsciiKeyword keyword,
                                    /*inout*/ int* infoByte,
                                    /*inout*/ TrapezoidRecord* recp);

    void        readCTrapezoidRecord (/*inout*/ CTrapezoidRecord* recp);
    void        readCTrapezoidField (AsciiKeyword keyword,
                                     /*inout*/ int* infoByte,
                                     /*inout*/ CTrapezoidRecord* recp);

    void        readCircleRecord (/*inout*/ CircleRecord* recp);
    void        readCircleField (AsciiKeyword keyword,
                                 /*inout*/ int* infoByte,
                                 /*inout*/ CircleRecord* recp);

    void        readPropertyRecord (/*inout*/ PropertyRecord* recp);
    void        readPropertyField (AsciiKeyword keyword,
                                   /*inout*/ int* infoByte,
                                   /*inout*/ PropertyRecord* recp);
    PropValue*  readPropertyValue();

    void        readXNameRecord (/*inout*/ XNameRecord* recp);
    void        readXElementRecord (/*inout*/ XElementRecord* recp);

    void        readXGeometryRecord (/*inout*/ XGeometryRecord* recp);
    void        readXGeometryField (AsciiKeyword keyword,
                                    /*inout*/ int* infoByte,
                                    /*inout*/ XGeometryRecord* recp);

    void        readCblockRecord (/*inout*/ CblockRecord* recp);

    // Repetitions and point-lists

    void        readRepetition (/*out*/ RawRepetition* rawrep);
    void        readPointList (/*out*/ PointList* ptlist);

    // Errors

    void        abortReader (const char* fmt, ...)
                                        SJ_PRINTF_ARGS(2,3)  SJ_NORETURN;
    void        abortDupKeyword (AsciiKeyword keyword)  SJ_NORETURN;
    void        abortBadKeyword (AsciiKeyword keyword)  SJ_NORETURN;

private:
                AsciiRecordReader (const AsciiRecordReader&);   // forbidden
    void        operator= (const AsciiRecordReader&);           // forbidden
};


} // namespace Oasis

#endif  // OASIS_ASC_RECREADER_H_INCLUDED
