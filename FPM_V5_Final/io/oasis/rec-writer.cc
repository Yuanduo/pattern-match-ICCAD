// oasis/rec-writer.cc -- create OASIS file record by record
//
// last modified:   28-Dec-2009  Mon  16:47
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cassert>
#include "infobyte.h"
#include "rec-writer.h"

namespace Oasis {

using namespace std;
using namespace SoftJin;


OasisRecordWriter::OasisRecordWriter (const char* fname) : writer(fname) { }
OasisRecordWriter::~OasisRecordWriter() { }


// beginFile -- open the file for writing
// This must be the first method invoked.

void
OasisRecordWriter::beginFile (Validation::Scheme valScheme) {
    writer.beginFile(valScheme);
    writer.writeBytes(OasisMagic, OasisMagicLength);
}



// endFile -- close the file
// This must be the last method invoked.

void
OasisRecordWriter::endFile() {
    writer.endFile();
}



void
OasisRecordWriter::endCblock() {
    writer.endCblock();
}



// writeRecord -- write a single OASIS record
// The argument orecp must point to an instance of OasisRecord or one of
// its subclasses.  The recID field in the record must be one of the
// RID_* enumerators (RIDX_* enumerators are not valid here; those
// records appear only in the ASCII version).  The record's recPos field
// is not used.
//
// For CBLOCK records, writeRecord begins a cblock of the compression
// type given by the CblockRecord::compType.  The caller must invoke
// endCblock() later to end the cblock.
//
// OasisRecordWriter always puts the table-offsets in the END record.
// So in StartRecord and EndRecord the member 'offsetFlag' must be 1,
// and in EndRecord the member 'offsets' must be defined.
//
// In the END record the validation scheme and signature are ignored.

void
OasisRecordWriter::writeRecord (const OasisRecord* orecp)
{
    // Write the record-ID and then call the type-specific function to
    // write the rest of the record.  RID_CBLOCK needs special treatment
    // because OasisWriter::beginCblock(), which writeCblockRecord()
    // calls, writes the record-ID itself.

    assert (RecordTypeIsValid(orecp->recID));
    if (orecp->recID != RID_CBLOCK)
        writer.beginRecord(static_cast<RecordID>(orecp->recID));

    switch (orecp->recID) {
        case RID_PAD:
            // nothing to do
            break;

        case RID_START:
            writeStartRecord(static_cast<const StartRecord*>(orecp));
            break;

        case RID_END:
            writeEndRecord(static_cast<const EndRecord*>(orecp));
            break;

        case RID_CELLNAME:
        case RID_TEXTSTRING:
        case RID_PROPNAME:
        case RID_PROPSTRING:
            writeNameRecord(static_cast<const NameRecord*>(orecp));
            break;

        case RID_CELLNAME_R:
        case RID_TEXTSTRING_R:
        case RID_PROPNAME_R:
        case RID_PROPSTRING_R:
            writeNameRecordWithRefnum(static_cast<const NameRecord*>(orecp));
            break;

        case RID_LAYERNAME_GEOMETRY:
        case RID_LAYERNAME_TEXT:
            writeLayerNameRecord(static_cast<const LayerNameRecord*>(orecp));
            break;

        case RID_CELL_REF:
        case RID_CELL_NAMED:
            writeCellRecord(static_cast<const CellRecord*>(orecp));
            break;

        case RID_XYABSOLUTE:
        case RID_XYRELATIVE:
            // nothing to do
            break;

        case RID_PLACEMENT:
        case RID_PLACEMENT_TRANSFORM:
            writePlacementRecord(static_cast<const PlacementRecord*>(orecp));
            break;

        case RID_TEXT:
            writeTextRecord(static_cast<const TextRecord*>(orecp));
            break;

        case RID_RECTANGLE:
            writeRectangleRecord(static_cast<const RectangleRecord*>(orecp));
            break;

        case RID_POLYGON:
            writePolygonRecord(static_cast<const PolygonRecord*>(orecp));
            break;

        case RID_PATH:
            writePathRecord(static_cast<const PathRecord*>(orecp));
            break;

        case RID_TRAPEZOID:
        case RID_TRAPEZOID_A:
        case RID_TRAPEZOID_B:
            writeTrapezoidRecord(static_cast<const TrapezoidRecord*>(orecp));
            break;

        case RID_CTRAPEZOID:
            writeCTrapezoidRecord(static_cast<const CTrapezoidRecord*>(orecp));
            break;

        case RID_CIRCLE:
            writeCircleRecord(static_cast<const CircleRecord*>(orecp));
            break;

        case RID_PROPERTY:
            writePropertyRecord(static_cast<const PropertyRecord*>(orecp));
            break;

        case RID_PROPERTY_REPEAT:
            // nothing to do
            break;

        case RID_XNAME:
        case RID_XNAME_R:
            writeXNameRecord(static_cast<const XNameRecord*>(orecp));
            break;

        case RID_XELEMENT:
            writeXElementRecord(static_cast<const XElementRecord*>(orecp));
            break;

        case RID_XGEOMETRY:
            writeXGeometryRecord(static_cast<const XGeometryRecord*>(orecp));
            break;

        case RID_CBLOCK:
            writeCblockRecord(static_cast<const CblockRecord*>(orecp));
            break;

        default:
            assert (false);
    }
}


//----------------------------------------------------------------------
// Everything below is private.
// First some inline functions to shorten the code.

inline void
OasisRecordWriter::writeUInt (Ulong val) {
    writer.writeUnsignedInteger(val);
}

inline void
OasisRecordWriter::writeSInt (long val) {
    writer.writeSignedInteger(val);
}

inline void
OasisRecordWriter::writeUInt64 (Ullong val) {
    writer.writeUnsignedInteger64(val);
}

inline void
OasisRecordWriter::writeSInt64 (llong val) {
    writer.writeSignedInteger64(val);
}

inline void
OasisRecordWriter::writeReal (const Oreal& val) {
    writer.writeReal(val);
}

inline void
OasisRecordWriter::writeString (const string& val) {
    writer.writeString(val);
}

inline void
OasisRecordWriter::writeInfoByte (int infoByte) {
    assert ((infoByte & ~0xff) == 0);
    writer.writeByte(infoByte);
}


//----------------------------------------------------------------------


void
OasisRecordWriter::writeStartRecord (const StartRecord* recp)
{
    // Section 13:  START record
    // `1' version-string unit offset-flag [table-offsets]

    // We do not support writing the table-offsets in the START record.
    // They must be in the END record.

    assert (recp->recID == RID_START);
    assert (recp->offsetFlag != 0);

    writeString(recp->version);
    writeReal(recp->unit);
    writeUInt(recp->offsetFlag);
}



void
OasisRecordWriter::writeEndRecord (const EndRecord* recp)
{
    // Section 14:  END record
    // `2' [table-offsets] padding-string validation-scheme
    //     [validation-signature]

    assert (recp->recID == RID_END);
    assert (recp->offsetFlag != 0);

    writeTableOffsets(recp->offsets);
    // The call to writer.endFile() in endFile() writes the padding
    // string and validation.
}



void
OasisRecordWriter::writeTableOffsets (const TableOffsets& toffs)
{
    writeTableInfo(toffs.cellName);
    writeTableInfo(toffs.textString);
    writeTableInfo(toffs.propName);
    writeTableInfo(toffs.propString);
    writeTableInfo(toffs.layerName);
    writeTableInfo(toffs.xname);
}



void
OasisRecordWriter::writeTableInfo (const TableOffsets::TableInfo& tinfo)
{
    writeUInt(tinfo.strict);
    writeUInt64(tinfo.offset);
}


//----------------------------------------------------------------------
// Name records


// writeNameRecord() is invoked for the variants of CELLNAME, TEXTSTRING,
// PROPNAME and PROPSTRING that contain only a name.

void
OasisRecordWriter::writeNameRecord (const NameRecord* recp)
{
    assert (recp->recID == RID_CELLNAME
            ||  recp->recID == RID_TEXTSTRING
            ||  recp->recID == RID_PROPNAME
            ||  recp->recID == RID_PROPSTRING);

    writeString(recp->name);
}



// writeNameRecordWithRefnum() is invoked for the variants of CELLNAME,
// TEXTSTRING, PROPNAME and PROPSTRING that contain both a name and a
// reference-number.

void
OasisRecordWriter::writeNameRecordWithRefnum (const NameRecord* recp)
{
    assert (recp->recID == RID_CELLNAME_R
            ||  recp->recID == RID_TEXTSTRING_R
            ||  recp->recID == RID_PROPNAME_R
            ||  recp->recID == RID_PROPSTRING_R);

    writeString(recp->name);
    writeUInt(recp->refnum);
}



void
OasisRecordWriter::writeLayerNameRecord (const LayerNameRecord* recp)
{
    // Section 19:  LAYERNAME record
    // `11' layername-string layer-interval datatype-interval
    // `12' layername-string textlayer-interval texttype-interval

    assert (recp->recID == RID_LAYERNAME_GEOMETRY
            ||  recp->recID == RID_LAYERNAME_TEXT);
    writeString(recp->name);
    writeInterval(recp->layers);
    writeInterval(recp->types);
}



void
OasisRecordWriter::writeInterval (const LayerNameRecord::RawInterval& ival)
{
    writeUInt(ival.type);
    switch (ival.type) {
        case 0:
            break;
        case 1:
        case 2:
        case 3:
            writeUInt(ival.bound_a);
            break;
        case 4:
            writeUInt(ival.bound_a);
            writeUInt(ival.bound_b);
            break;
        default:
            assert (false);
    }
}


//----------------------------------------------------------------------
// Cell record


void
OasisRecordWriter::writeCellRecord (const CellRecord* recp)
{
    // Section 20:  CELL Record
    // `13' reference-number
    // `14' cellname-string

    assert (recp->recID == RID_CELL_REF  ||  recp->recID == RID_CELL_NAMED);

    if (recp->recID == RID_CELL_REF)
        writeUInt(recp->refnum);
    else
        writeString(recp->name);
}


//----------------------------------------------------------------------
// Element records


void
OasisRecordWriter::writePlacementRecord (const PlacementRecord* recp)
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

    int  infoByte = recp->infoByte;
    writeInfoByte(infoByte);

    if (infoByte & CellExplicitBit) {
        if (infoByte & RefnumBit)
            writeUInt(recp->refnum);
        else
            writeString(recp->name);
    }
    if (recp->recID == RID_PLACEMENT_TRANSFORM) {
        if (infoByte & MagBit)    writeReal(recp->mag);
        if (infoByte & AngleBit)  writeReal(recp->angle);
    }

    if (infoByte & XBit)   writeSInt(recp->x);
    if (infoByte & YBit)   writeSInt(recp->y);
    if (infoByte & RepBit) writeRepetition(recp->rawrep);
}



void
OasisRecordWriter::writeTextRecord (const TextRecord* recp)
{
    // Section 24:  TEXT record
    // `19' text-info-byte [reference-number | text-string]
    //      [textlayer-number] [texttype-number] [x] [y] [repetition]
    // text-info-byte ::= 0CNXYRTL

    assert (recp->recID == RID_TEXT);
    using namespace TextInfoByte;

    int  infoByte = recp->infoByte;
    writeInfoByte(infoByte);

    if (infoByte & TextExplicitBit) {
        if (infoByte & RefnumBit)
            writeUInt(recp->refnum);
        else
            writeString(recp->text);
    }
    if (infoByte & TextlayerBit) writeUInt(recp->textlayer);
    if (infoByte & TexttypeBit)  writeUInt(recp->texttype);
    if (infoByte & XBit)         writeSInt(recp->x);
    if (infoByte & YBit)         writeSInt(recp->y);
    if (infoByte & RepBit)       writeRepetition(recp->rawrep);
}



void
OasisRecordWriter::writeRectangleRecord (const RectangleRecord* recp)
{
    // Section 25:  RECTANGLE record
    // `20' rectangle-info-byte [layer-number] [datatype-number]
    //      [width] [height] [x] [y] [repetition]
    // rectangle-info-byte ::= SWHXYRDL
    // If S = 1, the rectangle is a square, and H must be 0.

    assert (recp->recID == RID_RECTANGLE);
    using namespace RectangleInfoByte;

    int  infoByte = recp->infoByte;
    writeInfoByte(infoByte);

    if (infoByte & LayerBit)    writeUInt(recp->layer);
    if (infoByte & DatatypeBit) writeUInt(recp->datatype);
    if (infoByte & WidthBit)    writeUInt(recp->width);
    if (infoByte & HeightBit)   writeUInt(recp->height);
    if (infoByte & XBit)        writeSInt(recp->x);
    if (infoByte & YBit)        writeSInt(recp->y);
    if (infoByte & RepBit)      writeRepetition(recp->rawrep);
}



void
OasisRecordWriter::writePolygonRecord (const PolygonRecord* recp)
{
    // Section 26:  POLYGON Record
    // `21' polygon-info-byte [layer-number] [datatype-number]
    //      [point-list] [x] [y] [repetition]
    // polygon-info-byte ::= 00PXYRDL

    assert (recp->recID == RID_POLYGON);
    using namespace PolygonInfoByte;

    int  infoByte = recp->infoByte;
    writeInfoByte(infoByte);

    if (infoByte & LayerBit)     writeUInt(recp->layer);
    if (infoByte & DatatypeBit)  writeUInt(recp->datatype);
    if (infoByte & PointListBit) writePointList(recp->ptlist);
    if (infoByte & XBit)         writeSInt(recp->x);
    if (infoByte & YBit)         writeSInt(recp->y);
    if (infoByte & RepBit)       writeRepetition(recp->rawrep);
}



void
OasisRecordWriter::writePathRecord (const PathRecord* recp)
{
    // Section 27:  PATH record
    // `22' path-info-byte [layer-number] [datatype-number] [half-width]
    //      [ extension-scheme [start-extension] [end-extension] ]
    //      [point-list] [x] [y] [repetition]
    // path-info-byte   ::= EWPXYRDL
    // extension-scheme ::= 0000SSEE

    assert (recp->recID == RID_PATH);
    using namespace PathInfoByte;

    int  infoByte = recp->infoByte;
    writeInfoByte(infoByte);

    if (infoByte & LayerBit)     writeUInt(recp->layer);
    if (infoByte & DatatypeBit)  writeUInt(recp->datatype);
    if (infoByte & HalfwidthBit) writeUInt(recp->halfwidth);

    if (infoByte & ExtensionBit) {
        writeUInt(recp->extnScheme);
        if (GetStartScheme(recp->extnScheme) == ExplicitExtn)
            writeSInt(recp->startExtn);
        if (GetEndScheme(recp->extnScheme) == ExplicitExtn)
            writeSInt(recp->endExtn);
    }

    if (infoByte & PointListBit) writePointList(recp->ptlist);
    if (infoByte & XBit)         writeSInt(recp->x);
    if (infoByte & YBit)         writeSInt(recp->y);
    if (infoByte & RepBit)       writeRepetition(recp->rawrep);
}



void
OasisRecordWriter::writeTrapezoidRecord (const TrapezoidRecord* recp)
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

    int  infoByte = recp->infoByte;
    writeInfoByte(infoByte);

    if (infoByte & LayerBit)    writeUInt(recp->layer);
    if (infoByte & DatatypeBit) writeUInt(recp->datatype);
    if (infoByte & WidthBit)    writeUInt(recp->width);
    if (infoByte & HeightBit)   writeUInt(recp->height);

    if (recp->recID == RID_TRAPEZOID  ||  recp->recID == RID_TRAPEZOID_A)
        writer.writeOneDelta(recp->delta_a);
    if (recp->recID == RID_TRAPEZOID  ||  recp->recID == RID_TRAPEZOID_B)
        writer.writeOneDelta(recp->delta_b);

    if (infoByte & XBit)        writeSInt(recp->x);
    if (infoByte & YBit)        writeSInt(recp->y);
    if (infoByte & RepBit)      writeRepetition(recp->rawrep);
}



void
OasisRecordWriter::writeCTrapezoidRecord (const CTrapezoidRecord* recp)
{
    // Section 29:  CTRAPEZOID record
    // `26' ctrapezoid-info-byte [layer-number] [datatype-number]
    //      [ctrapezoid-type] [width] [height] [x] [y] [repetition]
    // ctrapezoid-info-byte ::= TWHXYRDL

    assert (recp->recID == RID_CTRAPEZOID);
    using namespace CTrapezoidInfoByte;

    int  infoByte = recp->infoByte;
    writeInfoByte(infoByte);

    if (infoByte & LayerBit)    writeUInt(recp->layer);
    if (infoByte & DatatypeBit) writeUInt(recp->datatype);
    if (infoByte & TrapTypeBit) writeUInt(recp->ctrapType);
    if (infoByte & WidthBit)    writeUInt(recp->width);
    if (infoByte & HeightBit)   writeUInt(recp->height);
    if (infoByte & XBit)        writeSInt(recp->x);
    if (infoByte & YBit)        writeSInt(recp->y);
    if (infoByte & RepBit)      writeRepetition(recp->rawrep);
}



void
OasisRecordWriter::writeCircleRecord (const CircleRecord* recp)
{
    // Section 30:  CIRCLE record
    // `27' circle-info-byte [layer-number] [datatype-number]
    //      [radius] [x] [y] [repetition]
    // circle-info-byte ::= 00rXYRDL   (r is radius, R is repetition)

    assert (recp->recID == RID_CIRCLE);
    using namespace CircleInfoByte;

    int  infoByte = recp->infoByte;
    writeInfoByte(infoByte);

    if (infoByte & LayerBit)    writeUInt(recp->layer);
    if (infoByte & DatatypeBit) writeUInt(recp->datatype);
    if (infoByte & RadiusBit)   writeUInt(recp->radius);
    if (infoByte & XBit)        writeSInt(recp->x);
    if (infoByte & YBit)        writeSInt(recp->y);
    if (infoByte & RepBit)      writeRepetition(recp->rawrep);
}



//----------------------------------------------------------------------
// Properties


void
OasisRecordWriter::writePropertyRecord (const PropertyRecord* recp)
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

    int  infoByte = recp->infoByte;
    writeInfoByte(infoByte);

    if (infoByte & NameExplicitBit) {
        if (infoByte & RefnumBit)
            writeUInt(recp->refnum);
        else
            writeString(recp->name);
    }

    // If the V bit is set, the previous PROPERTY record's values
    // should be reused.

    if (infoByte & ReuseValueBit)
        return;

    // prop-value-count, the number of values that follow, must appear
    // if UUUU in the info-byte is 1111 (decimal 15).

    if (GetNumValues(infoByte) == 15)
        writeUInt(recp->values.size());

    // Write the property values.

    PropValueVector::const_iterator  iter = recp->values.begin();
    PropValueVector::const_iterator  end  = recp->values.end();
    for ( ;  iter != end;  ++iter)
        writePropertyValue(*iter);
}



void
OasisRecordWriter::writePropertyValue (const PropValue* propval)
{
    // Handle real property values specially.  The type field written
    // for the real also serves as the type field of the property value.

    if (propval->isReal()) {
        writeReal(propval->getRealValue());
        return;
    }

    // Other types of property values.  Write the value type and then
    // the value.

    Uint  valType = propval->getType();
    writeUInt(valType);
    switch (valType) {
        case PV_UnsignedInteger:
            writeUInt64(propval->getUnsignedIntegerValue());
            break;

        case PV_SignedInteger:
            writeSInt64(propval->getSignedIntegerValue());
            break;

        case PV_AsciiString:
        case PV_BinaryString:
        case PV_NameString:
            writeString(propval->getStringValue());
            break;

        // If the value is a propstring-reference, it must be
        // unresolved, i.e., it must contain an unsigned-integer refnum,
        // not a PropString*, which this layer does not know about.

        case PV_Ref_AsciiString:
        case PV_Ref_BinaryString:
        case PV_Ref_NameString:
            assert (! propval->isResolved());
            writeUInt(propval->getUnsignedIntegerValue());
            break;

        default:
            assert (false);
    }
}


//----------------------------------------------------------------------
// Extension records

void
OasisRecordWriter::writeXNameRecord (const XNameRecord* recp)
{
    // Section 32:  XNAME record
    // `30' xname-attribute xname-string
    // `31' xname-attribute xname-string reference-number

    assert (recp->recID == RID_XNAME  ||  recp->recID == RID_XNAME_R);

    writeUInt(recp->attribute);
    writeString(recp->name);
    if (recp->recID == RID_XNAME_R)
        writeUInt(recp->refnum);
}



void
OasisRecordWriter::writeXElementRecord (const XElementRecord* recp)
{
    // Section 33:  XELEMENT record
    // `32' xelement-attribute xelement-string

    assert (recp->recID == RID_XELEMENT);

    writeUInt(recp->attribute);
    writeString(recp->data);
}



void
OasisRecordWriter::writeXGeometryRecord (const XGeometryRecord* recp)
{
    // Section 34:  XGEOMETRY record
    // `33' xgeometry-info-byte xgeometry-attribute [layer-number]
    //      [datatype-number] xgeometry-string [x] [y] [repetition]
    // xgeometry-info-byte ::= 000XYRDL

    assert (recp->recID == RID_XGEOMETRY);
    using namespace XGeometryInfoByte;

    int  infoByte = recp->infoByte;
    writeInfoByte(recp->infoByte);

    writeUInt(recp->attribute);
    if (infoByte & LayerBit)    writeUInt(recp->layer);
    if (infoByte & DatatypeBit) writeUInt(recp->datatype);
    writeString(recp->data);
    if (infoByte & XBit)        writeSInt(recp->x);
    if (infoByte & YBit)        writeSInt(recp->y);
    if (infoByte & RepBit)      writeRepetition(recp->rawrep);
}



void
OasisRecordWriter::writeCblockRecord (const CblockRecord* recp)
{
    // Section 35:  CBLOCK record
    // `34' comp-type uncomp-byte-count comp-byte-count comp-bytes

    assert (recp->recID == RID_CBLOCK);

    writer.beginCblock(recp->compType);
    // The caller must invoke endCblock() to end this cblock.
}


//----------------------------------------------------------------------
// Repetition and point-list


// writeRepetition -- write the repetition field of element records

void
OasisRecordWriter::writeRepetition (const RawRepetition& rawrep)
{
    // 7.6.1: "A repetition consists of an unsigned-integer which
    // encodes the type, followed by any repetition parameters."

    Uint  repType = rawrep.repType;
    writeUInt(repType);

    switch (repType) {
        // 7.6.2  Type 0
        case Rep_ReusePrevious:
            break;

        // 7.6.3  Type 1
        // x-dimension y-dimension x-space y-space

        case Rep_Matrix:
            writeUInt(rawrep.xdimen);
            writeUInt(rawrep.ydimen);
            writeUInt(rawrep.xspace);
            writeUInt(rawrep.yspace);
            break;

        // 7.6.4  Type 2
        // x-dimension x-space

        case Rep_UniformX:
            writeUInt(rawrep.dimen);
            writeUInt(rawrep.xspace);
            break;

        // 7.6.5  Type 3
        // y-dimension y-space

        case Rep_UniformY:
            writeUInt(rawrep.dimen);
            writeUInt(rawrep.yspace);
            break;

        // 7.6.6 to 7.6.9
        // Type 4:  x-dimension      x-space_1 ... x-space_(n-1)
        // Type 5:  x-dimension grid x-space_1 ... x-space_(n-1)
        // Type 6:  y-dimension      y-space_1 ... y-space_(n-1)
        // Type 7:  y-dimension grid y-space_1 ... y-space_(n-1)
        //
        // [xy]-dimension + 1 == (n-1)

        case Rep_VaryingX:
        case Rep_GridVaryingX:
        case Rep_VaryingY:
        case Rep_GridVaryingY: {
            assert (rawrep.dimen + 1 == rawrep.spaces.size());
            writeUInt(rawrep.dimen);
            if (repType == Rep_GridVaryingX  ||  repType == Rep_GridVaryingY)
                writeUInt(rawrep.grid);

            vector<Ulong>::const_iterator  iter = rawrep.spaces.begin();
            vector<Ulong>::const_iterator  end  = rawrep.spaces.end();
            for ( ;  iter != end;  ++iter)
                writeUInt(*iter);
            break;
        }

        // 7.6.10  Type 8
        // n-dimension m-dimension n-displacement m-displacement

        case Rep_TiltedMatrix:
            writeUInt(rawrep.ndimen);
            writeUInt(rawrep.mdimen);
            writer.writeGDelta(rawrep.getMatrixNdelta());
            writer.writeGDelta(rawrep.getMatrixMdelta());
            break;

        // 7.6.11  Type 9
        // dimension displacement

        case Rep_Diagonal:
            writeUInt(rawrep.dimen);
            writer.writeGDelta(rawrep.getDiagonalDelta());
            break;

        // 7.6.12 and 7.6.13
        // Type 10:  dimension      displacement_1 ... displacement_(n-1)
        // Type 11:  dimension grid displacement_1 ... displacement_(n-1)

        case Rep_Arbitrary:
        case Rep_GridArbitrary: {
            assert (rawrep.dimen + 1 == rawrep.deltas.size());
            writeUInt (rawrep.dimen);
            if (repType == Rep_GridArbitrary)
                writeUInt (rawrep.grid);

            vector<Delta>::const_iterator  iter = rawrep.deltas.begin();
            vector<Delta>::const_iterator  end  = rawrep.deltas.end();
            for ( ;  iter != end;  ++iter)
                writer.writeGDelta(*iter);
            break;
        }

        default:
            assert (false);
    }
}



// writePointList -- write the point-list field in POLYGON and PATH records

void
OasisRecordWriter::writePointList (const PointList& ptlist)
{
    // A point-list begins with an unsigned-integer denoting its type.
    // After that is the number of deltas appearing in the file.

    writeUInt(ptlist.getType());
    writeUInt(ptlist.size());

    PointList::const_iterator  iter = ptlist.begin();
    PointList::const_iterator  end  = ptlist.end();

    // Then come the deltas for the points.  The form of the delta (1-delta,
    // 2-delta, 3-delta, or g-delta) depends on the list type.

    switch (ptlist.getType()) {
        case PointList::ManhattanHorizFirst:
        case PointList::ManhattanVertFirst:
            // In the raw point-list, 1-deltas are stored in the x member
            // regardless of whether it's an X offset or Y offset.
            for ( ;  iter != end;  ++iter)
                writer.writeOneDelta(iter->x);
            break;

        case PointList::Manhattan:
            for ( ;  iter != end;  ++iter)
                writer.writeTwoDelta(*iter);
            break;

        case PointList::Octangular:
            for ( ;  iter != end;  ++iter)
                writer.writeThreeDelta(*iter);
            break;

        // Both AllAngle and AllAngleDoubleDelta have a sequence of
        // g-deltas.  They differ only in the interpretation of the deltas.

        case PointList::AllAngle:
        case PointList::AllAngleDoubleDelta:
            for ( ;  iter != end;  ++iter)
                writer.writeGDelta(*iter);
            break;
    }
}


}  // namespace Oasis
