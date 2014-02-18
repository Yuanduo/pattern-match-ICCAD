// oasis/asc-recwriter.cc -- write OASIS records in ASCII
//
// last modified:   02-Jan-2010  Sat  17:10
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <algorithm>
#include "asc-recwriter.h"
#include "infobyte.h"
#include "rectypes.h"


namespace Oasis {

using namespace std;
using namespace SoftJin;


// AsciiRecordWriter constructor
//   fname          pathname of output file
//   foldLongLines  whether long output lines should be folded.
//                  false => print each record on a single line regardless
//                  of how long it gets

AsciiRecordWriter::AsciiRecordWriter (const char* fname, bool foldLongLines)
  : writer(fname, foldLongLines)
{
    // Specify for each record type whether a blank line should be left
    // before records of that type.

    for (int j = 0;  j <= RIDX_MaxID;  ++j)
        breakAtRecord[j] = false;

    breakAtRecord[RID_CELL_REF]   = true;
    breakAtRecord[RID_CELL_NAMED] = true;

    breakAtRecord[RIDX_CELLNAME_TABLE]   = true;
    breakAtRecord[RIDX_TEXTSTRING_TABLE] = true;
    breakAtRecord[RIDX_PROPNAME_TABLE]   = true;
    breakAtRecord[RIDX_PROPSTRING_TABLE] = true;
    breakAtRecord[RIDX_LAYERNAME_TABLE]  = true;
    breakAtRecord[RIDX_XNAME_TABLE]      = true;

    fill_n(implicitRefnums, RefnumArraySize, 0);
}



void
AsciiRecordWriter::beginFile (bool showOffsets) {
    writer.beginFile();
    this->showOffsets = showOffsets;
}


void
AsciiRecordWriter::endFile() {
    writer.endFile();
}



void
AsciiRecordWriter::writeRecord (OasisRecord* orecp)
{
    // Write the record name, preceded by the record offset if we have
    // been asked to print offsets.  Leave a blank line before important
    // records.

    writer.beginRecord(orecp->recID, orecp->recPos,
                       showOffsets, breakAtRecord[orecp->recID] );

    // Call the record-specific function to print the rest of the record.

    switch (orecp->recID) {
        case RID_PAD:
            // nothing to do
            break;

        case RID_START:
            writeStartRecord(static_cast<StartRecord*>(orecp));
            break;

        case RID_END:
            writeEndRecord(static_cast<EndRecord*>(orecp));
            break;

        case RID_CELLNAME:
        case RID_TEXTSTRING:
        case RID_PROPNAME:
        case RID_PROPSTRING:
            writeNameRecord(static_cast<NameRecord*>(orecp));
            break;

        case RID_CELLNAME_R:
        case RID_TEXTSTRING_R:
        case RID_PROPNAME_R:
        case RID_PROPSTRING_R:
            writeNameRecordWithRefnum(static_cast<NameRecord*>(orecp));
            break;

        case RID_LAYERNAME_GEOMETRY:
        case RID_LAYERNAME_TEXT:
            writeLayerNameRecord(static_cast<LayerNameRecord*>(orecp));
            break;

        case RID_CELL_REF:
        case RID_CELL_NAMED:
            writeCellRecord(static_cast<CellRecord*>(orecp));
            break;

        case RID_XYABSOLUTE:
        case RID_XYRELATIVE:
            // nothing to do
            break;

        case RID_PLACEMENT:
        case RID_PLACEMENT_TRANSFORM:
            writePlacementRecord(static_cast<PlacementRecord*>(orecp));
            break;

        case RID_TEXT:
            writeTextRecord(static_cast<TextRecord*>(orecp));
            break;

        case RID_RECTANGLE:
            writeRectangleRecord(static_cast<RectangleRecord*>(orecp));
            break;

        case RID_POLYGON:
            writePolygonRecord(static_cast<PolygonRecord*>(orecp));
            break;

        case RID_PATH:
            writePathRecord(static_cast<PathRecord*>(orecp));
            break;

        case RID_TRAPEZOID:
        case RID_TRAPEZOID_A:
        case RID_TRAPEZOID_B:
            writeTrapezoidRecord(static_cast<TrapezoidRecord*>(orecp));
            break;

        case RID_CTRAPEZOID:
            writeCTrapezoidRecord(static_cast<CTrapezoidRecord*>(orecp));
            break;

        case RID_CIRCLE:
            writeCircleRecord(static_cast<CircleRecord*>(orecp));
            break;

        case RID_PROPERTY:
            writePropertyRecord(static_cast<PropertyRecord*>(orecp));
            break;

        case RID_PROPERTY_REPEAT:
            // nothing to do
            break;

        case RID_XNAME:
        case RID_XNAME_R:
            writeXNameRecord(static_cast<XNameRecord*>(orecp));
            break;

        case RID_XELEMENT:
            writeXElementRecord(static_cast<XElementRecord*>(orecp));
            break;

        case RID_XGEOMETRY:
            writeXGeometryRecord(static_cast<XGeometryRecord*>(orecp));
            break;

        case RID_CBLOCK:
            writeCblockRecord(static_cast<CblockRecord*>(orecp));
            break;

        // None of the pseudo-records contain data.
        case RIDX_END_CBLOCK:
        case RIDX_CELLNAME_TABLE:
        case RIDX_TEXTSTRING_TABLE:
        case RIDX_PROPNAME_TABLE:
        case RIDX_PROPSTRING_TABLE:
        case RIDX_LAYERNAME_TABLE:
        case RIDX_XNAME_TABLE:
            break;

        default:
            assert (false);
    }

    writer.endRecord();
}


//----------------------------------------------------------------------
// Everything that follows is private.
// First we have a bunch of inline functions to shorten the code.
// For each frequently-used data type we have two functions.  The first
// writes just a value and the second writes a keyword and then a value.
// The keyword/value variants are used for the element records.


inline void
AsciiRecordWriter::writeKeyword (AsciiKeyword keyword) {
    writer.writeKeyword(keyword);
}


inline void
AsciiRecordWriter::writeUInt (Ulong val) {
    writer.writeUnsignedInteger(val);
}


inline void
AsciiRecordWriter::writeUInt (AsciiKeyword keyword, Ulong val) {
    writer.writeKeyword(keyword);
    writer.writeUnsignedInteger(val);
}


inline void
AsciiRecordWriter::writeSInt (long val) {
    writer.writeSignedInteger(val);
}


inline void
AsciiRecordWriter::writeSInt (AsciiKeyword keyword, long val) {
    writer.writeKeyword(keyword);
    writer.writeSignedInteger(val);
}


inline void
AsciiRecordWriter::writeUInt64 (Ullong val) {
    writer.writeUnsignedInteger64(val);
}


inline void
AsciiRecordWriter::writeSInt64 (llong val) {
    writer.writeSignedInteger64(val);
}


inline void
AsciiRecordWriter::writeReal (const Oreal& val) {
    writer.writeReal(val);
}


inline void
AsciiRecordWriter::writeReal (AsciiKeyword keyword, const Oreal& val) {
    writer.writeKeyword(keyword);
    writer.writeReal(val);
}


inline void
AsciiRecordWriter::writeString (const string& val) {
    writer.writeString(val);
}


inline void
AsciiRecordWriter::writeString (AsciiKeyword keyword, const string& val) {
    writer.writeKeyword(keyword);
    writer.writeString(val);
}


inline void
AsciiRecordWriter::writeInfoByte (int infoByte) {
    assert ((infoByte & ~0xff) == 0);
    writer.writeInfoByte(infoByte);
}


//----------------------------------------------------------------------
// START and END records


void
AsciiRecordWriter::writeStartRecord (StartRecord* recp)
{
    // Section 13:  START record
    // `1' version-string unit offset-flag [table-offsets]

    assert (recp->recID == RID_START);

    writeString(recp->version);
    writeReal(recp->unit);
    writeUInt(recp->offsetFlag);
    if (recp->offsetFlag == 0)
        writeTableOffsets(recp->offsets);
}



void
AsciiRecordWriter::writeEndRecord (const EndRecord* recp)
{
    // Section 14:  END record
    // `2' [table-offsets] padding-string validation-scheme
    //     [validation-signature]

    assert (recp->recID == RID_END);

    if (recp->offsetFlag != 0)
        writeTableOffsets(recp->offsets);
    writer.writeValidationScheme(recp->valScheme);
    if (recp->valScheme != Validation::None)
        writeUInt(recp->valSignature);
}



void
AsciiRecordWriter::writeTableOffsets (const TableOffsets& toffs)
{
    writeTableInfo(toffs.cellName);
    writeTableInfo(toffs.textString);
    writeTableInfo(toffs.propName);
    writeTableInfo(toffs.propString);
    writeTableInfo(toffs.layerName);
    writeTableInfo(toffs.xname);
}



void
AsciiRecordWriter::writeTableInfo (const TableOffsets::TableInfo& tinfo)
{
    writeUInt(tinfo.strict);
    writeUInt64(tinfo.offset);
}


//----------------------------------------------------------------------
// Name records


// writeNameRecord() is invoked for the variants of CELLNAME, TEXTSTRING,
// PROPNAME and PROPSTRING that contain only a name.

void
AsciiRecordWriter::writeNameRecord (const NameRecord* recp)
{
    assert (recp->recID == RID_CELLNAME
            ||  recp->recID == RID_TEXTSTRING
            ||  recp->recID == RID_PROPNAME
            ||  recp->recID == RID_PROPSTRING);

    writeString(recp->name);
    Ulong  refnum = implicitRefnums[recp->recID]++;
    writer.writeImplicitRefnum(refnum);
}



// writeNameRecordWithRefnum() is invoked for the variants of CELLNAME,
// TEXTSTRING, PROPNAME and PROPSTRING that contain both a name and a
// reference-number.

void
AsciiRecordWriter::writeNameRecordWithRefnum (const NameRecord* recp)
{
    assert (recp->recID == RID_CELLNAME_R
            ||  recp->recID == RID_TEXTSTRING_R
            ||  recp->recID == RID_PROPNAME_R
            ||  recp->recID == RID_PROPSTRING_R);

    writeString(recp->name);
    writeUInt(recp->refnum);
}



void
AsciiRecordWriter::writeLayerNameRecord (const LayerNameRecord* recp)
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
AsciiRecordWriter::writeInterval (const LayerNameRecord::RawInterval& ival)
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
AsciiRecordWriter::writeCellRecord (const CellRecord* recp)
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
AsciiRecordWriter::writePlacementRecord (const PlacementRecord* recp)
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
            writeUInt(KeyRefnum, recp->refnum);
        else
            writeString(KeyName, recp->name);
    }

    // Write the transform fields: mag, angle, and flip.  For
    // RID_PLACEMENT there is no mag and the two AA bits specify the
    // angle.  The int need not be converted to Oreal because
    // AsciiScanner accepts integers as reals.  The keyword 'flip' has
    // no associated value.

    if (recp->recID == RID_PLACEMENT) {
        int  angle = GetAngle(infoByte);
        if (angle != 0)
            writeSInt(KeyAngle, angle);
    } else {
        if (infoByte & MagBit)    writeReal(KeyMag,   recp->mag);
        if (infoByte & AngleBit)  writeReal(KeyAngle, recp->angle);
    }
    if (infoByte & FlipBit)  writeKeyword(KeyFlip);

    if (infoByte & XBit)     writeSInt(KeyX, recp->x);
    if (infoByte & YBit)     writeSInt(KeyY, recp->y);
    if (infoByte & RepBit)   writeRepetition(recp->rawrep);
}



void
AsciiRecordWriter::writeTextRecord (const TextRecord* recp)
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
            writeUInt(KeyRefnum, recp->refnum);
        else
            writeString(KeyString, recp->text);
    }
    if (infoByte & TextlayerBit) writeUInt(KeyTextlayer, recp->textlayer);
    if (infoByte & TexttypeBit)  writeUInt(KeyTexttype,  recp->texttype);
    if (infoByte & XBit)         writeSInt(KeyX, recp->x);
    if (infoByte & YBit)         writeSInt(KeyY, recp->y);
    if (infoByte & RepBit)       writeRepetition(recp->rawrep);
}



void
AsciiRecordWriter::writeRectangleRecord (const RectangleRecord* recp)
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

    if (infoByte & LayerBit)    writeUInt(KeyLayer,    recp->layer);
    if (infoByte & DatatypeBit) writeUInt(KeyDatatype, recp->datatype);
    if (infoByte & SquareBit)   writeKeyword(KeySquare);
    if (infoByte & WidthBit)    writeUInt(KeyWidth,    recp->width);
    if (infoByte & HeightBit)   writeUInt(KeyHeight,   recp->height);
    if (infoByte & XBit)        writeSInt(KeyX, recp->x);
    if (infoByte & YBit)        writeSInt(KeyY, recp->y);
    if (infoByte & RepBit)      writeRepetition(recp->rawrep);
}



void
AsciiRecordWriter::writePolygonRecord (const PolygonRecord* recp)
{
    // Section 26:  POLYGON Record
    // `21' polygon-info-byte [layer-number] [datatype-number]
    //      [point-list] [x] [y] [repetition]
    // polygon-info-byte ::= 00PXYRDL

    assert (recp->recID == RID_POLYGON);
    using namespace PolygonInfoByte;

    int  infoByte = recp->infoByte;
    writeInfoByte(infoByte);

    // Note that the point-list is printed after x and y.  The order of
    // fields does not matter in the ASCII file, and it is more readable
    // to have the variable-length fields at the end.

    if (infoByte & LayerBit)     writeUInt(KeyLayer,    recp->layer);
    if (infoByte & DatatypeBit)  writeUInt(KeyDatatype, recp->datatype);
    if (infoByte & XBit)         writeSInt(KeyX, recp->x);
    if (infoByte & YBit)         writeSInt(KeyY, recp->y);
    if (infoByte & PointListBit) writePointList(recp->ptlist);
    if (infoByte & RepBit)       writeRepetition(recp->rawrep);
}



void
AsciiRecordWriter::writePathRecord (const PathRecord* recp)
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

    if (infoByte & LayerBit)     writeUInt(KeyLayer,     recp->layer);
    if (infoByte & DatatypeBit)  writeUInt(KeyDatatype,  recp->datatype);
    if (infoByte & HalfwidthBit) writeUInt(KeyHalfwidth, recp->halfwidth);

    // Write the start and end extensions if they appear.  We don't
    // write the extension-scheme because the ASCII reader can compute
    // it from the extensions.  Some information is lost: we don't
    // distinguish between setting SS to FlushExtn and setting it to
    // ExplicitExtn with start-extension 0, and similarly for EE and
    // end-extension.  Preserving this information is not worth the
    // trouble.

    if (infoByte & ExtensionBit) {
        writeExtnField(KeyStartExtn,
                       GetStartScheme(recp->extnScheme), recp->startExtn);
        writeExtnField(KeyEndExtn,
                       GetEndScheme(recp->extnScheme), recp->endExtn);
    }
    if (infoByte & XBit)         writeSInt(KeyX, recp->x);
    if (infoByte & YBit)         writeSInt(KeyY, recp->y);
    if (infoByte & PointListBit) writePointList(recp->ptlist);
    if (infoByte & RepBit)       writeRepetition(recp->rawrep);
}



// writeExtnField -- write start-extension or end-extension in PATH record
//   keyword    field identifier: KeyStartExtn or KeyEndExtn
//   scheme     the value of the SS or EE bits from the extension-scheme.
//              Must be in the range 0..3.
//   extn       the startExtn or endExtn field from the PathRecord.
//              Its value needs to be defined only if scheme == ExplicitExtn.

void
AsciiRecordWriter::writeExtnField (AsciiKeyword keyword,
                                   Uint scheme, long extn)
{
    using namespace PathInfoByte;

    if (scheme == ReuseLastExtn)
        return;

    // Write the extension either as a signed-integer (0 for flush) or
    // as the keyword "halfwidth".

    writeKeyword(keyword);
    switch (scheme) {
        case FlushExtn:      writeSInt(0);                break;
        case HalfwidthExtn:  writeKeyword(KeyHalfwidth);  break;
        case ExplicitExtn:   writeSInt(extn);             break;
        default:
            assert (false);
    }
}



void
AsciiRecordWriter::writeTrapezoidRecord (const TrapezoidRecord* recp)
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

    if (infoByte & LayerBit)    writeUInt(KeyLayer,    recp->layer);
    if (infoByte & DatatypeBit) writeUInt(KeyDatatype, recp->datatype);

    writeKeyword((infoByte & VerticalBit) ? KeyVertical : KeyHorizontal);

    if (infoByte & WidthBit)    writeUInt(KeyWidth,    recp->width);
    if (infoByte & HeightBit)   writeUInt(KeyHeight,   recp->height);

    if (recp->recID == RID_TRAPEZOID  ||  recp->recID == RID_TRAPEZOID_A) {
        writeKeyword(KeyDelta_A);
        writer.writeOneDelta(recp->delta_a);
    }
    if (recp->recID == RID_TRAPEZOID  ||  recp->recID == RID_TRAPEZOID_B) {
        writeKeyword(KeyDelta_B);
        writer.writeOneDelta(recp->delta_b);
    }

    if (infoByte & XBit)        writeSInt(KeyX, recp->x);
    if (infoByte & YBit)        writeSInt(KeyY, recp->y);
    if (infoByte & RepBit)      writeRepetition(recp->rawrep);
}



void
AsciiRecordWriter::writeCTrapezoidRecord (const CTrapezoidRecord* recp)
{
    // Section 29:  CTRAPEZOID record
    // `26' ctrapezoid-info-byte [layer-number] [datatype-number]
    //      [ctrapezoid-type] [width] [height] [x] [y] [repetition]
    // ctrapezoid-info-byte ::= TWHXYRDL

    assert (recp->recID == RID_CTRAPEZOID);
    using namespace CTrapezoidInfoByte;

    int  infoByte = recp->infoByte;
    writeInfoByte(infoByte);

    if (infoByte & LayerBit)    writeUInt(KeyLayer,    recp->layer);
    if (infoByte & DatatypeBit) writeUInt(KeyDatatype, recp->datatype);
    if (infoByte & TrapTypeBit) writeUInt(KeyTrapType, recp->ctrapType);
    if (infoByte & WidthBit)    writeUInt(KeyWidth,    recp->width);
    if (infoByte & HeightBit)   writeUInt(KeyHeight,   recp->height);
    if (infoByte & XBit)        writeSInt(KeyX,        recp->x);
    if (infoByte & YBit)        writeSInt(KeyY,        recp->y);
    if (infoByte & RepBit)      writeRepetition(recp->rawrep);
}



void
AsciiRecordWriter::writeCircleRecord (const CircleRecord* recp)
{
    // Section 30:  CIRCLE record
    // `27' circle-info-byte [layer-number] [datatype-number]
    //      [radius] [x] [y] [repetition]
    // circle-info-byte ::= 00rXYRDL   (r is radius, R is repetition)

    assert (recp->recID == RID_CIRCLE);
    using namespace CircleInfoByte;

    int  infoByte = recp->infoByte;
    writeInfoByte(infoByte);

    if (infoByte & LayerBit)    writeUInt(KeyLayer,    recp->layer);
    if (infoByte & DatatypeBit) writeUInt(KeyDatatype, recp->datatype);
    if (infoByte & RadiusBit)   writeUInt(KeyRadius,   recp->radius);
    if (infoByte & XBit)        writeSInt(KeyX,        recp->x);
    if (infoByte & YBit)        writeSInt(KeyY,        recp->y);
    if (infoByte & RepBit)      writeRepetition(recp->rawrep);
}



//----------------------------------------------------------------------
// Properties


void
AsciiRecordWriter::writePropertyRecord (const PropertyRecord* recp)
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
            writeUInt(KeyRefnum, recp->refnum);
        else
            writeString(KeyName, recp->name);
    }

    if (infoByte & StandardBit)
        writeKeyword(KeyStandard);

    // If the V bit is set, the previous PROPERTY record's values
    // should be reused.

    if (infoByte & ReuseValueBit)
        return;

    // Otherwise write the number of values, which is always present in
    // the ASCII file, and then each value.

    writeUInt(KeyValues, recp->values.size());
    PropValueVector::const_iterator  iter = recp->values.begin();
    PropValueVector::const_iterator  end  = recp->values.end();
    for ( ;  iter != end;  ++iter)
        writePropertyValue(*iter);
}



void
AsciiRecordWriter::writePropertyValue (const PropValue* propval)
{
    // Write the value type (an unsigned-integer) and then the value.

    // XXX: writeSpacer();
    writeUInt(propval->getType());
    switch (propval->getType()) {
        case PV_Real_PosInteger:
        case PV_Real_NegInteger:
        case PV_Real_PosReciprocal:
        case PV_Real_NegReciprocal:
        case PV_Real_PosRatio:
        case PV_Real_NegRatio:
        case PV_Real_Float32:
        case PV_Real_Float64:
            writeReal(propval->getRealValue());
            break;

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
AsciiRecordWriter::writeXNameRecord (const XNameRecord* recp)
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
AsciiRecordWriter::writeXElementRecord (const XElementRecord* recp)
{
    // Section 33:  XELEMENT record
    // `32' xelement-attribute xelement-string

    assert (recp->recID == RID_XELEMENT);

    writeUInt(recp->attribute);
    writeString(recp->data);
}



void
AsciiRecordWriter::writeXGeometryRecord (const XGeometryRecord* recp)
{
    // Section 34:  XGEOMETRY record
    // `33' xgeometry-info-byte xgeometry-attribute [layer-number]
    //      [datatype-number] xgeometry-string [x] [y] [repetition]
    // xgeometry-info-byte ::= 000XYRDL

    assert (recp->recID == RID_XGEOMETRY);
    using namespace XGeometryInfoByte;

    int  infoByte = recp->infoByte;
    writeInfoByte(recp->infoByte);

    writeUInt(KeyAttribute, recp->attribute);
    if (infoByte & LayerBit)    writeUInt(KeyLayer,    recp->layer);
    if (infoByte & DatatypeBit) writeUInt(KeyDatatype, recp->datatype);
    writeString(KeyString, recp->data);
    if (infoByte & XBit)        writeSInt(KeyX, recp->x);
    if (infoByte & YBit)        writeSInt(KeyY, recp->y);
    if (infoByte & RepBit)      writeRepetition(recp->rawrep);
}



void
AsciiRecordWriter::writeCblockRecord (const CblockRecord* recp)
{
    // Section 35:  CBLOCK record
    // `34' comp-type uncomp-byte-count comp-byte-count comp-bytes

    assert (recp->recID == RID_CBLOCK);
    writeUInt(recp->compType);
    writeUInt(recp->uncompBytes);
    writeUInt(recp->compBytes);
}


//----------------------------------------------------------------------
// Repetition and point-list


// writeRepetition -- write the repetition field of element records

void
AsciiRecordWriter::writeRepetition (const RawRepetition& rawrep)
{
    writeKeyword(KeyRepetition);

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
AsciiRecordWriter::writePointList (const PointList& ptlist)
{
    writeKeyword(KeyPointList);

    // A point-list begins with an unsigned-integer denoting its type.
    // After that is the number of deltas appearing in the file.

    writeUInt(ptlist.getType());
    writeUInt(ptlist.size());

    // Then come the deltas for the points.  The form of the delta (1-delta,
    // 2-delta, etc.) depends on the list type.

    PointList::const_iterator  iter = ptlist.begin();
    PointList::const_iterator  end  = ptlist.end();

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


} // namespace Oasis
