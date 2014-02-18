// oasis/rec-reader.cc -- assemble tokens into records without interpretation
//
// last modified:   30-Dec-2009  Wed  22:09
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <memory>

#include "misc/utils.h"
#include "infobyte.h"
#include "rec-reader.h"


namespace Oasis {

using namespace std;
using namespace SoftJin;


OasisRecordReader::OasisRecordReader (OasisScanner& scan)
  : scanner(scan)
{
    rm.createRecords();   // create cache -- one record of each type
    currRecord = Null;
    wantValidation = true;
}



OasisRecordReader::~OasisRecordReader() { }



// getNextRecord -- read record beginning at current offset in file.
// The return value points to space owned by this class.  The caller is
// allowed to modify the struct, but the values in the struct may be
// changed by the next call to getNextRecord() or any other method of
// this class.  The caller must copy the values elsewhere if it wants to
// keep them.
//
// Preconditions:
//    - The scanner is positioned at the start of a record.
//      Note that for the first record the caller is responsible for
//      skipping the magic string that begins a file.
//    - The caller must get the START record before it gets the END record.
//      This is because we don't know whether to expect table-offsets in
//      the END record without reading the START record.

OasisRecord*
OasisRecordReader::getNextRecord()
{
    // Read the record-ID that begins the record.  Get the cached record
    // for that ID and fill in the record's type and file offset.

    OFilePos  recPos;
    RecordID  recID = scanner.readRecordID(&recPos);
    OasisRecord*  orecp = rm.getRecord(recID);
    orecp->recID = recID;
    orecp->recPos = recPos;

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

        case RID_CELLNAME:
        case RID_TEXTSTRING:
        case RID_PROPNAME:
        case RID_PROPSTRING:
            readNameRecord(static_cast<NameRecord*>(orecp));
            break;

        case RID_CELLNAME_R:
        case RID_TEXTSTRING_R:
        case RID_PROPNAME_R:
        case RID_PROPSTRING_R:
            readNameRecordWithRefnum(static_cast<NameRecord*>(orecp));
            break;

        case RID_LAYERNAME_GEOMETRY:
        case RID_LAYERNAME_TEXT:
            readLayerNameRecord(static_cast<LayerNameRecord*>(orecp));
            break;

        case RID_CELL_REF:
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

        case RID_XNAME:
        case RID_XNAME_R:
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

        default:
            assert (false);
    }
    return orecp;
}



// Wrappers for getNextRecord() when reading the START and END record,
// which appear at fixed positions in the file.


StartRecord*
OasisRecordReader::getStartRecord()
{
    // Paragraph 13.10 says that the first record must be START.
    // PAD and CBLOCK are not allowed before it.

    scanner.seekTo(StartRecordOffset);
    currRecord = getNextRecord();
    if (currRecord->recID != RID_START)
        abortReader("file does not begin with START record");
    return (static_cast<StartRecord*>(currRecord));
}



EndRecord*
OasisRecordReader::getEndRecord()
{
    const Uint  MinStartRecordSize = 5;     // with empty version string
    const Uint  MinFileSize = OasisMagicLength
                              + MinStartRecordSize
                              + EndRecordSize;
    currRecord = Null;
    Ullong  fileSize = scanner.getFileSize();
    if (fileSize < MinFileSize)
        abortReader("file is too small to be valid OASIS");

    scanner.seekTo(fileSize - EndRecordSize);
    currRecord = getNextRecord();
    if (currRecord->recID != RID_END)
        abortReader("no END record at the expected offset");
    return (static_cast<EndRecord*>(currRecord));
}



//----------------------------------------------------------------------
//                      Record-reading functions
//----------------------------------------------------------------------

// Everything below is private.  We have one function for each record
// type (more or less).  Each readFooRecord() reads a record of type FOO
// into the FooRecord structure whose address it is given.  Each expects
// the scanner's read cursor to be immediately after the record-ID
// that begins the record.  Each leaves the cursor at the beginning of
// the next record.


// First some inline functions to shorten the code.

inline long
OasisRecordReader::readSInt() {
    return scanner.readSignedInteger();
}

inline Ulong
OasisRecordReader::readUInt() {
    return scanner.readUnsignedInteger();
}

inline llong
OasisRecordReader::readSInt64() {
    return scanner.readSignedInteger64();
}

inline Ullong
OasisRecordReader::readUInt64() {
    return scanner.readUnsignedInteger64();
}

inline void
OasisRecordReader::readString (/*out*/ string* pstr) {
    scanner.readString(pstr);
}




//----------------------------------------------------------------------
// START and END records

void
OasisRecordReader::readStartRecord (/*inout*/ StartRecord* recp)
{
    // Section 13:  START record
    // `1' version-string unit offset-flag [table-offsets]

    assert (recp->recID == RID_START);

    readString(&recp->version);
    scanner.readReal(&recp->unit);
    offsetFlag = recp->offsetFlag = readUInt();
    if (recp->offsetFlag == 0)
        readTableOffsets(&recp->offsets);
}



// readEndRecord() requires readStartRecord() to have been called
// earlier because it uses the member offsetFlag that readStartRecord()
// sets.

void
OasisRecordReader::readEndRecord (/*inout*/ EndRecord* recp)
{
    // Section 14:  END record
    // `2' [table-offsets] padding-string validation-scheme
    //     [validation-signature]

    assert (recp->recID == RID_END);

    recp->offsetFlag = offsetFlag;      // copy offset-flag from START record
    if (offsetFlag != 0)
        readTableOffsets(&recp->offsets);

    if (wantValidation) {
        string  padding;
        readString(&padding);           // discard padding
        recp->valScheme = readUInt();
        if (recp->valScheme != Validation::None)
            recp->valSignature = scanner.readValidationSignature();
    } else {
        // Fake the validation scheme for safety.
        recp->valScheme = Validation::None;
    }
}



void
OasisRecordReader::readTableOffsets (/*out*/ TableOffsets* toffs)
{
    readTableInfo(&toffs->cellName);
    readTableInfo(&toffs->textString);
    readTableInfo(&toffs->propName);
    readTableInfo(&toffs->propString);
    readTableInfo(&toffs->layerName);
    readTableInfo(&toffs->xname);
}



void
OasisRecordReader::readTableInfo (/*out*/ TableOffsets::TableInfo* tinfo)
{
    tinfo->strict = readUInt();
    tinfo->offset = readUInt64();
}


//----------------------------------------------------------------------
// Name records

// readNameRecord() is invoked for the variants of CELLNAME, TEXTSTRING,
// PROPNAME and PROPSTRING that contain only a name.  The
// reference-number is automatically generated.

void
OasisRecordReader::readNameRecord (/*inout*/ NameRecord* recp)
{
    assert (recp->recID == RID_CELLNAME
            ||  recp->recID == RID_TEXTSTRING
            ||  recp->recID == RID_PROPNAME
            ||  recp->recID == RID_PROPSTRING);

    readString(&recp->name);
}



// readNameRecordWithRefnum() is invoked for the variants of CELLNAME,
// TEXTSTRING, PROPNAME and PROPSTRING that contain both a name and a
// reference-number.

void
OasisRecordReader::readNameRecordWithRefnum (/*inout*/ NameRecord* recp)
{
    assert (recp->recID == RID_CELLNAME_R
            ||  recp->recID == RID_TEXTSTRING_R
            ||  recp->recID == RID_PROPNAME_R
            ||  recp->recID == RID_PROPSTRING_R);

    readString(&recp->name);
    recp->refnum = readUInt();
}



void
OasisRecordReader::readLayerNameRecord (/*inout*/ LayerNameRecord* recp)
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
OasisRecordReader::readInterval (/*out*/ LayerNameRecord::RawInterval* ivalp)
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
// Cell record


void
OasisRecordReader::readCellRecord (/*inout*/ CellRecord* recp)
{
    // Section 20:  CELL Record
    // `13' reference-number
    // `14' cellname-string

    assert (recp->recID == RID_CELL_REF  ||  recp->recID == RID_CELL_NAMED);

    if (recp->recID == RID_CELL_REF)
        recp->refnum = readUInt();
    else
        readString(&recp->name);
}



//----------------------------------------------------------------------
// Element records

void
OasisRecordReader::readPlacementRecord (/*inout*/ PlacementRecord* recp)
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

    int  infoByte = scanner.readByte();
    recp->infoByte = infoByte;
    if (infoByte & CellExplicitBit) {
        if (infoByte & RefnumBit)
            recp->refnum = readUInt();
        else
            readString(&recp->name);
    }

    if (recp->recID == RID_PLACEMENT_TRANSFORM) {
        if (infoByte & MagBit)    scanner.readReal(&recp->mag);
        if (infoByte & AngleBit)  scanner.readReal(&recp->angle);
    }

    if (infoByte & XBit)   recp->x = readSInt();
    if (infoByte & YBit)   recp->y = readSInt();
    if (infoByte & RepBit) readRepetition(&recp->rawrep);
}



void
OasisRecordReader::readTextRecord (/*inout*/ TextRecord* recp)
{
    // Section 24:  TEXT record
    // `19' text-info-byte [reference-number | text-string]
    //      [textlayer-number] [texttype-number] [x] [y] [repetition]
    // text-info-byte ::= 0CNXYRTL

    assert (recp->recID == RID_TEXT);
    using namespace TextInfoByte;

    int  infoByte = scanner.readByte();
    recp->infoByte = infoByte;
    if (infoByte & TextExplicitBit) {
        if (infoByte & RefnumBit)
            recp->refnum = readUInt();
        else
            readString(&recp->text);
    }
    if (infoByte & TextlayerBit) recp->textlayer = readUInt();
    if (infoByte & TexttypeBit)  recp->texttype  = readUInt();
    if (infoByte & XBit)         recp->x         = readSInt();
    if (infoByte & YBit)         recp->y         = readSInt();
    if (infoByte & RepBit)       readRepetition(&recp->rawrep);
}



void
OasisRecordReader::readRectangleRecord (/*inout*/ RectangleRecord* recp)
{
    // Section 25:  RECTANGLE record
    // `20' rectangle-info-byte [layer-number] [datatype-number]
    //      [width] [height] [x] [y] [repetition]
    // rectangle-info-byte ::= SWHXYRDL
    // If S = 1, the rectangle is a square, and H must be 0.

    assert (recp->recID == RID_RECTANGLE);
    using namespace RectangleInfoByte;

    int  infoByte = scanner.readByte();
    recp->infoByte = infoByte;
    if (infoByte & LayerBit)    recp->layer    = readUInt();
    if (infoByte & DatatypeBit) recp->datatype = readUInt();
    if (infoByte & WidthBit)    recp->width    = readUInt();
    if (infoByte & HeightBit)   recp->height   = readUInt();
    if (infoByte & XBit)        recp->x        = readSInt();
    if (infoByte & YBit)        recp->y        = readSInt();
    if (infoByte & RepBit)      readRepetition(&recp->rawrep);
}



void
OasisRecordReader::readPolygonRecord (/*inout*/ PolygonRecord* recp)
{
    // Section 26:  POLYGON Record
    // `21' polygon-info-byte [layer-number] [datatype-number]
    //      [point-list] [x] [y] [repetition]
    // polygon-info-byte ::= 00PXYRDL

    assert (recp->recID == RID_POLYGON);
    using namespace PolygonInfoByte;

    int  infoByte = scanner.readByte();
    recp->infoByte = infoByte;

    if (infoByte & LayerBit)     recp->layer    = readUInt();
    if (infoByte & DatatypeBit)  recp->datatype = readUInt();
    if (infoByte & PointListBit) readPointList(&recp->ptlist);
    if (infoByte & XBit)         recp->x        = readSInt();
    if (infoByte & YBit)         recp->y        = readSInt();
    if (infoByte & RepBit)       readRepetition(&recp->rawrep);
}



void
OasisRecordReader::readPathRecord (/*inout*/ PathRecord* recp)
{
    // Section 27:  PATH record
    // `22' path-info-byte [layer-number] [datatype-number] [half-width]
    //      [ extension-scheme [start-extension] [end-extension] ]
    //      [point-list] [x] [y] [repetition]
    // path-info-byte   ::= EWPXYRDL
    // extension-scheme ::= 0000SSEE

    assert (recp->recID == RID_PATH);
    using namespace PathInfoByte;

    int  infoByte = scanner.readByte();
    recp->infoByte = infoByte;

    if (infoByte & LayerBit)     recp->layer     = readUInt();
    if (infoByte & DatatypeBit)  recp->datatype  = readUInt();
    if (infoByte & HalfwidthBit) recp->halfwidth = readUInt();

    Ulong  extnScheme = (infoByte & ExtensionBit) ? readUInt() : 0;
    recp->extnScheme = extnScheme;
    if (GetStartScheme(extnScheme) == ExplicitExtn)
        recp->startExtn = readSInt();
    if (GetEndScheme(extnScheme) == ExplicitExtn)
        recp->endExtn = readSInt();

    if (infoByte & PointListBit) readPointList(&recp->ptlist);
    if (infoByte & XBit)   recp->x = readSInt();
    if (infoByte & YBit)   recp->y = readSInt();
    if (infoByte & RepBit) readRepetition(&recp->rawrep);
}



void
OasisRecordReader::readTrapezoidRecord (/*inout*/ TrapezoidRecord* recp)
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

    int  infoByte = scanner.readByte();
    recp->infoByte = infoByte;

    if (infoByte & LayerBit)    recp->layer    = readUInt();
    if (infoByte & DatatypeBit) recp->datatype = readUInt();
    if (infoByte & WidthBit)    recp->width    = readUInt();
    if (infoByte & HeightBit)   recp->height   = readUInt();

    if (recp->recID == RID_TRAPEZOID  ||  recp->recID == RID_TRAPEZOID_A)
        recp->delta_a = scanner.readOneDelta();
    if (recp->recID == RID_TRAPEZOID  ||  recp->recID == RID_TRAPEZOID_B)
        recp->delta_b = scanner.readOneDelta();

    if (infoByte & XBit)        recp->x        = readSInt();
    if (infoByte & YBit)        recp->y        = readSInt();
    if (infoByte & RepBit)      readRepetition(&recp->rawrep);
}



void
OasisRecordReader::readCTrapezoidRecord (/*inout*/ CTrapezoidRecord* recp)
{
    // Section 29:  CTRAPEZOID record
    // `26' ctrapezoid-info-byte [layer-number] [datatype-number]
    //      [ctrapezoid-type] [width] [height] [x] [y] [repetition]
    // ctrapezoid-info-byte ::= TWHXYRDL

    assert (recp->recID == RID_CTRAPEZOID);
    using namespace CTrapezoidInfoByte;

    int  infoByte = scanner.readByte();
    recp->infoByte = infoByte;

    if (infoByte & LayerBit)    recp->layer     = readUInt();
    if (infoByte & DatatypeBit) recp->datatype  = readUInt();
    if (infoByte & TrapTypeBit) recp->ctrapType = readUInt();
    if (infoByte & WidthBit)    recp->width     = readUInt();
    if (infoByte & HeightBit)   recp->height    = readUInt();
    if (infoByte & XBit)        recp->x         = readSInt();
    if (infoByte & YBit)        recp->y         = readSInt();
    if (infoByte & RepBit)      readRepetition(&recp->rawrep);
}



void
OasisRecordReader::readCircleRecord (/*inout*/ CircleRecord* recp)
{
    // Section 30:  CIRCLE record
    // `27' circle-info-byte [layer-number] [datatype-number]
    //      [radius] [x] [y] [repetition]
    // circle-info-byte ::= 00rXYRDL   (r is radius, R is repetition)

    assert (recp->recID == RID_CIRCLE);
    using namespace CircleInfoByte;

    int  infoByte = scanner.readByte();
    recp->infoByte = infoByte;

    if (infoByte & LayerBit)    recp->layer     = readUInt();
    if (infoByte & DatatypeBit) recp->datatype  = readUInt();
    if (infoByte & RadiusBit)   recp->radius    = readUInt();
    if (infoByte & XBit)        recp->x         = readSInt();
    if (infoByte & YBit)        recp->y         = readSInt();
    if (infoByte & RepBit)      readRepetition(&recp->rawrep);
}



//----------------------------------------------------------------------
// Properties


void
OasisRecordReader::readPropertyRecord (/*inout*/ PropertyRecord* recp)
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

    int  infoByte = scanner.readByte();
    recp->infoByte = infoByte;

    if (infoByte & NameExplicitBit) {
        if (infoByte & RefnumBit)
            recp->refnum = readUInt();
        else
            readString(&recp->name);
    }

    // Delete any old PropertyValues that happen to remain in the
    // PropertyRecord.  Note that when OasisParser uses OasisRecordReader
    // it takes ownership of all the values and removes them from the
    // PropertyRecord.

    DeleteContainerElements(recp->values);

    // If the V bit is set, the previous PROPERTY record's values
    // should be reused.

    if (infoByte & ReuseValueBit)
        return;

    // Otherwise look at UUUU in the info-byte.  If its value is in
    // 0..14, that is the number of values in the property value list
    // that follows.  If it is 15, the number is the next field,
    // prop-value-count.  We don't need to store numValues in the
    // PropertyRecord because the upper layer can look at the size of
    // the PropValueVector.

    Ulong  numValues = GetNumValues(infoByte);
    if (numValues == 15)
        numValues = readUInt();

    // Read each value and add it to the record.  The auto_ptr ensures
    // that the Property object is deleted if push_back() throws.

    for (Ulong j = 0;  j < numValues;  ++j) {
        auto_ptr<PropValue>  propval(readPropertyValue());
        recp->values.push_back(propval.get());
        propval.release();
    }
}



PropValue*
OasisRecordReader::readPropertyValue()
{
    // Each property value begins with an unsigned-integer that gives
    // the type of the value.  For real values, this type field also
    // serves as the type field of the real.

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
            scanner.readReal(valType, &rval);
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

        // If the value is a propstring-reference-number, store the refnum
        // itself in the PropValue.  The parser will resolve the reference
        // into a PropString*.  This layer is not concerned with name objects.

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
OasisRecordReader::readXNameRecord (/*inout*/ XNameRecord* recp)
{
    // Section 32:  XNAME record
    // `30' xname-attribute xname-string
    // `31' xname-attribute xname-string reference-number

    assert (recp->recID == RID_XNAME  ||  recp->recID == RID_XNAME_R);

    recp->attribute = readUInt();
    readString(&recp->name);
    if (recp->recID == RID_XNAME_R)
        recp->refnum = readUInt();
}



void
OasisRecordReader::readXElementRecord (/*inout*/ XElementRecord* recp)
{
    // Section 33:  XELEMENT record
    // `32' xelement-attribute xelement-string

    assert (recp->recID == RID_XELEMENT);

    recp->attribute = readUInt();
    readString(&recp->data);
}



void
OasisRecordReader::readXGeometryRecord (/*inout*/ XGeometryRecord* recp)
{
    // Section 34:  XGEOMETRY record
    // `33' xgeometry-info-byte xgeometry-attribute [layer-number]
    //      [datatype-number] xgeometry-string [x] [y] [repetition]
    // xgeometry-info-byte ::= 000XYRDL

    assert (recp->recID == RID_XGEOMETRY);
    using namespace XGeometryInfoByte;

    int  infoByte = scanner.readByte();
    recp->infoByte = infoByte;

    recp->attribute = readUInt();
    if (infoByte & LayerBit)    recp->layer     = readUInt();
    if (infoByte & DatatypeBit) recp->datatype  = readUInt();
    readString(&recp->data);
    if (infoByte & XBit)        recp->x         = readSInt();
    if (infoByte & YBit)        recp->y         = readSInt();
    if (infoByte & RepBit)      readRepetition(&recp->rawrep);
}



void
OasisRecordReader::readCblockRecord (/*inout*/ CblockRecord* recp)
{
    // Section 35:  CBLOCK record
    // `34' comp-type uncomp-byte-count comp-byte-count comp-bytes

    assert (recp->recID == RID_CBLOCK);

    recp->compType    = readUInt();
    recp->uncompBytes = readUInt64();
    recp->compBytes   = readUInt64();

    // Decompress the compressed data so that the next call to
    // getNextRecord() reads the uncompressed contents of the cblock.

    scanner.decompressBlock(recp->compType,
                            recp->uncompBytes, recp->compBytes);
}



//----------------------------------------------------------------------
// Repetition and point-list


// readRepetition -- read the repetition field of element records

void
OasisRecordReader::readRepetition (/*out*/ RawRepetition* rawrep)
{
    // 7.6.1: "A repetition consists of an unsigned-integer which
    // encodes the type, followed by any repetition parameters."

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
            Delta  ndisp = scanner.readGDelta();
            Delta  mdisp = scanner.readGDelta();
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
//
// These are the differences between the raw PointList created here
// and the PointList that OasisParser passes to OasisBuilder:
//   - OasisParser inserts the implicit first point at (0, 0); this does not.
//   - This stores the deltas as they are in the file; OasisParser
//     converts everything into offsets with respect to the first point
//   - For types 0 and 1 (ManhattanHorizFirst and ManhattanVertFirst),
//     this stores all the 1-deltas in the x member of the Delta.

void
OasisRecordReader::readPointList (/*out*/ PointList* ptlist)
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


// abortReader -- throw runtime_error for unrecoverable error
//   fmt        printf() format string for error message
//   ...        args, if any, for the format string
// The formatted error message can be retrieved from the exception by
// using exception::what().

void
OasisRecordReader::abortReader (const char* fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);

    char  msgbuf[512];
    const size_t  bufsize = sizeof(msgbuf);

    Uint  n = SNprintf(msgbuf, bufsize, "file '%s'", scanner.getFileName());
    if (n < bufsize-1  &&  currRecord != Null) {
        char  posbuf[100];
        currRecord->recPos.print(posbuf, sizeof posbuf);
        n += SNprintf(msgbuf+n, bufsize-n, ", %s(%d) record at %s",
                      GetRecordErrName(currRecord->recID),
                      currRecord->recID, posbuf);
    }
    if (n < bufsize-1)
        n += SNprintf(msgbuf+n, bufsize-n, ": ");
    if (n < bufsize-1)
        VSNprintf(msgbuf+n, bufsize-n, fmt, ap);

    va_end(ap);
    ThrowRuntimeError("%s", msgbuf);
}


}  // namespace Oasis
