// oasis/asc-recreader.cc -- assemble tokens from ASCII file into OASIS records
//
// last modified:   01-Jan-2010  Fri  11:28
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <memory>

#include "misc/utils.h"
#include "asc-recreader.h"
#include "infobyte.h"
#include "oasis.h"


namespace Oasis {

using namespace std;
using namespace SoftJin;


AsciiRecordReader::AsciiRecordReader (const char* fname)
  : scanner(fname),
    filename(fname)
{
    rm.createRecords();
    currRecord = Null;
}



AsciiRecordReader::~AsciiRecordReader() { }


//----------------------------------------------------------------------
// First some inline functions to shorten the code.

inline AsciiScanner::TokenType
AsciiRecordReader::readToken() {
    return scanner.readToken();
}


//  peekToken -- get type of the next token without consuming it

inline AsciiScanner::TokenType
AsciiRecordReader::peekToken()
{
    AsciiScanner::TokenType  tokType = scanner.readToken();
    scanner.unreadToken();
    return tokType;
}


inline void
AsciiRecordReader::unreadToken() {
    scanner.unreadToken();
}


inline long
AsciiRecordReader::readSInt() {
    return scanner.readSignedInteger();
}

inline Ulong
AsciiRecordReader::readUInt() {
    return scanner.readUnsignedInteger();
}

inline llong
AsciiRecordReader::readSInt64() {
    return scanner.readSignedInteger64();
}

inline Ullong
AsciiRecordReader::readUInt64() {
    return scanner.readUnsignedInteger64();
}


inline Uint
AsciiRecordReader::readInfoByte()
{
    // The infobyte value must be an unsigned-integer in which 1-bits
    // appear only in the low byte.

    Ulong  val = scanner.readUnsignedInteger();
    if (val & ~0xffUL)
        abortReader("invalid info-byte %#lx: high bits set", val);
    return val;
}


inline void
AsciiRecordReader::readString (/*out*/ string* pstr) {
    scanner.readString(pstr);
}


//----------------------------------------------------------------------


// getNextRecord -- read next record from ASCII file
// The return value points to space owned by this class.  The caller is
// allowed to modify the struct, but the values in the struct may be
// changed by the next call to getNextRecord() or any other method of
// this class.  The caller must copy the values elsewhere if it wants to
// keep them.
//
// recPos.cblockOffset is undefined in the records returned;
// recPos.fileOffset contains the record's starting line number.

OasisRecord*
AsciiRecordReader::getNextRecord()
{
    // Read the record-ID that begins the record.  Get the cached record
    // for that ID and fill in the ID field.  Use the fileOffset to hold
    // the line number.

    Ullong  lineNum;
    Uint  recID = scanner.readRecordID(&lineNum);
    OasisRecord*  orecp = rm.getRecord(recID);
    orecp->recID = recID;
    orecp->recPos.fileOffset = lineNum;

    // Set context for abortReader() to use if it is called.
    currRecord = orecp;

    // Call the record-specific method to read the rest of the record
    // into the struct.

    switch (recID) {
        case RID_PAD:
            // nothing to do
            break;

        case RID_START:
            readStartRecord(static_cast<StartRecord*>(orecp));
            break;

        case RID_END:
            readEndRecord(static_cast<EndRecord*>(orecp));
            break;

        // AsciiScanner does not return the _R variants for name records.

        case RID_CELLNAME:
            readCellNameRecord(static_cast<NameRecord*>(orecp));
            break;

        case RID_TEXTSTRING:
            readTextStringRecord(static_cast<NameRecord*>(orecp));
            break;

        case RID_PROPNAME:
            readPropNameRecord(static_cast<NameRecord*>(orecp));
            break;

        case RID_PROPSTRING:
            readPropStringRecord(static_cast<NameRecord*>(orecp));
            break;

        case RID_LAYERNAME_GEOMETRY:
        case RID_LAYERNAME_TEXT:
            readLayerNameRecord(static_cast<LayerNameRecord*>(orecp));
            break;

        // AsciiScanner does not return RID_CELL_REF
        case RID_CELL_NAMED:
            readCellRecord(static_cast<CellRecord*>(orecp));
            break;

        case RID_XYABSOLUTE:
        case RID_XYRELATIVE:
            // nothing to do
            break;

        case RID_PLACEMENT:
        case RID_PLACEMENT_TRANSFORM:
            readPlacementRecord(static_cast<PlacementRecord*>(orecp));
            break;

        case RID_TEXT:
            readTextRecord(static_cast<TextRecord*>(orecp));
            break;

        case RID_RECTANGLE:
            readRectangleRecord(static_cast<RectangleRecord*>(orecp));
            break;

        case RID_POLYGON:
            readPolygonRecord(static_cast<PolygonRecord*>(orecp));
            break;

        case RID_PATH:
            readPathRecord(static_cast<PathRecord*>(orecp));
            break;

        case RID_TRAPEZOID:
        case RID_TRAPEZOID_A:
        case RID_TRAPEZOID_B:
            readTrapezoidRecord(static_cast<TrapezoidRecord*>(orecp));
            break;

        case RID_CTRAPEZOID:
            readCTrapezoidRecord(static_cast<CTrapezoidRecord*>(orecp));
            break;

        case RID_CIRCLE:
            readCircleRecord(static_cast<CircleRecord*>(orecp));
            break;

        case RID_PROPERTY:
            readPropertyRecord(static_cast<PropertyRecord*>(orecp));
            break;

        case RID_PROPERTY_REPEAT:
            // nothing to do
            break;

        // AsciiScanner does not return RID_XNAME_R
        case RID_XNAME:
            readXNameRecord(static_cast<XNameRecord*>(orecp));
            break;

        case RID_XELEMENT:
            readXElementRecord(static_cast<XElementRecord*>(orecp));
            break;

        case RID_XGEOMETRY:
            readXGeometryRecord(static_cast<XGeometryRecord*>(orecp));
            break;

        case RID_CBLOCK:
            readCblockRecord(static_cast<CblockRecord*>(orecp));
            break;

        // Pseudo-records.  No data in any of these.

        case RIDX_END_CBLOCK:
        case RIDX_CELLNAME_TABLE:
        case RIDX_TEXTSTRING_TABLE:
        case RIDX_PROPNAME_TABLE:
        case RIDX_PROPSTRING_TABLE:
        case RIDX_LAYERNAME_TABLE:
        case RIDX_XNAME_TABLE:
            // nothing to do
            break;

        default:
            assert (false);
    }

    return orecp;
}



//----------------------------------------------------------------------
//                      Record-reading functions
//----------------------------------------------------------------------

// Everything below is private.  We have one function for each record
// type (more or less).  Each readFooRecord() reads a record of type FOO
// into the FooRecord structure whose address it is given.  Each expects
// the next input token to be whatever follows the record name, and reads
// all the tokens in the record.  At the end of the function the next
// input token is EOF or the record name of the next record.


// START and END records

void
AsciiRecordReader::readStartRecord (/*inout*/ StartRecord* recp)
{
    // Section 13:  START record
    // `1' version-string unit offset-flag [table-offsets]

    assert (recp->recID == RID_START);

    readString(&recp->version);
    scanner.readReal(&recp->unit);
    offsetFlag = recp->offsetFlag = readUInt();
    if (offsetFlag == 0)
        readTableOffsets(&recp->offsets);
}



// readEndRecord() requires readStartRecord() to have been called
// earlier because it uses the member variable offsetsInStart that
// readStartRecord() sets.

void
AsciiRecordReader::readEndRecord (/*inout*/ EndRecord* recp)
{
    // Section 14:  END record
    // `2' [table-offsets] padding-string validation-scheme
    //     [validation-signature]

    assert (recp->recID == RID_END);

    recp->offsetFlag = offsetFlag;      // copy offset-flag from START record
    if (offsetFlag != 0)
        readTableOffsets(&recp->offsets);

    recp->valScheme = scanner.readValidationScheme();
    if (recp->valScheme != Validation::None)
        recp->valSignature = readUInt();
}



void
AsciiRecordReader::readTableOffsets (/*out*/ TableOffsets* toffs)
{
    readTableInfo(&toffs->cellName);
    readTableInfo(&toffs->textString);
    readTableInfo(&toffs->propName);
    readTableInfo(&toffs->propString);
    readTableInfo(&toffs->layerName);
    readTableInfo(&toffs->xname);
}



void
AsciiRecordReader::readTableInfo (/*out*/ TableOffsets::TableInfo* tinfo)
{
    tinfo->strict = readUInt();
    tinfo->offset = readUInt64();
}



//----------------------------------------------------------------------
// Name records

// The ASCII file uses a single record name for both variants of
// CELLNAME, TEXTSTRING, PROPNAME, PROPSTRING, and XNAME records.  For
// those record names AsciiScanner returns the RecordID variant without
// the _R, e.g., RID_CELLNAME and never RID_CELLNAME_R.  The
// read-function for each of these record types checks whether the
// record contains a reference-number.  If it does, the function reads
// the refnum and changes the record's recID to the _R variant.


void
AsciiRecordReader::readCellNameRecord (/*inout*/ NameRecord* recp)
{
    assert (recp->recID == RID_CELLNAME);

    readString(&recp->name);
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->refnum = readUInt();
        recp->recID = RID_CELLNAME_R;
    }
}



void
AsciiRecordReader::readTextStringRecord (/*inout*/ NameRecord* recp)
{
    assert (recp->recID == RID_TEXTSTRING);

    readString(&recp->name);
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->refnum = readUInt();
        recp->recID = RID_TEXTSTRING_R;
    }
}



void
AsciiRecordReader::readPropNameRecord (/*inout*/ NameRecord* recp)
{
    assert (recp->recID == RID_PROPNAME);

    readString(&recp->name);
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->refnum = readUInt();
        recp->recID = RID_PROPNAME_R;
    }
}



void
AsciiRecordReader::readPropStringRecord (/*inout*/ NameRecord* recp)
{
    assert (recp->recID == RID_PROPSTRING);

    readString(&recp->name);
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->refnum = readUInt();
        recp->recID = RID_PROPSTRING_R;
    }
}



void
AsciiRecordReader::readLayerNameRecord (/*inout*/ LayerNameRecord* recp)
{
    // Section 19:  LAYERNAME record
    // `11' layername-string layer-interval datatype-interval
    // `12' layername-string textlayer-interval texttype-interval

    assert (recp->recID == RID_LAYERNAME_GEOMETRY
            ||  recp->recID == RID_LAYERNAME_TEXT);
    readString(&recp->name);
    readInterval(&recp->layers);
    readInterval(&recp->types);
}



void
AsciiRecordReader::readInterval (/*out*/ LayerNameRecord::RawInterval* ivalp)
{
    ivalp->type = readUInt();
    switch (ivalp->type) {
        case 0:
            break;
        case 1:
        case 2:
        case 3:
            ivalp->bound_a = readUInt();
            break;
        case 4:
            ivalp->bound_a = readUInt();
            ivalp->bound_b = readUInt();
            break;
        default:
            abortReader("invalid interval type %lu", ivalp->type);
    }
}



//----------------------------------------------------------------------
//                              Cell record
//----------------------------------------------------------------------

void
AsciiRecordReader::readCellRecord (/*inout*/ CellRecord* recp)
{
    // We use a single token "cell" for both kinds of CELL records.  The
    // scanner maps this to RID_CELL_NAMED.

    assert (recp->recID == RID_CELL_NAMED);

    // The CELL record contains either a reference-number (an
    // unsigned-integer) or a cellname-string.  If the former, the
    // record-ID is really RID_CELL_REF.

    switch (readToken()) {
        case AsciiScanner::TokUnsignedInteger:
            recp->refnum = scanner.getUnsignedIntegerValue();
            recp->recID = RID_CELL_REF;
            break;

        case AsciiScanner::TokString:
            scanner.getStringValue(&recp->name);
            break;

        default:
            abortReader("cellname-string or reference-number required here");
    }
}



//----------------------------------------------------------------------
//                              Reading fields
//----------------------------------------------------------------------

// In the OASIS file, the PROPERTY record and most element records
// contain several optional fields whose values typically default to the
// current value of a modal variable.  The first byte in these records
// is the info-byte, whose bits indicate which fields are present in the
// record.
//
// In the ASCII file the values of these optional fields are specified
// using keyword/value pairs.  The fields may appear in any order, but
// each field may appear at most once.  For each record we compute what
// the info-byte should be by ORing the bits for the fields that appear.
// If the ASCII file does not contain the info-byte field, we store the
// computed value in the record.
//
// Typically the infobyte field will appear in the ASCII file if it was
// generated by OasisToAsciiConverter and will not appear if it was
// manually created.
//
// Some records have mandatory fields mixed with the optional fields.
// For example, XGEOMETRY has xgeometry-attribute and xgeometry-string.
// In the ASCII file these fields appear as keyword/value pairs just
// like the optional fields, but we make sure that they are present.  To
// keep track of such fields, and to keep track of the infobyte field
// itself, we need additional bits not defined in the OASIS info-byte.
// For these fields we use additional bits from the next higher byte,
// 0xff00.  We can do this because we use an int for the info-byte value.


// For each type of field (unsigned-integer, real, etc.) we have a
// function to read a field value of that type and set the appropriate
// bit in the info-byte.  If the bit is already set (i.e., the field
// appears twice in the record), the function aborts the reader.
//
// The arguments to all these functions:
//   keyword    the keyword for the field being read.
//              Used for the error message.
//   infoBytep  in/out: pointer to the computed infobyte that keeps track
//              of which fields appeared
//   bit        bitmask of the bit(s) in *infoBytep to set for this field.
//              The function aborts if any of these bits is set.
//   field      out: pointer to the record member where the input value
//              should be stored
//
// The only field that sets more than one bit is the 'refnum' field in
// PLACEMENT, TEXT, and PROPERTY records.  This sets RefnumBit and
// one of CellExplicitBit, TextExplicitBit, NameExplicitBit.


void
AsciiRecordReader::readUIntField (AsciiKeyword  keyword,
                                  /*inout*/ int* infoBytep, int bit,
                                  /*out*/ Ulong* field)
{
    if (*infoBytep & bit)
        abortDupKeyword(keyword);
    *infoBytep |= bit;
    *field = readUInt();
}



void
AsciiRecordReader::readSIntField (AsciiKeyword  keyword,
                                  /*inout*/ int* infoBytep, int bit,
                                  /*out*/ long* field)
{
    if (*infoBytep & bit)
        abortDupKeyword(keyword);
    *infoBytep |= bit;
    *field = readSInt();
}



void
AsciiRecordReader::readRealField (AsciiKeyword  keyword,
                                  /*inout*/ int* infoBytep, int bit,
                                  /*out*/ Oreal* field)
{
    if (*infoBytep & bit)
        abortDupKeyword(keyword);
    *infoBytep |= bit;
    scanner.readReal(field);
}



void
AsciiRecordReader::readStringField (AsciiKeyword  keyword,
                                    /*inout*/ int* infoBytep, int bit,
                                    /*out*/ string* field)
{
    if (*infoBytep & bit)
        abortDupKeyword(keyword);
    *infoBytep |= bit;
    readString(field);
}



// We do not need a keyword argument for repetitions and point-lists
// because the keyword is fixed.

void
AsciiRecordReader::readRepetitionField (/*inout*/ int* infoBytep, int bit,
                                        /*out*/ RawRepetition* field)
{
    if (*infoBytep & bit)
        abortDupKeyword(KeyRepetition);
    *infoBytep |= bit;
    readRepetition(field);
}



void
AsciiRecordReader::readPointListField (/*inout*/ int* infoBytep, int bit,
                                       /*out*/ PointList* field)
{
    if (*infoBytep & bit)
        abortDupKeyword(KeyPointList);
    *infoBytep |= bit;
    readPointList(field);
}



// Some records use bits in the info-byte not to indicate the presence
// of fields but as boolean fields in themselves.  Examples are the
// F (flip) bit in the PLACEMENT record and the S (standard) bit in the
// PROPERTY record.  The ASCII file specifies the values of these bits by
// bare keywords, not followed by a value.  The bit is 1 if the keyword
// is present and 0 if not.  setInfoByteFlag() sets the specified bit
// in the infoByte after first checking that it is not already set.


void
AsciiRecordReader::setInfoByteFlag (AsciiKeyword keyword,
                                    /*inout*/ int* infoBytep, int bit)
{
    if (*infoBytep & bit)
        abortDupKeyword(keyword);
    *infoBytep |= bit;
}


//----------------------------------------------------------------------
// Element records


void
AsciiRecordReader::readPlacementRecord (/*inout*/ PlacementRecord* recp)
{
    // Section 22:  PLACEMENT record
    // `17' placement-info-byte [reference-number | cellname-string]
    //      [x] [y] [repetition]
    // placement-info-byte ::= CNXYRAAF
    //
    // `18' placement-info-byte [reference-number | cellname-string]
    //      [magnification] [angle] [x] [y] [repetition]
    // placement-info-byte ::= CNXYRMAF

    assert (recp->recID == RID_PLACEMENT
            ||  recp->recID == RID_PLACEMENT_TRANSFORM);
    using namespace PlacementInfoByte;

    // The info-byte is optional in the ASCII file.  If it appears, it
    // must come immediately after the record name.  We set InfoByteBit
    // in infoByte to remember that it has appeared.

    int  infoByte = 0;
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->infoByte = readInfoByte();
        infoByte = InfoByteBit;
    }

    // The rest of the record consists of keyword/value pairs.  The
    // record ends when the next token is not a keyword.

    while (readToken() == AsciiScanner::TokKeyword)
        readPlacementField(scanner.getKeywordValue(), &infoByte, recp);
    unreadToken();

    // RID_PLACEMENT uses two AA bits in info-byte for angle.  If the
    // angle field appeared in the input, use its value to set those bits.

    if (recp->recID == RID_PLACEMENT  &&  (infoByte & AngleBit)) {
        int  angleBits;
        double  angle = recp->angle.getValue();
        if (angle == 0.0)         angleBits = AngleZero;
        else if (angle == 90.0)   angleBits = Angle90;
        else if (angle == 180.0)  angleBits = Angle180;
        else if (angle == 270.0)  angleBits = Angle270;
        else {
            abortReader("angle for %s record must be 0, 90, 180, or 270",
                        GetRecordAscName(RID_PLACEMENT));
            angleBits = 0;      // make gcc happy
        }
        infoByte = (infoByte & ~AngleMask) | angleBits;
    }

    // If the info-byte did not appear in the record, store the computed
    // value, after masking out the high-order bits like InfoByteBit.

    if ((infoByte & InfoByteBit) == 0)
        recp->infoByte = (infoByte & 0xff);
}



void
AsciiRecordReader::readPlacementField (AsciiKeyword keyword,
                                       /*inout*/ int* infoBytep,
                                       /*inout*/ PlacementRecord* recp)
{
    using namespace PlacementInfoByte;

    switch (keyword) {
        case KeyName:
            if (*infoBytep & RefnumBit) {
                abortReader("'%s' and '%s' are mutually exclusive",
                            GetKeywordName(KeyName),
                            GetKeywordName(KeyRefnum) );
            }
            readStringField(keyword, infoBytep, CellExplicitBit, &recp->name);
            break;

        case KeyRefnum:
            if ((*infoBytep & (CellExplicitBit|RefnumBit)) == CellExplicitBit) {
                abortReader("'%s' and '%s' are mutually exclusive",
                            GetKeywordName(KeyName),
                            GetKeywordName(KeyRefnum) );
            }
            readUIntField(keyword, infoBytep, CellExplicitBit|RefnumBit,
                          &recp->refnum);
            break;

        case KeyMag:
            if (recp->recID == RID_PLACEMENT) {
                abortReader("%s may appear only in %s records",
                            GetKeywordName(KeyMag),
                            GetRecordAscName(RID_PLACEMENT_TRANSFORM) );
            }
            readRealField(keyword, infoBytep, MagBit, &recp->mag);
            break;

        case KeyAngle:
            readRealField(keyword, infoBytep, AngleBit, &recp->angle);
            break;

        case KeyFlip:
            setInfoByteFlag(keyword, infoBytep, FlipBit);
            break;

        case KeyX:
            readSIntField(keyword, infoBytep, XBit, &recp->x);
            break;

        case KeyY:
            readSIntField(keyword, infoBytep, YBit, &recp->y);
            break;

        case KeyRepetition:
            readRepetitionField(infoBytep, RepBit, &recp->rawrep);
            break;

        default:
            abortBadKeyword(keyword);
    }
}



void
AsciiRecordReader::readTextRecord (/*inout*/ TextRecord* recp)
{
    // Section 24:  TEXT record
    // `19' text-info-byte [reference-number | text-string]
    //      [textlayer-number] [texttype-number] [x] [y] [repetition]
    // text-info-byte ::= 0CNXYRTL

    assert (recp->recID == RID_TEXT);
    using namespace TextInfoByte;

    int  infoByte = 0;
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->infoByte = readInfoByte();
        infoByte = InfoByteBit;
    }
    while (readToken() == AsciiScanner::TokKeyword)
        readTextField(scanner.getKeywordValue(), &infoByte, recp);
    unreadToken();

    if ((infoByte & InfoByteBit) == 0)
        recp->infoByte = (infoByte & 0xff);
}



void
AsciiRecordReader::readTextField (AsciiKeyword keyword,
                                  /*inout*/ int* infoBytep,
                                  /*inout*/ TextRecord* recp)
{
    using namespace TextInfoByte;

    switch (keyword) {
        case KeyString:
            if (*infoBytep & RefnumBit) {
                abortReader("'%s' and '%s' are mutually exclusive",
                            GetKeywordName(KeyString),
                            GetKeywordName(KeyRefnum) );
            }
            readStringField(keyword, infoBytep, TextExplicitBit, &recp->text);
            break;

        case KeyRefnum:
            if ((*infoBytep & (TextExplicitBit|RefnumBit)) == TextExplicitBit) {
                abortReader("'%s' and '%s' are mutually exclusive",
                            GetKeywordName(KeyString),
                            GetKeywordName(KeyRefnum) );
            }
            readUIntField(keyword, infoBytep, TextExplicitBit|RefnumBit,
                          &recp->refnum);
            break;

        case KeyTextlayer:
            readUIntField(keyword, infoBytep, TextlayerBit, &recp->textlayer);
            break;

        case KeyTexttype:
            readUIntField(keyword, infoBytep, TexttypeBit, &recp->texttype);
            break;

        case KeyX:
            readSIntField(keyword, infoBytep, XBit, &recp->x);
            break;

        case KeyY:
            readSIntField(keyword, infoBytep, YBit, &recp->y);
            break;

        case KeyRepetition:
            readRepetitionField(infoBytep, RepBit, &recp->rawrep);
            break;

        default:
            abortBadKeyword(keyword);
    }
}



void
AsciiRecordReader::readRectangleRecord (/*inout*/ RectangleRecord* recp)
{
    // Section 25:  RECTANGLE record
    // `20' rectangle-info-byte [layer-number] [datatype-number]
    //      [width] [height] [x] [y] [repetition]
    // rectangle-info-byte ::= SWHXYRDL
    // If S = 1, the rectangle is a square, and H must be 0.

    assert (recp->recID == RID_RECTANGLE);
    using namespace RectangleInfoByte;

    int  infoByte = 0;
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->infoByte = readInfoByte();
        infoByte = InfoByteBit;
    }
    while (readToken() == AsciiScanner::TokKeyword)
        readRectangleField(scanner.getKeywordValue(), &infoByte, recp);
    unreadToken();

    if ((infoByte & InfoByteBit) == 0)
        recp->infoByte = (infoByte & 0xff);
}



void
AsciiRecordReader::readRectangleField (AsciiKeyword keyword,
                                       /*inout*/ int* infoBytep,
                                       /*inout*/ RectangleRecord* recp)
{
    using namespace RectangleInfoByte;

    switch (keyword) {
        case KeyLayer:
            readUIntField(keyword, infoBytep, LayerBit, &recp->layer);
            break;
        case KeyDatatype:
            readUIntField(keyword, infoBytep, DatatypeBit, &recp->datatype);
            break;
        case KeySquare:
            setInfoByteFlag(keyword, infoBytep, SquareBit);
            break;
        case KeyWidth:
            readUIntField(keyword, infoBytep, WidthBit, &recp->width);
            break;
        case KeyHeight:
            readUIntField(keyword, infoBytep, HeightBit, &recp->height);
            break;
        case KeyX:
            readSIntField(keyword, infoBytep, XBit, &recp->x);
            break;
        case KeyY:
            readSIntField(keyword, infoBytep, YBit, &recp->y);
            break;
        case KeyRepetition:
            readRepetitionField(infoBytep, RepBit, &recp->rawrep);
            break;
        default:
            abortBadKeyword(keyword);
    }
}



void
AsciiRecordReader::readPolygonRecord (/*inout*/ PolygonRecord* recp)
{
    // Section 26:  POLYGON Record
    // `21' polygon-info-byte [layer-number] [datatype-number]
    //      [point-list] [x] [y] [repetition]
    // polygon-info-byte ::= 00PXYRDL

    assert (recp->recID == RID_POLYGON);
    using namespace PolygonInfoByte;

    int  infoByte = 0;
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->infoByte = readInfoByte();
        infoByte = InfoByteBit;
    }
    while (readToken() == AsciiScanner::TokKeyword)
        readPolygonField(scanner.getKeywordValue(), &infoByte, recp);
    unreadToken();

    if ((infoByte & InfoByteBit) == 0)
        recp->infoByte = (infoByte & 0xff);
}



void
AsciiRecordReader::readPolygonField (AsciiKeyword keyword,
                                     /*inout*/ int* infoBytep,
                                     /*inout*/ PolygonRecord* recp)
{
    using namespace PolygonInfoByte;

    switch (keyword) {
        case KeyLayer:
            readUIntField(keyword, infoBytep, LayerBit, &recp->layer);
            break;
        case KeyDatatype:
            readUIntField(keyword, infoBytep, DatatypeBit, &recp->datatype);
            break;
        case KeyPointList:
            readPointListField(infoBytep, PointListBit, &recp->ptlist);
            break;
        case KeyX:
            readSIntField(keyword, infoBytep, XBit, &recp->x);
            break;
        case KeyY:
            readSIntField(keyword, infoBytep, YBit, &recp->y);
            break;
        case KeyRepetition:
            readRepetitionField(infoBytep, RepBit, &recp->rawrep);
            break;
        default:
            abortBadKeyword(keyword);
    }
}



void
AsciiRecordReader::readPathRecord (/*inout*/ PathRecord* recp)
{
    // Section 27:  PATH record
    // `22' path-info-byte [layer-number] [datatype-number] [half-width]
    //      [ extension-scheme [start-extension] [end-extension] ]
    //      [point-list] [x] [y] [repetition]
    // path-info-byte   ::= EWPXYRDL
    // extension-scheme ::= 0000SSEE

    assert (recp->recID == RID_PATH);
    using namespace PathInfoByte;

    // The extension-scheme field (recp->extnScheme) does not appear in
    // the ASCII file.  We compute its value from start-extension and
    // end-extension.

    int  infoByte = 0;
    recp->extnScheme = 0;
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->infoByte = readInfoByte();
        infoByte = InfoByteBit;
    }
    while (readToken() == AsciiScanner::TokKeyword)
        readPathField(scanner.getKeywordValue(), &infoByte, recp);
    unreadToken();

    if (recp->extnScheme != 0)
        infoByte |= ExtensionBit;
    if ((infoByte & InfoByteBit) == 0)
        recp->infoByte = (infoByte & 0xff);
}



void
AsciiRecordReader::readPathField (AsciiKeyword keyword,
                                  /*inout*/ int* infoBytep,
                                  /*inout*/ PathRecord* recp)
{
    using namespace PathInfoByte;

    switch (keyword) {
        case KeyLayer:
            readUIntField(keyword, infoBytep, LayerBit, &recp->layer);
            break;
        case KeyDatatype:
            readUIntField(keyword, infoBytep, DatatypeBit, &recp->datatype);
            break;
        case KeyHalfwidth:
            readUIntField(keyword, infoBytep, HalfwidthBit, &recp->halfwidth);
            break;

        case KeyStartExtn: {
            Uint  ss = readExtnField(keyword, infoBytep, StartExtnBit,
                                     &recp->startExtn);
            SetStartScheme(&recp->extnScheme, ss);
            break;
        }
        case KeyEndExtn: {
            Uint  ee = readExtnField(keyword, infoBytep, EndExtnBit,
                                     &recp->endExtn);
            SetEndScheme(&recp->extnScheme, ee);
            break;
        }
        case KeyPointList:
            readPointListField(infoBytep, PointListBit, &recp->ptlist);
            break;
        case KeyX:
            readSIntField(keyword, infoBytep, XBit, &recp->x);
            break;
        case KeyY:
            readSIntField(keyword, infoBytep, YBit, &recp->y);
            break;
        case KeyRepetition:
            readRepetitionField(infoBytep, RepBit, &recp->rawrep);
            break;
        default:
            abortBadKeyword(keyword);
    }
}



// readExtnField -- read start-extension or end-extension in PATH record
//   keyword    KeyStartExtn or KeyEndExtn
//   infoBytep  in/out: pointer to computed infoByte for this record
//   bit        bit to set in infoByte: StartExtnBit for KeyStartExtn,
//              and EndExtnBit for KeyEndExtn.  Note that these bits are
//              not defined by OASIS.
//   extn       out: If the return value is ExplicitExtn, the extension
//              read from the input is stored here.  Otherwise its value
//              is not defined.
//
// Returns the value to put in the ss/ee bits of the extension scheme:
// FlushExtn, HalfwidthExtn, or ExplicitExtn.

Uint
AsciiRecordReader::readExtnField (AsciiKeyword keyword,
                                  /*inout*/ int* infoBytep, int bit,
                                  /*inout*/ long* extn)
{
    using namespace PathInfoByte;

    if (*infoBytep & bit)
        abortDupKeyword(keyword);
    *infoBytep |= bit;

    // For the value of the extension we accept both a signed-integer
    // and the keyword 'halfwidth'.  A zero integer is taken to be a
    // flush extension.  We do not provide a way to create an explicit
    // extension of 0.

    switch (readToken()) {
        case AsciiScanner::TokKeyword:
            if (scanner.getKeywordValue() == KeyHalfwidth)
                return HalfwidthExtn;
            break;

        case AsciiScanner::TokUnsignedInteger:
        case AsciiScanner::TokSignedInteger:
            *extn = scanner.getSignedIntegerValue();
            return (*extn == 0 ? FlushExtn : ExplicitExtn);

        default:        // make gcc happy
            ;
    }

    abortReader("integer or '%s' expected after %s",
                GetKeywordName(KeyHalfwidth), GetKeywordName(keyword) );
    return 0;   // make gcc happy
}



void
AsciiRecordReader::readTrapezoidRecord (/*inout*/ TrapezoidRecord* recp)
{
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

    assert (recp->recID == RID_TRAPEZOID
            ||  recp->recID == RID_TRAPEZOID_A
            ||  recp->recID == RID_TRAPEZOID_B);
    using namespace TrapezoidInfoByte;

    int  infoByte = 0;
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->infoByte = readInfoByte();
        infoByte = InfoByteBit;
    }
    while (readToken() == AsciiScanner::TokKeyword)
        readTrapezoidField(scanner.getKeywordValue(), &infoByte, recp);
    unreadToken();

    if ((infoByte & InfoByteBit) == 0)
        recp->infoByte = (infoByte & 0xff);

    // For RID_TRAPEZOID, both delta-a and delta-b must be present.
    // For RID_TRAPEZOID_A, only delta-a must be present.
    // For RID_TRAPEZOID_B, only delta-b must be present.

    if (recp->recID == RID_TRAPEZOID  ||  recp->recID == RID_TRAPEZOID_A) {
        if ((infoByte & DeltaABit) == 0)
            abortReader("field '%s' is required", GetKeywordName(KeyDelta_A));
    }
    if (recp->recID == RID_TRAPEZOID  ||  recp->recID == RID_TRAPEZOID_B) {
        if ((infoByte & DeltaBBit) == 0)
            abortReader("field '%s' is required", GetKeywordName(KeyDelta_B));
    }
}



void
AsciiRecordReader::readTrapezoidField (AsciiKeyword keyword,
                                       /*inout*/ int* infoBytep,
                                       /*inout*/ TrapezoidRecord* recp)
{
    using namespace TrapezoidInfoByte;

    switch (keyword) {
        case KeyLayer:
            readUIntField(keyword, infoBytep, LayerBit, &recp->layer);
            break;
        case KeyDatatype:
            readUIntField(keyword, infoBytep, DatatypeBit, &recp->datatype);
            break;
        case KeyWidth:
            readUIntField(keyword, infoBytep, WidthBit, &recp->width);
            break;
        case KeyHeight:
            readUIntField(keyword, infoBytep, HeightBit, &recp->height);
            break;

        // KeyHorizontal and KeyVertical are flags, with no associated value.
        // Note that VerticalBit is the important bit, defined by OASIS.
        // The trapezoid's orientation is horizontal if this bit is 0.
        // HorizontalBit is one of our extension bits, used only to verify
        // that KeyHorizontal appears at most once in the record.

        case KeyHorizontal:
            setInfoByteFlag(keyword, infoBytep, HorizontalBit);
            break;
        case KeyVertical:
            setInfoByteFlag(keyword, infoBytep, VerticalBit);
            break;

        case KeyDelta_A:
            if (recp->recID == RID_TRAPEZOID_B)
                abortBadKeyword(keyword);
            readSIntField(keyword, infoBytep, DeltaABit, &recp->delta_a);
            break;

        case KeyDelta_B:
            if (recp->recID == RID_TRAPEZOID_A)
                abortBadKeyword(keyword);
            readSIntField(keyword, infoBytep, DeltaBBit, &recp->delta_b);
            break;

        case KeyX:
            readSIntField(keyword, infoBytep, XBit, &recp->x);
            break;
        case KeyY:
            readSIntField(keyword, infoBytep, YBit, &recp->y);
            break;
        case KeyRepetition:
            readRepetitionField(infoBytep, RepBit, &recp->rawrep);
            break;
        default:
            abortBadKeyword(keyword);
    }
}



void
AsciiRecordReader::readCTrapezoidRecord (/*inout*/ CTrapezoidRecord* recp)
{
    // Section 29:  CTRAPEZOID record
    // `26' ctrapezoid-info-byte [layer-number] [datatype-number]
    //      [ctrapezoid-type] [width] [height] [x] [y] [repetition]
    // ctrapezoid-info-byte ::= TWHXYRDL

    assert (recp->recID == RID_CTRAPEZOID);
    using namespace CTrapezoidInfoByte;

    int  infoByte = 0;
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->infoByte = readInfoByte();
        infoByte = InfoByteBit;
    }
    while (readToken() == AsciiScanner::TokKeyword)
        readCTrapezoidField(scanner.getKeywordValue(), &infoByte, recp);
    unreadToken();

    if ((infoByte & InfoByteBit) == 0)
        recp->infoByte = (infoByte & 0xff);
}



void
AsciiRecordReader::readCTrapezoidField (AsciiKeyword keyword,
                                        /*inout*/ int* infoBytep,
                                        /*inout*/ CTrapezoidRecord* recp)
{
    using namespace CTrapezoidInfoByte;

    switch (keyword) {
        case KeyLayer:
            readUIntField(keyword, infoBytep, LayerBit, &recp->layer);
            break;
        case KeyDatatype:
            readUIntField(keyword, infoBytep, DatatypeBit, &recp->datatype);
            break;
        case KeyTrapType:
            readUIntField(keyword, infoBytep, TrapTypeBit, &recp->ctrapType);
            break;
        case KeyWidth:
            readUIntField(keyword, infoBytep, WidthBit, &recp->width);
            break;
        case KeyHeight:
            readUIntField(keyword, infoBytep, HeightBit, &recp->height);
            break;
        case KeyX:
            readSIntField(keyword, infoBytep, XBit, &recp->x);
            break;
        case KeyY:
            readSIntField(keyword, infoBytep, YBit, &recp->y);
            break;
        case KeyRepetition:
            readRepetitionField(infoBytep, RepBit, &recp->rawrep);
            break;
        default:
            abortBadKeyword(keyword);
    }
}



void
AsciiRecordReader::readCircleRecord (/*inout*/ CircleRecord* recp)
{
    // Section 30:  CIRCLE record
    // `27' circle-info-byte [layer-number] [datatype-number]
    //      [radius] [x] [y] [repetition]
    // circle-info-byte ::= 00rXYRDL   (r is radius, R is repetition)

    assert (recp->recID == RID_CIRCLE);
    using namespace CircleInfoByte;

    int  infoByte = 0;
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->infoByte = readInfoByte();
        infoByte = InfoByteBit;
    }
    while (readToken() == AsciiScanner::TokKeyword)
        readCircleField(scanner.getKeywordValue(), &infoByte, recp);
    unreadToken();

    if ((infoByte & InfoByteBit) == 0)
        recp->infoByte = (infoByte & 0xff);
}



void
AsciiRecordReader::readCircleField (AsciiKeyword keyword,
                                    /*inout*/ int* infoBytep,
                                    /*inout*/ CircleRecord* recp)
{
    using namespace CircleInfoByte;

    switch (keyword) {
        case KeyLayer:
            readUIntField(keyword, infoBytep, LayerBit, &recp->layer);
            break;
        case KeyDatatype:
            readUIntField(keyword, infoBytep, DatatypeBit, &recp->datatype);
            break;
        case KeyRadius:
            readUIntField(keyword, infoBytep, RadiusBit, &recp->radius);
            break;
        case KeyX:
            readSIntField(keyword, infoBytep, XBit, &recp->x);
            break;
        case KeyY:
            readSIntField(keyword, infoBytep, YBit, &recp->y);
            break;
        case KeyRepetition:
            readRepetitionField(infoBytep, RepBit, &recp->rawrep);
            break;
        default:
            abortBadKeyword(keyword);
    }
}



//----------------------------------------------------------------------
// Properties


void
AsciiRecordReader::readPropertyRecord (/*inout*/ PropertyRecord* recp)
{
    // Section 31:  PROPERTY record
    // `28' prop-info-byte [reference-number | propname-string]
    //      [prop-value-count]  [ <property-value>* ]
    // `29'
    // prop-info-byte ::= UUUUVCNS
    //
    // This function is called only for type 28.

    assert (recp->recID == RID_PROPERTY);
    using namespace PropertyInfoByte;

    // Delete PropertyValues created in the last call to this function.
    DeleteContainerElements(recp->values);

    int  infoByte = ReuseValueBit;      // bit==0 means values are present
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->infoByte = readInfoByte();
        infoByte |= InfoByteBit;
    }
    while (readToken() == AsciiScanner::TokKeyword)
        readPropertyField(scanner.getKeywordValue(), &infoByte, recp);
    unreadToken();

    SetNumValues(&infoByte, min(recp->values.size(), size_t(15)));
    if ((infoByte & InfoByteBit) == 0)
        recp->infoByte = (infoByte & 0xff);
}



void
AsciiRecordReader::readPropertyField (AsciiKeyword keyword,
                                      /*inout*/ int* infoBytep,
                                      /*inout*/ PropertyRecord* recp)
{
    using namespace PropertyInfoByte;

    switch (keyword) {
        case KeyStandard:
            setInfoByteFlag(keyword, infoBytep, StandardBit);
            break;

        case KeyName:
            if (*infoBytep & RefnumBit) {
                abortReader("'%s' and '%s' are mutually exclusive",
                            GetKeywordName(KeyName),
                            GetKeywordName(KeyRefnum) );
            }
            readStringField(keyword, infoBytep, NameExplicitBit, &recp->name);
            break;

        case KeyRefnum:
            if ((*infoBytep & (NameExplicitBit|RefnumBit)) == NameExplicitBit) {
                abortReader("'%s' and '%s' are mutually exclusive",
                            GetKeywordName(KeyName),
                            GetKeywordName(KeyRefnum) );
            }
            readUIntField(keyword, infoBytep, NameExplicitBit|RefnumBit,
                          &recp->refnum);
            break;

        case KeyValues: {
            // ReuseValueBit has the opposite sense to the other bits.
            // A 1 means that the value is missing; the value list in
            // the previous PROPERTY record should be reused.  So we
            // initialize the bit to 1 in readPropertyRecord() and set
            // it to 0 here.
            //
            if ((*infoBytep & ReuseValueBit) == 0)
                abortDupKeyword(KeyValues);
            *infoBytep &= ~ReuseValueBit;

            // Read the count of values and then each value.  The count
            // is always written in the ASCII file, even when there are
            // fewer than 15 values.

            Ulong  numValues = readUInt();
            for (Ulong j = 0;  j < numValues;  ++j) {
                auto_ptr<PropValue>  propval(readPropertyValue());
                recp->values.push_back(propval.get());
                propval.release();
            }
            break;
        }
        default:
            abortBadKeyword(keyword);
    }
}



PropValue*
AsciiRecordReader::readPropertyValue()
{
    // Each property value begins with an unsigned-integer that gives
    // the type of the value.  For real values we don't bother checking
    // that the type matches that of the real.

    Ulong type = readUInt();
    if (! PropValue::typeIsValid(type))
        abortReader("invalid type %lu for property-value", type);
    PropValueType  valType = static_cast<PropValueType>(type);

    PropValue*  propval;
    switch (valType) {
        case PV_Real_PosInteger:
        case PV_Real_NegInteger:
        case PV_Real_PosReciprocal:
        case PV_Real_NegReciprocal:
        case PV_Real_PosRatio:
        case PV_Real_NegRatio:
        case PV_Real_Float32:
        case PV_Real_Float64: {
            Oreal  rval;
            scanner.readReal(&rval);
            propval = new PropValue(valType, rval);
            break;
        }
        case PV_UnsignedInteger:
            propval = new PropValue(valType, readUInt64());
            break;

        case PV_SignedInteger:
            propval = new PropValue(valType, readSInt64());
            break;

        case PV_AsciiString:
        case PV_BinaryString:
        case PV_NameString: {
            string  s;
            readString(&s);
            propval = new PropValue(valType, s);
            break;
        }

        // If the value is a propstring-reference-number, store the
        // refnum itself in the PropValue.  This layer is not concerned
        // with name objects.

        case PV_Ref_AsciiString:
        case PV_Ref_BinaryString:
        case PV_Ref_NameString:
            propval = new PropValue(valType, static_cast<Ullong>(readUInt()));
            break;

        default:
            assert (false);
            propval = Null;     // make gcc happy
    }
    return propval;
}


//----------------------------------------------------------------------
// Extension records

void
AsciiRecordReader::readXNameRecord (/*inout*/ XNameRecord* recp)
{
    // Section 32:  XNAME record
    // `30' xname-attribute xname-string
    // `31' xname-attribute xname-string reference-number

    assert (recp->recID == RID_XNAME);

    recp->attribute = readUInt();
    readString(&recp->name);
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->refnum = readUInt();
        recp->recID = RID_XNAME_R;
    }
}



void
AsciiRecordReader::readXElementRecord (/*inout*/ XElementRecord* recp)
{
    // Section 33:  XELEMENT record
    // `32' xelement-attribute xelement-string

    assert (recp->recID == RID_XELEMENT);

    recp->attribute = readUInt();
    readString(&recp->data);
}



void
AsciiRecordReader::readXGeometryRecord (/*inout*/ XGeometryRecord* recp)
{
    // Section 34:  XGEOMETRY record
    // `33' xgeometry-info-byte xgeometry-attribute [layer-number]
    //      [datatype-number] xgeometry-string [x] [y] [repetition]
    // xgeometry-info-byte ::= 000XYRDL

    assert (recp->recID == RID_XGEOMETRY);
    using namespace XGeometryInfoByte;

    int  infoByte = 0;
    if (peekToken() == AsciiScanner::TokUnsignedInteger) {
        recp->infoByte = readInfoByte();
        infoByte = InfoByteBit;
    }
    while (readToken() == AsciiScanner::TokKeyword)
        readXGeometryField(scanner.getKeywordValue(), &infoByte, recp);
    unreadToken();

    if ((infoByte & InfoByteBit) == 0)
        recp->infoByte = (infoByte & 0xff);

    if ((infoByte & AttributeBit) == 0)
        abortReader("field '%s' is required", GetKeywordName(KeyAttribute));
    if ((infoByte & GeomStringBit) == 0)
        abortReader("field '%s' is required", GetKeywordName(KeyString));
}



void
AsciiRecordReader::readXGeometryField (AsciiKeyword keyword,
                                       /*inout*/ int* infoBytep,
                                       /*inout*/ XGeometryRecord* recp)
{
    using namespace XGeometryInfoByte;

    switch (keyword) {
        case KeyAttribute:
            readUIntField(keyword, infoBytep, AttributeBit, &recp->attribute);
            break;
        case KeyLayer:
            readUIntField(keyword, infoBytep, LayerBit, &recp->layer);
            break;
        case KeyDatatype:
            readUIntField(keyword, infoBytep, DatatypeBit, &recp->datatype);
            break;
        case KeyString:
            readStringField(keyword, infoBytep, GeomStringBit, &recp->data);
            break;
        case KeyX:
            readSIntField(keyword, infoBytep, XBit, &recp->x);
            break;
        case KeyY:
            readSIntField(keyword, infoBytep, YBit, &recp->y);
            break;
        case KeyRepetition:
            readRepetitionField(infoBytep, RepBit, &recp->rawrep);
            break;
        default:
            abortBadKeyword(keyword);
    }
}



void
AsciiRecordReader::readCblockRecord (/*inout*/ CblockRecord* recp)
{
    // Section 35:  CBLOCK record
    // `34' comp-type uncomp-byte-count comp-byte-count comp-bytes
    //
    // The two byte-count fields are ignored by AsciiToOasisConverter.

    assert (recp->recID == RID_CBLOCK);

    recp->compType    = readUInt();
    recp->uncompBytes = readUInt();
    recp->compBytes   = readUInt();

    // Check the compression type for validity because OasisWriter and
    // OasisRecordWriter assume it is.

    if (recp->compType != DeflateCompression)
        abortReader("invalid compression type %lu", recp->compType);
}



//----------------------------------------------------------------------
// Repetition and point-list
// The code for these two functions is the same as in OasisRecordReader.


// readRepetition -- read the repetition field of element records

void
AsciiRecordReader::readRepetition (/*out*/ RawRepetition* rawrep)
{
    // In the ASCII file as in the OASIS file, a repetition
    // begins with an unsigned-integer that gives its type.
    //
    // XXX: Perhaps we should write the rep type symbolically.
    // But that would mean a dozen more keywords.

    Ulong  repType = readUInt();
    switch (repType) {
        // 7.6.2  Type 0
        case Rep_ReusePrevious:
            rawrep->makeReuse();
            break;

        // 7.6.3  Type 1
        // x-dimension y-dimension x-space y-space

        case Rep_Matrix: {
            Ulong  xdimen = readUInt();
            Ulong  ydimen = readUInt();
            Ulong  xspace = readUInt();
            Ulong  yspace = readUInt();
            rawrep->makeMatrix(xdimen, ydimen, xspace, yspace);
            break;
        }

        // 7.6.4  Type 2
        // x-dimension x-space

        case Rep_UniformX: {
            Ulong  xdimen = readUInt();
            Ulong  xspace = readUInt();
            rawrep->makeUniformX(xdimen, xspace);
            break;
        }

        // 7.6.5  Type 3
        // y-dimension y-space

        case Rep_UniformY: {
            Ulong  ydimen = readUInt();
            Ulong  yspace = readUInt();
            rawrep->makeUniformY(ydimen, yspace);
            break;
        }

        // 7.6.6  Type 4
        // x-dimension x-space_1 ... x-space_(n-1)
        //
        // After x-dimension come x-dimension + 1 unsigned-integers
        // giving the spacing between successive elements.

        case Rep_VaryingX: {
            Ulong  xdimen = readUInt();
            rawrep->makeVaryingX(xdimen);
            for (++xdimen;  xdimen != 0;  --xdimen)
                rawrep->addSpace(readUInt());
            break;
        }

        // 7.6.7  Type 5
        // x-dimension grid x-space_1 ... x-space_(n-1)
        //
        // Like Type 4, but with a grid (multiplier) before the spaces

        case Rep_GridVaryingX: {
            Ulong  xdimen = readUInt();
            Ulong  grid   = readUInt();
            rawrep->makeGridVaryingX(xdimen, grid);
            for (++xdimen;  xdimen != 0;  --xdimen)
                rawrep->addSpace(readUInt());
            break;
        }

        // 7.6.8  Type 6
        // y-dimension y-space_1 ... y-space_(n-1)
        //
        // Types 6 and 7 are the same as types 4 and 5 with x replaced by y.

        case Rep_VaryingY: {
            Ulong  ydimen = readUInt();
            rawrep->makeVaryingY(ydimen);
            for (++ydimen;  ydimen != 0;  --ydimen)
                rawrep->addSpace(readUInt());
            break;
        }

        // 7.6.9  Type 7
        // y-dimension grid y-space_1 ... y-space_(n-1)

        case Rep_GridVaryingY: {
            Ulong  ydimen = readUInt();
            Ulong  grid   = readUInt();
            rawrep->makeGridVaryingY(ydimen, grid);
            for (++ydimen;  ydimen != 0;  --ydimen)
                rawrep->addSpace(readUInt());
            break;
        }

        // 7.6.10  Type 8
        // n-dimension m-dimension n-displacement m-displacement

        case Rep_TiltedMatrix: {
            Ulong  ndimen = readUInt();
            Ulong  mdimen = readUInt();
            Delta  ndisp  = scanner.readGDelta();
            Delta  mdisp  = scanner.readGDelta();
            rawrep->makeTiltedMatrix(ndimen, mdimen, ndisp, mdisp);
            break;
        }

        // 7.6.11  Type 9
        // dimension displacement

        case Rep_Diagonal: {
            Ulong  dimen = readUInt();
            rawrep->makeDiagonal(dimen, scanner.readGDelta());
            break;
        }

        // 7.6.12  Type 10
        // dimension displacement_1 ... displacement_(n-1)

        case Rep_Arbitrary: {
            Ulong  dimen = readUInt();
            rawrep->makeArbitrary(dimen);
            for (++dimen;  dimen != 0;  --dimen)
                rawrep->addDelta(scanner.readGDelta());
            break;
        }

        // 7.6.13  Type 11
        // dimension grid displacement_1 ... displacement_(n-1)

        case Rep_GridArbitrary: {
            Ulong  dimen = readUInt();
            Ulong  grid  = readUInt();
            rawrep->makeGridArbitrary(dimen, grid);
            for (++dimen;  dimen != 0;  --dimen)
                rawrep->addDelta(scanner.readGDelta());
            break;
        }

        default:
            abortReader("invalid repetition type %lu", repType);
    }
}



// readPointList -- read the point-list field in POLYGON and PATH records
// We store the raw point-list in the same PointList class that
// OasisParser uses.  We don't need an analogue of RawRepetition because
// PointList is general enough for the raw data too.

void
AsciiRecordReader::readPointList (/*out*/ PointList* ptlist)
{
    // 7.7.1
    // A point-list begins with an unsigned-integer denoting its type.

    Ulong  type = readUInt();
    if (! PointList::typeIsValid(type))
        abortReader("invalid point list type %lu", type);
    PointList::ListType  ltype = static_cast<PointList::ListType>(type);
    ptlist->init(ltype);

    // After that is the number of deltas appearing in the file.
    Ulong  numDeltas = readUInt();

    // Then come the deltas for the points.  The form of the delta (1-delta,
    // 2-delta, etc.) depends on the list type.

    switch (ltype) {
        case PointList::ManhattanHorizFirst:
        case PointList::ManhattanVertFirst:
            // In the raw point-list, store 1-deltas in the x member
            // regardless of whether it's an X offset or Y offset.
            for (Ulong j = 0;  j < numDeltas;  ++j)
                ptlist->addPoint(Delta(scanner.readOneDelta(), 0));
            break;

        case PointList::Manhattan:
            for (Ulong j = 0;  j < numDeltas;  ++j)
                ptlist->addPoint(scanner.readTwoDelta());
            break;

        case PointList::Octangular:
            for (Ulong j = 0;  j < numDeltas;  ++j)
                ptlist->addPoint(scanner.readThreeDelta());
            break;

        // Both AllAngle and AllAngleDoubleDelta have a sequence of
        // g-deltas.  They differ only in the interpretation of the deltas.

        case PointList::AllAngle:
        case PointList::AllAngleDoubleDelta:
            for (Ulong j = 0;  j < numDeltas;  ++j)
                ptlist->addPoint(scanner.readGDelta());
            break;
    }
}


//----------------------------------------------------------------------
// Handling errors


// abortReader -- throw runtime_error for unrecoverable error
//   fmt        printf() format string for error message
//   ...        args, if any, for the format string
// The formatted error message can be retrieved from the exception by
// using exception::what().

void
AsciiRecordReader::abortReader (const char* fmt, ...)
{
    assert (currRecord != Null);
    char  msgbuf[256];

    // getNextRecord() puts the record's starting line number in
    // recPos.fileOffset.

    Ullong  currLine = currRecord->recPos.fileOffset;
    Uint  n = SNprintf(msgbuf, sizeof msgbuf,
                       "file '%s', %s record at line %llu: ",
                       filename.c_str(), GetRecordAscName(currRecord->recID),
                       currLine);
    va_list  ap;
    va_start(ap, fmt);
    if (n < sizeof(msgbuf) - 1)
        VSNprintf(msgbuf + n, sizeof(msgbuf) - n, fmt, ap);

    va_end(ap);
    ThrowRuntimeError("%s", msgbuf);
}



void
AsciiRecordReader::abortDupKeyword (AsciiKeyword keyword) {
    abortReader("field '%s' appears twice", GetKeywordName(keyword));
}


void
AsciiRecordReader::abortBadKeyword (AsciiKeyword keyword) {
    abortReader("field '%s' is not legal for this record",
                GetKeywordName(keyword));
}


}  // namespace Oasis
