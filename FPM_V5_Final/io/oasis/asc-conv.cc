// oasis/asc-conv.cc -- convert between OASIS format and its ASCII version
//
// last modified:   30-Dec-2009  Wed  22:09
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <algorithm>            // for sort()
#include <string>
#include <sys/types.h>          // for off_t

#include "misc/utils.h"
#include "asc-conv.h"
#include "asc-recreader.h"
#include "asc-recwriter.h"
#include "rec-reader.h"
#include "rec-writer.h"
#include "records.h"
#include "scanner.h"


namespace Oasis {

using namespace std;
using namespace SoftJin;


//======================================================================
//                      OasisToAsciiConverter
//======================================================================


// NameTableStart -- name table's starting offset and pseudo-record-ID
// The starting offset comes from the START or END record in the input.
// OasisToAsciiConverter has an array of these -- one for each name table.

struct NameTableStart {
    Uint        recID;          // RIDX_* enumerator for name table pseudo-rec
    off_t       startOffset;    // file offset of start of name table in input
};



// OasisToAsciiConverter -- read binary OASIS files and write ASCII equivalent
//
// In addition to writing ASCII versions of the input records,
// OasisToAsciiConverter writes two kinds of 'pseudo-records' to make the
// reverse conversion (ASCII -> OASIS) possible.
//
// It writes an END_CBLOCK record when a cblock ends, i.e., when the
// previous record was in a cblock but the next record is not.  It
// writes a NAME_TABLE record when the next record begins a name table.
// There are six kinds of NAME_TABLE records: CELLNAME_TABLE,
// TEXTSTRING_TABLE, etc.  The record-IDs for these pseudo-records begin
// with 'RIDX_'.
//
// pseudoRecord         OasisRecord
//
//      Temporary used for writing the END_CBLOCK and NAME_TABLE
//      pseudo-records.  We use a class member instead of a local
//      variable to avoid having to construct and destroy it repeatedly.
//
// tableStarts          NameTableStart[6+1]
//
//      Contains the starting offset of each of the 6 kinds of name
//      tables and the RIDX_* enumerator to be used for the table's
//      pseudo-record.  Before we convert an input record, we compare
//      its offset with the offsets in this array.  If some array
//      element matches, we write the pseudo-record for that element
//      first.  The array is sorted in increasing order of offset so
//      that we need check only one entry in the table.  The +1 in the
//      array size is for the sentinel at the end, whose startOffset
//      is -1.
//
// nextTable            NameTable*
//
//      Points to the element in tableStarts whose offset should be
//      compared with each record offset.  Initialized to the first
//      element with a non-zero offset.  (A zero offset indicates the
//      absence of the name table; spec 13.6.)  Incremented whenever
//      the offset matches.


class OasisToAsciiConverter {
    OasisScanner        scanner;
    OasisRecordReader   recReader;      // read records from binary file
    AsciiRecordWriter   recWriter;      // write records in ASCII
    OasisToAsciiOptions options;        // conversion options
    string              filename;       // input filename for error messages
    OasisRecord         pseudoRecord;   // used for writing RIDX_* records

    enum { NumTableOffsets = 6 };       // 6 kinds of name tables
    NameTableStart      tableStarts[NumTableOffsets+1];     // +1 for sentinel
    NameTableStart*     nextTable;

public:
                OasisToAsciiConverter (const char*     infilename,
                                       const char*     outfilename,
                                       WarningHandler  warner,
                                       const OasisToAsciiOptions&  options);
    void        convertFile();

private:
    void        getTableOffsets();
    void        checkForStartOfNameTable (const OFilePos& pos);
    void        writePseudoRecord (Uint recID, const OFilePos& recPos);
    void        collectTableOffsets (const TableOffsets& toffsets);
    void        abortConverter (const char* msg);

private:
                OasisToAsciiConverter (const OasisToAsciiConverter&);
    void        operator= (const OasisToAsciiConverter&);
};



// OasisToAsciiConverter constructor
// This just sets up things.  Call convertFile() for the actual conversion.
//   infilename         pathname of input (OASIS) file.  Must be seekable.
//   outfilename        pathname of output (ASCII) file
//   warner             callback to display warnings.  Pass Null if you
//                      are not interested in them.
//   options            conversion options

OasisToAsciiConverter::OasisToAsciiConverter (
    const char*     infilename,
    const char*     outfilename,
    WarningHandler  warner,
    const OasisToAsciiOptions&  options
)
  : scanner(infilename, warner),
    recReader(scanner),
    recWriter(outfilename, options.foldLongLines),
    filename(infilename)
{
    this->options = options;
    scanner.verifyMagic();      // abort unless file begins with magic string

    // tableStarts and nextTable are initialized by collectTableOffsets().
    // pseudoRecord is a temporary whose value is set just before it is
    // used.
}



// CmpTableOffsets -- predicate for sorting tableStarts by startOffset

struct CmpTableOffsets {
    bool
    operator() (const NameTableStart& lhs, const NameTableStart& rhs) const {
        return (lhs.startOffset < rhs.startOffset);
    }
};



// collectTableOffsets -- initialize tableStarts and nextTable
//   toffsets   TableOffsets field from the START or END record

void
OasisToAsciiConverter::collectTableOffsets (const TableOffsets& toffsets)
{
    // Although we assign Ulongs from the file to (signed) off_t
    // members, we need not worry about overflow.  We won't be able to
    // open the file if some offset in the file exceeds the max off_t.

    tableStarts[0].recID = RIDX_CELLNAME_TABLE;
    tableStarts[0].startOffset = toffsets.cellName.offset;

    tableStarts[1].recID = RIDX_TEXTSTRING_TABLE;
    tableStarts[1].startOffset = toffsets.textString.offset;

    tableStarts[2].recID = RIDX_PROPNAME_TABLE;
    tableStarts[2].startOffset = toffsets.propName.offset;

    tableStarts[3].recID = RIDX_PROPSTRING_TABLE;
    tableStarts[3].startOffset = toffsets.propString.offset;

    tableStarts[4].recID = RIDX_LAYERNAME_TABLE;
    tableStarts[4].startOffset = toffsets.layerName.offset;

    tableStarts[5].recID = RIDX_XNAME_TABLE;
    tableStarts[5].startOffset = toffsets.xname.offset;

    // Sentinel
    tableStarts[6].startOffset = -1;

    // Sort the table entries in increasing order of startOffset,
    // excluding the sentinel from the sort.  The tables will then be
    // in the same order in the array as in the input file.

    sort(tableStarts, tableStarts + NumTableOffsets, CmpTableOffsets());

    // Set nextTable to point to the first entry whose offset is
    // non-zero, i.e., to the entry for the first name table in the
    // file.  A zero offset means that the table does not exist.  If no
    // table exists, nextTable will end up pointing to the sentinel.

    for (nextTable = tableStarts;  nextTable->startOffset == 0;  ++nextTable)
        ;
}



// checkForStartOfNameTable -- print NAME_TABLE pseudo-record at table start.
//   recPos     file position of current record.
// If some name table begins at recPos, writes the NAME_TABLE pseudo-record
// for that table.

inline void
OasisToAsciiConverter::checkForStartOfNameTable (const OFilePos& recPos)
{
    // We need not check positions within a cblock because a name table
    // cannot begin in a cblock (spec 13.9).

    if (recPos.inCblock())
        return;

    // More than one name table can have the same offset.  The parser
    // just considers one or more of the tables to be empty.  The last
    // table pseudo-record printed may not match the following name
    // record, but that is not worth worrying about.  The next call to
    // this function should check the table with the next higher offset.

    while (nextTable->startOffset == recPos.fileOffset) {
        writePseudoRecord(nextTable->recID, recPos);
        ++nextTable;
    }
}



// convertFile -- convert entire input file to ASCII

void
OasisToAsciiConverter::convertFile()
{
    // Read the offsets of the name tables from the START or END record.

    getTableOffsets();

    // Read each OASIS record and convert to ASCII.  Before writing it,
    // check for any pseudo-records to be written first.  Exit after
    // converting the END record.
    //
    // inCblock is true if we are now in a cblock.  That is, the last
    // record written was in a cblock, or was the CBLOCK record itself.
    // If we are now in a cblock and the next record to print is not in
    // a cblock, we must first print an END_CBLOCK pseudo-record.

    bool  inCblock = false;
    scanner.seekTo(StartRecordOffset);
    recWriter.beginFile(options.showOffsets);

    for (;;) {
        OasisRecord*  orecp = recReader.getNextRecord();
        // If a cblock just ended, print the END_CBLOCK pseudo-record.
        if (inCblock  &&  !orecp->recPos.inCblock()) {
            writePseudoRecord(RIDX_END_CBLOCK, orecp->recPos);
            inCblock = false;
        }

        // If this record is the first one in a name table, print a
        // NAME_TABLE pseudo-record before it.
        checkForStartOfNameTable(orecp->recPos);

        recWriter.writeRecord(orecp);
        if (orecp->recID == RID_CBLOCK)
            inCblock = true;
        else if (orecp->recID == RID_END)
            break;
    }
    recWriter.endFile();
}



// getTableOffsets -- read table-offsets from START or END record
// The scanner's position may be anywhere when this is called, and may
// be anywhere when this returns.

void
OasisToAsciiConverter::getTableOffsets()
{
    // Read the START record.  If that has the table-offsets, collect
    // them.  Otherwise read the END record too and get the
    // table-offsets from that.

    StartRecord*  srecp = recReader.getStartRecord();
    if (srecp->offsetFlag == 0)
        collectTableOffsets(srecp->offsets);
    else {
        EndRecord*  erecp = recReader.getEndRecord();
        collectTableOffsets(erecp->offsets);
    }
}



// writePseudoRecord -- write an END_CBLOCK or NAME_TABLE pseudo-record
//   recID      the RIDX_* record-ID for the pseudo-record
//   recPos     the record position to print, if positions are being printed

void
OasisToAsciiConverter::writePseudoRecord (Uint recID, const OFilePos& recPos)
{
    pseudoRecord.recID = recID;
    pseudoRecord.recPos = recPos;
    recWriter.writeRecord(&pseudoRecord);
}



void
OasisToAsciiConverter::abortConverter (const char* msg)
{
    ThrowRuntimeError("file '%s': %s", filename.c_str(), msg);
}



//======================================================================
//                      AsciiToOasisConverter
//======================================================================


class AsciiToOasisConverter {
    AsciiRecordReader   recReader;
    OasisRecordWriter   recWriter;
    AsciiToOasisOptions options;

public:
                AsciiToOasisConverter (const char*  infilename,
                                       const char*  outfilename,
                                       const AsciiToOasisOptions&  options);
    void        convertFile();

private:
                AsciiToOasisConverter (const AsciiToOasisConverter&);
    void        operator= (const AsciiToOasisConverter&);
};



AsciiToOasisConverter::AsciiToOasisConverter (
    const char*  infilename,
    const char*  outfilename,
    const AsciiToOasisOptions&  options
)
  : recReader(infilename),
    recWriter(outfilename)
{
    this->options = options;
}




void
AsciiToOasisConverter::convertFile()
{
    TableOffsets  toffsets;
    toffsets.cellName.strict   = 0;   toffsets.cellName.offset   = 0;
    toffsets.textString.strict = 0;   toffsets.textString.offset = 0;
    toffsets.propName.strict   = 0;   toffsets.propName.offset   = 0;
    toffsets.propString.strict = 0;   toffsets.propString.offset = 0;
    toffsets.layerName.strict  = 0;   toffsets.layerName.offset  = 0;
    toffsets.xname.strict      = 0;   toffsets.xname.offset      = 0;
        // The table-offsets field from the START or END record.  We
        // use only the 'strict' values from the record; we fill in the
        // offset values from the variables below.

    off_t  cellNameTableOffset   = 0;
    off_t  textStringTableOffset = 0;
    off_t  propNameTableOffset   = 0;
    off_t  propStringTableOffset = 0;
    off_t  layerNameTableOffset  = 0;
    off_t  xnameTableOffset      = 0;
        // The offsets of the name tables in the output OASIS file.
        // These are recorded when the NAME_TABLE pseudo-records are
        // read from the ASCII file.  The initial value of 0 means that
        // the name table is absent.

    // Use the validation scheme from the options when creating the
    // OASIS file.  We ignore the scheme in the ASCII file's END record
    // because getting it is too much trouble.

    recWriter.beginFile(options.valScheme);

    // Main loop.  Exits after writing the END record.  Most records are
    // converted in the ordinary way by the default case.

    for (;;) {
        OasisRecord*  orecp = recReader.getNextRecord();
        switch (orecp->recID) {
            // Save the table-offsets from the START record and remove
            // them from the record.  OasisRecordWriter does not support
            // table-offsets in START.
            //
            case RID_START: {
                StartRecord*  recp = static_cast<StartRecord*>(orecp);
                if (recp->offsetFlag == 0) {
                    recp->offsetFlag = 1;
                    toffsets = recp->offsets;
                }
                recWriter.writeRecord(orecp);
                break;
            }

            // Put the table-offsets in the END record, after updating
            // the 'offset' members with the actual offsets.

            case RID_END: {
                EndRecord*  recp = static_cast<EndRecord*>(orecp);
                if (recp->offsetFlag == 0) {
                    recp->offsetFlag = 1;
                    recp->offsets = toffsets;
                }

                // Copy the actual table offsets into toffsets.

                TableOffsets*  toff = &recp->offsets;
                toff->cellName.offset   = cellNameTableOffset;
                toff->textString.offset = textStringTableOffset;
                toff->propName.offset   = propNameTableOffset;
                toff->propString.offset = propStringTableOffset;
                toff->layerName.offset  = layerNameTableOffset;
                toff->xname.offset      = xnameTableOffset;

                recWriter.writeRecord(orecp);
                goto END_LOOP;
            }

            // The END_CBLOCK pseudo-record marks the end of a cblock.

            case RIDX_END_CBLOCK:
                recWriter.endCblock();
                break;

            // For NAME_TABLE pseudo-records, save the writer's current
            // file offset, which is where the next record will be
            // written, in the appropriate offset variable.

            case RIDX_CELLNAME_TABLE:
                cellNameTableOffset = recWriter.currFileOffset();
                break;
            case RIDX_TEXTSTRING_TABLE:
                textStringTableOffset = recWriter.currFileOffset();
                break;
            case RIDX_PROPNAME_TABLE:
                propNameTableOffset = recWriter.currFileOffset();
                break;
            case RIDX_PROPSTRING_TABLE:
                propStringTableOffset = recWriter.currFileOffset();
                break;
            case RIDX_LAYERNAME_TABLE:
                layerNameTableOffset = recWriter.currFileOffset();
                break;
            case RIDX_XNAME_TABLE:
                xnameTableOffset = recWriter.currFileOffset();
                break;

            default:
                recWriter.writeRecord(orecp);
                break;
        }
    }
  END_LOOP:

    recWriter.endFile();
}



//======================================================================
//                           External interface
//======================================================================

// The rest of the program need not see the classes.  A function
// interface will do.


void
ConvertOasisToAscii (const char*  infilename,
                     const char*  outfilename,
                     WarningHandler  warner,
                     const OasisToAsciiOptions&  options)
{
    OasisToAsciiConverter  cvt(infilename, outfilename, warner, options);
    cvt.convertFile();
}



void
ConvertAsciiToOasis (const char*  infilename,
                     const char*  outfilename,
                     WarningHandler,            // dummy arg for uniformity
                     const AsciiToOasisOptions&  options)
{
    AsciiToOasisConverter  cvt(infilename, outfilename, options);
    cvt.convertFile();
}


} // namespace Oasis
