// oasis/creator.cc -- high-level interface for writing OASIS files
//
// last modified:   01-Jan-2010  Fri  21:55
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <algorithm>    // for min()
#include <vector>

#include "misc/arith.h"
#include "misc/utils.h"
#include "creator.h"
#include "infobyte.h"


namespace Oasis {

using namespace std;
using namespace SoftJin;


//======================================================================
//                              Name Tables
//======================================================================

// OasisCreator uses NameTables to store whatever it will need when
// writing the name tables in the OASIS file.  It is more convenient
// to define a separate hierarchy of lightweight classes here than to
// reuse the OasisDict class hierarchy that the parser uses.
// The class hierarchy:
//
//     NameTable
//         RefNameTable
//         LayerNameTable
//         XNameTable
//
// RefNameTable is used for CellName, TextString, PropName, and PropString.


// NameTable -- base class for all name tables
//
// fileOffset   off_t
//
//      File offset of first byte of name table.  OasisCreator sets the
//      value when it starts writing the table and gets the value when
//      it writes the table-offsets field of the END record
//
// strict       bool
//
//      Whether the table is strict.  Since OasisCreator anyway writes
//      all the <name> records of each type together, this flag just
//      records whether OasisCreator wrote a record with a name instead
//      of a refnum.  It is initially true.  OasisCreator sets the flag
//      to false when the application passes an unregistered OasisName
//      to one of its beginFoo() methods.  As with fileOffset,
//      OasisCreator uses the value when writing the END record.


class OasisCreator::NameTable {
    off_t       fileOffset;
    bool        strict;

protected:
                NameTable() {
                    fileOffset = 0;     // offset 0 means no table
                    strict = true;
                }
public:
    void        notStrict();
    void        setOffset (off_t offset);

    Ulong       getStrict() const   { return strict;     }
    off_t       getOffset() const   { return fileOffset; }

private:
                NameTable (const NameTable&);           // forbidden
    void        operator= (const NameTable&);           // forbidden
};



// notStrict -- mark table as being not strict.
// Note that there is no way to set it back to being strict.
//
// OasisCreator calls this when it writes a record that uses an
// unregistered name.  For example, when it writes a CELL record with an
// unregistered CellName, it cannot use a refnum and so it must write
// the cellname-string form of the record, which is forbidden if the
// cellname table is strict.
//
// OasisCreator also calls this when it writes a name immediately on
// registration (immediateNames is true)

inline void
OasisCreator::NameTable::notStrict() {
    strict = false;
}



// setOffset -- set the file offset of the name table
// OasisCreator calls this to save the offset when it begins writing the
// table.  It will need the offsets when it writes the table-offsets
// field in the END record.

inline void
OasisCreator::NameTable::setOffset (off_t offset) {
    assert (offset > 0);
    fileOffset = offset;
}


//----------------------------------------------------------------------


// RefNameTable -- store registered OasisNames and give each a refnum.
// This is for CellName, TextString, PropName, and PropString.  Each
// name added to the table is associated with a reference-number.
// Refnums start from 0 and are allocated sequentially.  The iterator is
// guaranteed to return the names in increasing order of refnums.  Hence
// for the <name> records we can use the version that contains just the
// name.

class OasisCreator::RefNameTable : public OasisCreator::NameTable {
    typedef HashMap<OasisName*, Ulong, HashPointer>    NameToRefnumMap;

    vector<OasisName*>  allNames;       // all names added to table
    NameToRefnumMap     nameToRefnum;   // get refnum given name
    Ulong               nextRefnum;     // refnum to give to next named added

public:
                RefNameTable()  { nextRefnum = 0; }
                ~RefNameTable() { }

    void        registerName (OasisName* oname);
    bool        getRefnum (OasisName* oname, /*out*/ Ulong* refnump) const;
    bool        hasRefnum (OasisName* oname) const;
    bool        empty() const;

    typedef vector<OasisName*>::iterator        iterator;
    typedef vector<OasisName*>::const_iterator  const_iterator;

    iterator            begin()       { return allNames.begin(); }
    const_iterator      begin() const { return allNames.begin(); }
    iterator            end()         { return allNames.end();   }
    const_iterator      end()   const { return allNames.end();   }
};



// registerName -- add name to table and give it a refnum
// The argument is not copied.  The caller should preserve it until
// OasisCreator finishes writing all the names.

inline void
OasisCreator::RefNameTable::registerName (OasisName* oname)
{
    allNames.push_back(oname);
    nameToRefnum[oname] = nextRefnum;
    ++nextRefnum;
}



// getRefnum -- get the reference-number for an OasisName, if any
//   oname      the OasisName to look up
//   refnump    out: the refnum for oname is stored here
// If oname has been inserted in the table, getRefnum() stores the
// associated refnum in *refnump and returns true.  Otherwise it returns
// false.

bool
OasisCreator::RefNameTable::getRefnum (OasisName* oname,
                                       /*out*/ Ulong* refnump) const
{
    NameToRefnumMap::const_iterator  iter = nameToRefnum.find(oname);
    if (iter == nameToRefnum.end())
        return false;
    *refnump = iter->second;
    return true;
}



// hasRefnum -- see if a name has been registered in the table

inline bool
OasisCreator::RefNameTable::hasRefnum (OasisName* oname) const {
    return (nameToRefnum.find(oname) != nameToRefnum.end());
}



inline bool
OasisCreator::RefNameTable::empty() const {
    return allNames.empty();
}


//----------------------------------------------------------------------


// Because LayerNames do not have refnums, LayerNameTable just has to
// store the names that are registered and iterate through them.


class OasisCreator::LayerNameTable : public OasisCreator::NameTable {
    vector<LayerName*>  allNames;
public:
    void        registerName (LayerName* layerName);
    bool        empty() const;

    typedef vector<LayerName*>::iterator        iterator;
    typedef vector<LayerName*>::const_iterator  const_iterator;

    iterator            begin()       { return allNames.begin(); }
    const_iterator      begin() const { return allNames.begin(); }
    iterator            end()         { return allNames.end();   }
    const_iterator      end()   const { return allNames.end();   }
};


inline void
OasisCreator::LayerNameTable::registerName (LayerName* layerName) {
    allNames.push_back(layerName);
}


inline bool
OasisCreator::LayerNameTable::empty() const {
    return allNames.empty();
}


//----------------------------------------------------------------------


// XNameTable, like LayerNameTable, just stores the names and iterates
// through them.  Although XNames have refnums, assigning them is the
// application's responsibility.  We don't have to do anything here.


class OasisCreator::XNameTable : public OasisCreator::NameTable {
    vector<XName*>      allNames;
public:
    void        registerName (XName* xname);
    bool        empty() const;

    typedef vector<XName*>::iterator            iterator;
    typedef vector<XName*>::const_iterator      const_iterator;

    iterator            begin()       { return allNames.begin(); }
    const_iterator      begin() const { return allNames.begin(); }
    iterator            end()         { return allNames.end();   }
    const_iterator      end()   const { return allNames.end();   }
};



inline void
OasisCreator::XNameTable::registerName (XName* xname) {
    allNames.push_back(xname);
}


inline bool
OasisCreator::XNameTable::empty() const {
    return allNames.empty();
}



//======================================================================
//                              OasisCreator
//======================================================================

// OasisCreator constructor
//   fname      pathname of file to create
//   options    file construction options

OasisCreator::OasisCreator (const char* fname,
                            const OasisCreatorOptions& options)
  : writer(fname),
    s_cell_offset(S_CELL_OFFSET),
    options(options),

    cellNameTable(new CellNameTable),
    textStringTable(new TextStringTable),
    propNameTable(new PropNameTable),
    propStringTable(new PropStringTable),
    layerNameTable(new LayerNameTable),
    xnameTable(new XNameTable)
{
    mustCompress = true;                // default is to compress
    nowCompressing = false;
    repReuse.makeReuse();
    cellOffsetPropName = Null;
    deletePropName = false;
}



OasisCreator::~OasisCreator()
{
    if (deletePropName)
        delete cellOffsetPropName;
}


//----------------------------------------------------------------------
// Some inline functions to shorten the code

inline void
OasisCreator::writeSInt (long val) {
    writer.writeSignedInteger(val);
}

inline void
OasisCreator::writeUInt (Ulong val) {
    writer.writeUnsignedInteger(val);
}

inline void
OasisCreator::writeSInt64 (llong val) {
    writer.writeSignedInteger64(val);
}

inline void
OasisCreator::writeUInt64 (Ullong val) {
    writer.writeUnsignedInteger64(val);
}

inline void
OasisCreator::writeReal (const Oreal& val) {
    writer.writeReal(val);
}

inline void
OasisCreator::writeString (const string& val) {
    writer.writeString(val);
}

inline void
OasisCreator::beginRecord (RecordID recID) {
    writer.beginRecord(recID);
}


inline void
OasisCreator::writeInfoByte (int infoByte) {
    assert ((infoByte & ~0xff) == 0);
    writer.writeByte(infoByte);
}


// beginBlock() is invoked immediately after writing the CELL record,
// and just before writing a name table.  endBlock() is invoked at the
// end of each cell and name table.  They turn compression on and off
// if the application has asked for it.  Compression must be turned off
// at the end of each cell and name table so that the next one will not
// begin in the middle of a cblock.
//
// Spec 35.4 forbids CELL records from appearing in a cblock.
// Spec 13.9 forbids strict-mode name tables from starting in the middle
// of a cblock.  We can start non-strict name tables in the middle of a
// cblock and forget about putting their offsets in the END record, but
// there is no point in that.


void
OasisCreator::beginBlock()
{
    if (mustCompress) {
        writer.beginCblock(DeflateCompression);
        nowCompressing = true;
    }
}


void
OasisCreator::endBlock()
{
    if (nowCompressing) {
        writer.endCblock();
        nowCompressing = false;
    }
}


//----------------------------------------------------------------------

// beginFile -- open file for writing and write START record
//   version    OASIS file version (should currently be "1.0")
//   unit       database unit for START record
//   valScheme  what kind of validation signature to compute:
//              Validation::None, Validation::Checksum32, or Validation::CRC32

/*virtual*/ void
OasisCreator::beginFile (const string&  version,
                         const Oreal&  unit,
                         Validation::Scheme  valScheme)
{
    writer.beginFile(valScheme);
    writer.writeBytes(OasisMagic, OasisMagicLength);

    // Section 13:  START record
    // `1' version-string unit offset-flag [table-offsets]
    //
    // Write 1 for offset-flag because we always put the table-offsets
    // in the END record.

    beginRecord(RID_START);
    writeString(version);
    writeReal(unit);
    writeUInt(1);
}



/*virtual*/ void
OasisCreator::endFile()
{
    writeNameTables();

    // Section 14:  END record
    // `2' [table-offsets] padding-string validation-scheme
    //     [validation-signature]

    // Table 11 in the spec gives the order of the entries.

    beginRecord(RID_END);
    writeTableInfo(cellNameTable.get());
    writeTableInfo(textStringTable.get());
    writeTableInfo(propNameTable.get());
    writeTableInfo(propStringTable.get());
    writeTableInfo(layerNameTable.get());
    writeTableInfo(xnameTable.get());

    // This writes the padding string and validation.
    writer.endFile();
}



void
OasisCreator::writeTableInfo (const NameTable* ntab)
{
    // For each table, the first integer is 1 if the table is strict and
    // 0 otherwise.  The second integer is the file offset of the table;
    // 0 if the table is absent.

    writeUInt(ntab->getStrict());
    writeUInt64(ntab->getOffset());
}



/*virtual*/ void
OasisCreator::beginCell (CellName* cellName)
{
    // Save the cell's offset for use when writing the cellName's
    // S_CELL_OFFSET property.

    cellOffsets[cellName] = writer.currFileOffset();

    // If the cellName has been registered, use the refnum form of the
    // CELL record.  Otherwise use the name form and mark the table as
    // being non-strict.

    Ulong  refnum;
    if (cellNameTable->getRefnum(cellName, &refnum)) {
        beginRecord(RID_CELL_REF);
        writeUInt(refnum);
    } else {
        beginRecord(RID_CELL_NAMED);
        writeString(cellName->getName());
        cellNameTable->notStrict();
    }

    // Begin compression if requested in the constructor and reset all
    // modal variables to the uninitialized or default state.

    beginBlock();
    modvars.reset();            // 10.1
}



/*virtual*/ void
OasisCreator::endCell()
{
    // If compressing, stop.
    endBlock();
}



// setXYrelative -- change the xy-mode for the rest of the cell

void
OasisCreator::setXYrelative (bool flag)
{
    if (flag != modvars.xyIsRelative()) {
        modvars.xySetRelative(flag);
        beginRecord(flag ? RID_XYRELATIVE : RID_XYABSOLUTE);
    }
}



// setCompression -- should cell contents and name tables be compressed?
// They are compressed by default.  The new setting takes effect with
// the next cell or name table.

void
OasisCreator::setCompression (bool flag)
{
    mustCompress = flag;
}



//----------------------------------------------------------------------
//                              Elements
//----------------------------------------------------------------------

/*virtual*/ void
OasisCreator::beginPlacement (CellName*  cellName,
                              long x, long y,
                              const Oreal&  mag,
                              const Oreal&  angle,
                              bool  flip,
                              const Repetition*  rep)
{
    // Section 22:  PLACEMENT record
    // `17' placement-info-byte [reference-number | cellname-string]
    //      [x] [y] [repetition]
    // placement-info-byte ::= CNXYRAAF
    //
    // `18' placement-info-byte [reference-number | cellname-string]
    //      [magnification] [angle] [x] [y] [repetition]
    // placement-info-byte ::= CNXYRMAF

    using namespace PlacementInfoByte;

    int         infoByte = 0;
    RecordID    recID = RID_PLACEMENT;

    // If magnification is 1 and angle is one of (0,90,180,270), use
    // record type 17 (RID_PLACEMENT) and set the AA bits.  Otherwise use
    // record type 18 (RID_PLACEMENT_TRANSFORM) and set the M and A bits.

    int  angleBits = 0;         // AA bits for type 17 or A bit for type 18
    double  angleValue = angle.getValue();
    if (mag.getValue() == 1.0) {
        if (angleValue == 0.0)         angleBits = AngleZero;
        else if (angleValue == 90.0)   angleBits = Angle90;
        else if (angleValue == 180.0)  angleBits = Angle180;
        else if (angleValue == 270.0)  angleBits = Angle270;
        else {
            recID = RID_PLACEMENT_TRANSFORM;
            angleBits = AngleBit;
        }
    } else {
        recID = RID_PLACEMENT_TRANSFORM;
        infoByte |= MagBit;
        if (angleValue != 0.0)
            angleBits = AngleBit;
    }
    infoByte |= angleBits;

    // Set the C and N bits and update placement-cell.
    // C = 1 => a cell name or reference is present.  C = 0 => use the same
    // cell as in the modal state.  If C = 1, then N = 0 means a name
    // appears and N = 1 means a reference-number appears.

    // XXX: Because the cellName is stored in modvars, all cellNames
    // passed in must be permanent.

    Ulong  refnum;
    setPlacementCell (&infoByte, CellExplicitBit, cellName);
    if (infoByte & CellExplicitBit) {
        if (cellNameTable->getRefnum(cellName, &refnum))
            infoByte |= RefnumBit;
        else
            cellNameTable->notStrict();
    }

    // setPlacementX() and setPlacementY() relativize x and y if they
    // must be written and xy-mode is relative.

    setPlacementX (&infoByte, XBit, &x);
    setPlacementY (&infoByte, YBit, &y);
    setRepetition (&infoByte, RepBit, &rep);
    if (flip)
        infoByte |= FlipBit;

    // Write the fields.

    beginRecord(recID);
    writeInfoByte(infoByte);
    if (infoByte & CellExplicitBit) {
        if (infoByte & RefnumBit)
            writeUInt(refnum);
        else
            writeString(cellName->getName());
    }
    if (recID == RID_PLACEMENT_TRANSFORM) {
        if (infoByte & MagBit)    writeReal(mag);
        if (infoByte & AngleBit)  writeReal(angle);
    }
    if (infoByte & XBit)    writeSInt(x);
    if (infoByte & YBit)    writeSInt(y);
    if (infoByte & RepBit)  writeRepetition(rep);
}



void
OasisCreator::beginText (Ulong textlayer, Ulong texttype,
                         long x, long y,
                         TextString*  textString,
                         const Repetition*  rep)
{
    // Section 24:  TEXT record
    // `19' text-info-byte [reference-number | text-string]
    //      [textlayer-number] [texttype-number] [x] [y] [repetition]
    // text-info-byte ::= 0CNXYRTL

    using namespace TextInfoByte;

    // Decide which fields to write.  Because setTextString() might
    // store a pointer to textString in the modal variable, the caller
    // must not delete textString, even if it is unregistered.

    int  infoByte = 0;
    setTextString (&infoByte, TextExplicitBit, textString);
    setTextlayer  (&infoByte, TextlayerBit,    textlayer);
    setTexttype   (&infoByte, TexttypeBit,     texttype);
    setTextX      (&infoByte, XBit,            &x);
    setTextY      (&infoByte, YBit,            &y);
    setRepetition (&infoByte, RepBit,          &rep);

    // If the string was registered earlier, set the N bit and get
    // the string's reference-number.

    Ulong  refnum;
    if (infoByte & TextExplicitBit) {
        if (textStringTable->getRefnum(textString, &refnum))
            infoByte |= RefnumBit;
        else
            textStringTable->notStrict();
    }

    beginRecord(RID_TEXT);
    writeInfoByte(infoByte);
    if (infoByte & TextExplicitBit) {
        if (infoByte & RefnumBit)
            writeUInt(refnum);
        else
            writeString(textString->getName());
    }
    if (infoByte & TextlayerBit)   writeUInt(textlayer);
    if (infoByte & TexttypeBit)    writeUInt(texttype);
    if (infoByte & XBit)           writeSInt(x);
    if (infoByte & YBit)           writeSInt(y);
    if (infoByte & RepBit)         writeRepetition(rep);
}



/*virtual*/ void
OasisCreator::beginRectangle (Ulong layer, Ulong datatype,
                              long x, long y,
                              long width, long height,
                              const Repetition* rep)
{
    // Section 25:  RECTANGLE record
    // `20' rectangle-info-byte [layer-number] [datatype-number]
    //      [width] [height] [x] [y] [repetition]
    // rectangle-info-byte ::= SWHXYRDL
    // If S = 1, the rectangle is a square, and H must be 0.

    using namespace RectangleInfoByte;

    int  infoByte = 0;
    setLayer      (&infoByte, LayerBit,    layer);
    setDatatype   (&infoByte, DatatypeBit, datatype);
    setGeometryX  (&infoByte, XBit,        &x);
    setGeometryY  (&infoByte, YBit,        &y);
    setRepetition (&infoByte, RepBit,      &rep);

    setGeometryWidth (&infoByte, WidthBit, width);
    if (width == height) {
        infoByte |= SquareBit;
        modvars.setGeometryHeight(height);
    } else {
        setGeometryHeight (&infoByte, HeightBit, height);
    }

    beginRecord(RID_RECTANGLE);
    writeInfoByte(infoByte);
    if (infoByte & LayerBit)     writeUInt(layer);
    if (infoByte & DatatypeBit)  writeUInt(datatype);
    if (infoByte & WidthBit)     writeUInt(width);
    if (infoByte & HeightBit)    writeUInt(height);
    if (infoByte & XBit)         writeSInt(x);
    if (infoByte & YBit)         writeSInt(y);
    if (infoByte & RepBit)       writeRepetition(rep);
}



/*virtual*/ void
OasisCreator::beginPolygon (Ulong layer, Ulong datatype,
                            long x, long y,
                            const PointList& ptlist,
                            const Repetition* rep)
{
    // Section 26:  POLYGON Record
    // `21' polygon-info-byte [layer-number] [datatype-number]
    //      [point-list] [x] [y] [repetition]
    // polygon-info-byte ::= 00PXYRDL

    using namespace PolygonInfoByte;

    int  infoByte = 0;
    setLayer         (&infoByte, LayerBit,     layer);
    setDatatype      (&infoByte, DatatypeBit,  datatype);
    setPolygonPoints (&infoByte, PointListBit, ptlist);
    setGeometryX     (&infoByte, XBit,         &x);
    setGeometryY     (&infoByte, YBit,         &y);
    setRepetition    (&infoByte, RepBit,       &rep);

    beginRecord(RID_POLYGON);
    writeInfoByte(infoByte);
    if (infoByte & LayerBit)     writeUInt(layer);
    if (infoByte & DatatypeBit)  writeUInt(datatype);
    if (infoByte & PointListBit) writePointList(ptlist, true);
    if (infoByte & XBit)         writeSInt(x);
    if (infoByte & YBit)         writeSInt(y);
    if (infoByte & RepBit)       writeRepetition(rep);
}



/*virtual*/ void
OasisCreator::beginPath (Ulong layer, Ulong datatype,
                         long x, long  y,
                         long halfwidth,
                         long startExtn, long endExtn,
                         const PointList&  ptlist,
                         const Repetition* rep)
{
    // Section 27:  PATH record
    // `22' path-info-byte [layer-number] [datatype-number] [half-width]
    //      [ extension-scheme [start-extension] [end-extension] ]
    //      [point-list] [x] [y] [repetition]
    // path-info-byte ::= EWPXYRDL

    using namespace PathInfoByte;

    int  infoByte = 0;
    setLayer         (&infoByte, LayerBit,     layer);
    setDatatype      (&infoByte, DatatypeBit,  datatype);
    setPathHalfwidth (&infoByte, HalfwidthBit, halfwidth);
    setPathPoints    (&infoByte, PointListBit, ptlist);
    setGeometryX     (&infoByte, XBit,         &x);
    setGeometryY     (&infoByte, YBit,         &y);
    setRepetition    (&infoByte, RepBit,       &rep);

    Ulong  extnScheme = 0;
    setPathStartExtnScheme (&extnScheme, startExtn, halfwidth);
    setPathEndExtnScheme   (&extnScheme, endExtn,   halfwidth);
    if (extnScheme != 0)
        infoByte |= ExtensionBit;

    beginRecord(RID_PATH);
    writeInfoByte(infoByte);
    if (infoByte & LayerBit)     writeUInt(layer);
    if (infoByte & DatatypeBit)  writeUInt(datatype);
    if (infoByte & HalfwidthBit) writeUInt(halfwidth);

    if (infoByte & ExtensionBit) {
        writeUInt(extnScheme);
        if (GetStartScheme(extnScheme) == ExplicitExtn)  writeSInt(startExtn);
        if (GetEndScheme(extnScheme)   == ExplicitExtn)  writeSInt(endExtn);
    }

    if (infoByte & PointListBit) writePointList(ptlist, false);
    if (infoByte & XBit)         writeSInt(x);
    if (infoByte & YBit)         writeSInt(y);
    if (infoByte & RepBit)       writeRepetition(rep);
}



/*virtual*/ void
OasisCreator::beginTrapezoid (Ulong layer, Ulong datatype,
                              long x, long  y,
                              const Trapezoid& trap,
                              const Repetition* rep)
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
    //
    // Section 29:  CTRAPEZOID record
    // `26' ctrapezoid-info-byte [layer-number] [datatype-number]
    //      [ctrapezoid-type] [width] [height] [x] [y] [repetition]
    // ctrapezoid-info-byte ::= TWHXYRDL

    // The two info-bytes differ only in the first bit; so import only
    // the T bit's name (TrapTypeBit) from CTrapezoidInfoByte.

    using namespace TrapezoidInfoByte;
    using CTrapezoidInfoByte::TrapTypeBit;

    Ulong       width  = trap.getWidth();
    Ulong       height = trap.getHeight();
    long        delta_a = 0;            // only for uncompressed trapezoids
    long        delta_b = 0;            // only for uncompressed trapezoids
    Uint        ctrapType = 0;          // only for compressed trapezoids
    int         infoByte = 0;
    RecordID    recID;

    setLayer          (&infoByte, LayerBit,    layer);
    setDatatype       (&infoByte, DatatypeBit, datatype);
    setGeometryWidth  (&infoByte, WidthBit,    width);
    setGeometryHeight (&infoByte, HeightBit,   height);
    setGeometryX      (&infoByte, XBit,        &x);
    setGeometryY      (&infoByte, YBit,        &y);
    setRepetition     (&infoByte, RepBit,      &rep);

    if (trap.isCompressed()) {
        recID = RID_CTRAPEZOID;
        ctrapType = trap.getCompressType();
        setCTrapezoidType (&infoByte, TrapTypeBit, ctrapType);
        if (! Trapezoid::needWidth(ctrapType))   infoByte &= ~WidthBit;
        if (! Trapezoid::needHeight(ctrapType))  infoByte &= ~HeightBit;
    }
    else {
        delta_a = trap.getDelta_A();
        delta_b = trap.getDelta_B();

        if (delta_a == 0)
            recID = RID_TRAPEZOID_B;
        else if (delta_b == 0)
            recID = RID_TRAPEZOID_A;
        else
            recID = RID_TRAPEZOID;

        if (trap.getOrientation() == Trapezoid::Vertical)
            infoByte |= VerticalBit;
    }

    beginRecord(recID);
    writeInfoByte(infoByte);
    if (infoByte & LayerBit)     writeUInt(layer);
    if (infoByte & DatatypeBit)  writeUInt(datatype);

    if (recID == RID_CTRAPEZOID  &&  (infoByte & TrapTypeBit))
        writeUInt(ctrapType);

    if (infoByte & WidthBit)     writeUInt(width);
    if (infoByte & HeightBit)    writeUInt(height);

    if (recID != RID_CTRAPEZOID) {
        if (recID != RID_TRAPEZOID_B)  writeSInt(delta_a);
        if (recID != RID_TRAPEZOID_A)  writeSInt(delta_b);
    }

    if (infoByte & XBit)         writeSInt(x);
    if (infoByte & YBit)         writeSInt(y);
    if (infoByte & RepBit)       writeRepetition(rep);
}



/*virtual*/ void
OasisCreator::beginCircle (Ulong layer, Ulong datatype,
                           long x, long y,
                           long radius,
                           const Repetition* rep)
{
    // Section 30:  CIRCLE record
    // `27' circle-info-byte [layer-number] [datatype-number]
    //      [radius] [x] [y] [repetition]
    // circle-info-byte ::= 00rXYRDL   (r is radius, R is repetition)

    using namespace CircleInfoByte;

    int  infoByte = 0;
    setLayer      (&infoByte, LayerBit,    layer);
    setDatatype   (&infoByte, DatatypeBit, datatype);
    setRadius     (&infoByte, RadiusBit,   radius);
    setGeometryX  (&infoByte, XBit,        &x);
    setGeometryY  (&infoByte, YBit,        &y);
    setRepetition (&infoByte, RepBit,      &rep);

    beginRecord(RID_CIRCLE);
    writeInfoByte(infoByte);
    if (infoByte & LayerBit)     writeUInt(layer);
    if (infoByte & DatatypeBit)  writeUInt(datatype);
    if (infoByte & RadiusBit)    writeUInt(radius);
    if (infoByte & XBit)         writeSInt(x);
    if (infoByte & YBit)         writeSInt(y);
    if (infoByte & RepBit)       writeRepetition(rep);
}



/*virtual*/ void
OasisCreator::beginXElement (Ulong attribute, const string& data)
{
    // Section 33:  XELEMENT record
    // `32' xelement-attribute xelement-string

    beginRecord(RID_XELEMENT);
    writeUInt(attribute);
    writeString(data);
}



/*virtual*/ void
OasisCreator::beginXGeometry (Ulong layer, Ulong datatype,
                              long x, long y,
                              Ulong attribute,
                              const string& data,
                              const Repetition* rep)
{
    // Section 34:  XGEOMETRY record
    // `33' xgeometry-info-byte xgeometry-attribute [layer-number]
    //      [datatype-number] xgeometry-string [x] [y] [repetition]
    // xgeometry-info-byte ::= 000XYRDL

    using namespace XGeometryInfoByte;

    int  infoByte = 0;
    setLayer      (&infoByte, LayerBit,    layer);
    setDatatype   (&infoByte, DatatypeBit, datatype);
    setGeometryX  (&infoByte, XBit,        &x);
    setGeometryY  (&infoByte, YBit,        &y);
    setRepetition (&infoByte, RepBit,      &rep);

    beginRecord(RID_XGEOMETRY);
    writeInfoByte(infoByte);
    writeUInt(attribute);
    if (infoByte & LayerBit)     writeUInt(layer);
    if (infoByte & DatatypeBit)  writeUInt(datatype);
    writeString(data);
    if (infoByte & XBit)         writeSInt(x);
    if (infoByte & YBit)         writeSInt(y);
    if (infoByte & RepBit)       writeRepetition(rep);
}



/*virtual*/ void
OasisCreator::addCellProperty (Property* prop) {
    writeProperty(prop);
}

/*virtual*/ void
OasisCreator::addFileProperty (Property* prop) {
    writeProperty(prop);
}

/*virtual*/ void
OasisCreator::addElementProperty (Property* prop) {
    writeProperty(prop);
}



// Each registerFooName() saves a pointer to the name in the
// corresponding name table and assigns the name a refnum.
//
// When OasisCreator later writes a record that refers to the name, it
// uses the refnum instead.  If immediateNames is false and all names of
// a given type are registered before they are used, the name table for
// that type will be strict.
//
// If immediateNames is true, OasisCreator immediately writes the name
// record, and any associated PROPERTY records.  It also marks the
// corresponding name table non-strict because the name records may be
// scattered throughout the file.  If immediateNames is false,
// OasisCreator will write the name records in the name tables at the
// end of the file.
//
// The only names that _must_ be registered are PropStrings and names
// that have properties.  Other names _should_ be registered before use
// because then OasisCreator can make all name tables strict (assuming
// immediateNames is false).  If any name table is not strict, every
// program that reads the file using OasisParser will be slowed down
// because OasisParser will make a separate pass of the whole file to
// collect all names.


/*virtual*/ void
OasisCreator::registerCellName (CellName* cellName)
{
    cellNameTable->registerName(cellName);
    if (options.immediateNames) {
        writeCellName(cellName);
        cellNameTable->notStrict();
    }
}



/*virtual*/ void
OasisCreator::registerTextString (TextString* textString)
{
    textStringTable->registerName(textString);
    if (options.immediateNames) {
        writeTextString(textString);
        textStringTable->notStrict();
    }
}



/*virtual*/ void
OasisCreator::registerPropName (PropName* propName)
{
    propNameTable->registerName(propName);
    if (options.immediateNames) {
        writePropName(propName);
        propNameTable->notStrict();
    }
}



/*virtual*/ void
OasisCreator::registerPropString (PropString* propString)
{
    propStringTable->registerName(propString);
    if (options.immediateNames) {
        writePropString(propString);
        propStringTable->notStrict();
    }
}



/*virtual*/ void
OasisCreator::registerLayerName (LayerName* layerName)
{
    layerNameTable->registerName(layerName);
    if (options.immediateNames) {
        writeLayerName(layerName);
        layerNameTable->notStrict();
    }
}



/*virtual*/ void
OasisCreator::registerXName (XName* xname)
{
    xnameTable->registerName(xname);
    if (options.immediateNames) {
        writeXName(xname);
        xnameTable->notStrict();
    }
}


// End of public methods.  Everything below is private.

//----------------------------------------------------------------------
//                            Name Tables
//----------------------------------------------------------------------

void
OasisCreator::writeNameTables()
{
    // If immediateNames is true, each name record was written
    // immediately on registration.  There are no name tables to write.

    if (options.immediateNames)
        return;

    // Locate or create a PropName for S_CELL_OFFSET.  We shall need it
    // when writing CELLNAME records, but only if some cells were written.

    if (! cellOffsets.empty())
        makeCellOffsetPropName();

    if (! propNameTable->empty())    writePropNameTable();
    if (! propStringTable->empty())  writePropStringTable();
    if (! cellNameTable->empty())    writeCellNameTable();
    if (! textStringTable->empty())  writeTextStringTable();
    if (! layerNameTable->empty())   writeLayerNameTable();
    if (! xnameTable->empty())       writeXNameTable();
}



// makeCellOffsetPropName -- create PropName for S_CELL_OFFSET if needed.
// When writing the cellname table we need to write the S_CELL_OFFSET
// property for each cell in the file.  To save space we use a
// reference-number instead of a propname-string in the PROPERTY record.
//
// This function gets the propName to use for S_CELL_OFFSET, creating
// one if needed, and stores it in the member variable
// cellOffsetPropName.

void
OasisCreator::makeCellOffsetPropName()
{
    // See if there already exists a PropName object for S_CELL_OFFSET.

    PropNameTable::const_iterator  iter = propNameTable->begin();
    PropNameTable::const_iterator  end  = propNameTable->end();
    while (iter != end  &&  (*iter)->getName() != s_cell_offset)
        ++iter;

    // If yes, save it in the member variable.  Otherwise create a new
    // PropName object and register it so that it gets assigned a
    // refnum.

    if (iter != end)
        cellOffsetPropName = *iter;
    else {
        cellOffsetPropName = new PropName(s_cell_offset);
        deletePropName = true;
        propNameTable->registerName(cellOffsetPropName);
    }
}



void
OasisCreator::writeCellNameTable()
{
    cellNameTable->setOffset(writer.currFileOffset());
    beginBlock();

    CellNameTable::const_iterator  iter = cellNameTable->begin();
    CellNameTable::const_iterator  end  = cellNameTable->end();
    for ( ;  iter != end;  ++iter)
        writeCellName(*iter);

    endBlock();
}



void
OasisCreator::writeTextStringTable()
{
    textStringTable->setOffset(writer.currFileOffset());
    beginBlock();

    TextStringTable::const_iterator  iter = textStringTable->begin();
    TextStringTable::const_iterator  end  = textStringTable->end();
    for ( ;  iter != end;  ++iter)
        writeTextString(*iter);

    endBlock();
}



void
OasisCreator::writePropNameTable()
{
    propNameTable->setOffset(writer.currFileOffset());
    beginBlock();

    PropNameTable::const_iterator  iter = propNameTable->begin();
    PropNameTable::const_iterator  end  = propNameTable->end();
    for ( ;  iter != end;  ++iter)
        writePropName(*iter);

    endBlock();
}



void
OasisCreator::writePropStringTable()
{
    propStringTable->setOffset(writer.currFileOffset());
    beginBlock();

    PropStringTable::const_iterator  iter = propStringTable->begin();
    PropStringTable::const_iterator  end  = propStringTable->end();
    for ( ;  iter != end;  ++iter)
        writePropString(*iter);

    endBlock();
}



void
OasisCreator::writeLayerNameTable()
{
    layerNameTable->setOffset(writer.currFileOffset());
    beginBlock();

    LayerNameTable::const_iterator  iter = layerNameTable->begin();
    LayerNameTable::const_iterator  end  = layerNameTable->end();
    for ( ;  iter != end;  ++iter)
        writeLayerName(*iter);

    endBlock();
}



void
OasisCreator::writeXNameTable()
{
    xnameTable->setOffset(writer.currFileOffset());
    beginBlock();

    XNameTable::const_iterator  iter = xnameTable->begin();
    XNameTable::const_iterator  end  = xnameTable->end();
    for ( ;  iter != end;  ++iter)
        writeXName(*iter);

    endBlock();
}



//----------------------------------------------------------------------
//                              Names
//----------------------------------------------------------------------

// Each writeFooName() method below is called either from
//   - the corresponding registerFooName(), if immediateNames is true, or
//   - the corresponding writeFooNameTable() otherwise.
//
// writeCellName(), writeTextString(), writePropName(), and
// writePropString() use the auto-numbering form of the <name> record.
// The name tables must therefore
//   - assign consecutive refnums to names, in the order in which they
//     are registered and starting from 0; and
//   - iterate through the names in the same order.


void
OasisCreator::writeCellName (CellName* cellName)
{
    // Section 15:  CELLNAME record
    // `3' cellname-string
    // `4' cellname-string reference-number
    // We write only type-3 records.

    modvars.reset();

    // Write the CELLNAME record.
    beginRecord(RID_CELLNAME);
    writeString(cellName->getName());

    // Write the property records, but skip all standard S_CELL_OFFSET
    // properties because they will be garbage.  We write that property
    // using special code further down.

    MatchPropertyName  isCellOffsetProperty(s_cell_offset, true);
        // A predicate that matches all standard properties named
        // S_CELL_OFFSET.

    const PropertyList&  plist(cellName->getPropertyList());
    PropertyList::const_iterator  iter = plist.begin();
    PropertyList::const_iterator  end  = plist.end();
    for ( ;  iter != end;  ++iter) {
        if (! isCellOffsetProperty(*iter))
            writeProperty(*iter);
    }

    // If immediateNames is true, the CELLNAME records will be scattered
    // around the file.  There is no point in writing the S_CELL_OFFSET
    // properties then.  Also note that cellOffsetPropName is defined
    // only when immediateNames is false.

    if (options.immediateNames)
        return;

    // Write the property record for S_CELL_OFFSET.  If no cell was
    // written with this CellName, the cell is external, so put 0 as the
    // offset (spec A2-2.3).

    Ullong  offset = 0;
    CellOffsetMap::const_iterator  offIter = cellOffsets.find(cellName);
    if (offIter != cellOffsets.end())
        offset = offIter->second;

    Property  prop(cellOffsetPropName, true);
    prop.addValue(PV_UnsignedInteger, offset);
    writeProperty(&prop);
}



void
OasisCreator::writeTextString (TextString* textString)
{
    // Section 16:  TEXTSTRING record
    // `5' text-string
    // `6' text-string reference-number
    // We write only type-5 records.

    modvars.reset();
    beginRecord(RID_TEXTSTRING);
    writeString(textString->getName());
    writePropertyList(textString->getPropertyList());
}



void
OasisCreator::writePropName (PropName* propName)
{
    // Section 17:  PROPNAME record
    // `7' propname-string
    // `8' propname-string reference-number
    // We write only type-7 records.

    modvars.reset();
    beginRecord(RID_PROPNAME);
    writeString(propName->getName());
    writePropertyList(propName->getPropertyList());
}



void
OasisCreator::writePropString (PropString* propString)
{
    // Section 18:  PROPSTRING record
    // `9'  prop-string
    // `10' prop-string reference-number
    // We write only type-9 records

    modvars.reset();
    beginRecord(RID_PROPSTRING);
    writeString(propString->getName());
    writePropertyList(propString->getPropertyList());
}



void
OasisCreator::writeLayerName (LayerName* layerName)
{
    // Section 19:  LAYERNAME record
    // `11' layername-string layer-interval datatype-interval
    // `12' layername-string textlayer-interval texttype-interval

    modvars.reset();
    RecordID  recID;
    if (layerName->getIntervalType() == LayerName::GeometryInterval)
        recID = RID_LAYERNAME_GEOMETRY;
    else
        recID = RID_LAYERNAME_TEXT;

    beginRecord(recID);
    writeString(layerName->getName());
    writeInterval(layerName->getLayers());
    writeInterval(layerName->getTypes());
    writePropertyList(layerName->getPropertyList());
}



void
OasisCreator::writeInterval (const Interval& interval)
{
    Ulong  lower = interval.getLowerBound();
    Ulong  upper = interval.getUpperBound();

    if (interval.isUnbounded()) {
        if (lower == 0)
            writeUInt(0);               // type 0:  0..infinity
        else {
            writeUInt(2);               // type 2:  lower..infinity
            writeUInt(lower);
        }
    } else {
        if (lower == 0) {
            writeUInt(1);               // type 1:  0..upper
            writeUInt(upper);
        } else if (lower == upper) {
            writeUInt(3);               // type 3:  lower..lower
            writeUInt(lower);
        } else {
            writeUInt(4);               // type 4:  lower..upper
            writeUInt(lower);
            writeUInt(upper);
        }
    }
}



void
OasisCreator::writeXName (XName* xname)
{
    // Section 32:  XNAME record
    // `30' xname-attribute xname-string
    // `31' xname-attribute xname-string reference-number

    // We write only type-31 records, with the reference number.  This
    // library does not use the refnums of XNames.  The parser passes
    // them up to the application and the creator expects the
    // application to provide them.  If we wanted to write type-30
    // records, we would have to verify that all the reference numbers
    // were contiguous and began at 0, and then sort them before writing.

    modvars.reset();
    beginRecord(RID_XNAME_R);
    writeUInt(xname->getAttribute());
    writeString(xname->getName());
    writeUInt(xname->getRefnum());

    writePropertyList(xname->getPropertyList());
}



//----------------------------------------------------------------------
//                              Properties
//----------------------------------------------------------------------

void
OasisCreator::writePropertyList (const PropertyList& plist)
{
    PropertyList::const_iterator  iter = plist.begin();
    PropertyList::const_iterator  end  = plist.end();
    for ( ;  iter != end;  ++iter)
        writeProperty(*iter);
}



void
OasisCreator::writeProperty (const Property* prop)
{
    // Section 31:  PROPERTY record
    // `28' prop-info-byte [reference-number | propname-string]
    //      [prop-value-count]  [ <property-value>* ]
    // `29'
    // prop-info-byte ::= UUUUVCNS

    using namespace PropertyInfoByte;
    int  infoByte = 0;

    // Decide whether to write the propname and value list or pick them
    // up from the modal variables.

    PropName*  propName = prop->getName();
    setPropName(&infoByte, NameExplicitBit, propName);
    setPropValueList(&infoByte, ReuseValueBit, prop->getValueVector());

    Ulong  refnum;
    if (infoByte & NameExplicitBit) {
        if (propNameTable->getRefnum(propName, &refnum))
            infoByte |= RefnumBit;
        else
            propNameTable->notStrict();
    }

    // If all parts of the Property object (the name, the value list, and
    // the StandardBit in the info-byte) are the same as that of the
    // previous Property object written, use record type 29.

    bool  propIsStandard = prop->isStandard();
    if ((infoByte & (NameExplicitBit|ReuseValueBit)) == ReuseValueBit
            &&  propIsStandard == modvars.getPropertyIsStandard()) {
        beginRecord(RID_PROPERTY_REPEAT);
        return;
    }

    // Set the Standard bit in the info-byte and save it in the modal
    // variable.

    if (propIsStandard)
        infoByte |= StandardBit;
    modvars.setPropertyIsStandard(propIsStandard);

    // If we have to write the values (i.e., we cannot reuse the
    // previous property's value list), set the UUUU part of the
    // info-byte to the number of values.  Setting it to 15 means that
    // the the actual number of values appears in prop-value-count.

    if (! (infoByte & ReuseValueBit))
        SetNumValues(&infoByte, min(prop->numValues(), size_t(15)));

    // Write the PROPERTY record.

    beginRecord(RID_PROPERTY);
    writeInfoByte(infoByte);
    if (infoByte & NameExplicitBit) {
        if (infoByte & RefnumBit)
            writeUInt(refnum);
        else
            writeString(propName->getName());
    }
    if (GetNumValues(infoByte) == 15)
        writeUInt(prop->numValues());

    if (! (infoByte & ReuseValueBit)) {
        PropValueVector::const_iterator  iter = prop->beginValues();
        PropValueVector::const_iterator  end  = prop->endValues();
        for ( ;  iter != end;  ++iter)
            writePropValue(*iter);
    }
}



void
OasisCreator::writePropValue (const PropValue* propval)
{
    // Handle real property values specially.  The type field written
    // for the real also serves as the type field of the property value.

    if (propval->isReal()) {
        writeReal(propval->getRealValue());
        return;
    }

    // Other types of property values.  Write the value type and then
    // the value.  The PropString table becomes non-strict if the value
    // is a string literal.

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
            propStringTable->notStrict();
            break;

        // For PropString property values, write the refnum assigned
        // to the PropString value.  Every propString _must_ be registered.

        case PV_Ref_AsciiString:
        case PV_Ref_BinaryString:
        case PV_Ref_NameString: {
            Ulong  refnum;
            PropString*  propString = propval->getPropStringValue();
            if (! propStringTable->getRefnum(propString, &refnum))
                assert(false);
            writeUInt(refnum);
            break;
        }
        default:
            assert (false);
    }
}



void
OasisCreator::writeRepetition (const Repetition* rep)
    /* throw (overflow_error) */
{
    assert (rep != Null);

    // The loops here for offsets and deltas begin from 1.  The first
    // element, offset 0 or delta (0,0), is implicit in the file.

    RepetitionType  repType = rep->getType();
    writeUInt(repType);
    switch (repType) {
        case Rep_ReusePrevious:
            break;

        // 7.6.3  Type 1
        // x-dimension y-dimension x-space y-space

        case Rep_Matrix:
            writeUInt(rep->getMatrixXdimen() - 2);
            writeUInt(rep->getMatrixYdimen() - 2);
            writeUInt(rep->getMatrixXspace());
            writeUInt(rep->getMatrixYspace());
            break;

        // 7.6.4  Type 2:  x-dimension x-space

        case Rep_UniformX:
            writeUInt(rep->getDimen() - 2);
            writeUInt(rep->getUniformXspace());
            break;

        // 7.6.5  Type 3:  y-dimension y-space

        case Rep_UniformY:
            writeUInt(rep->getDimen() - 2);
            writeUInt(rep->getUniformYspace());
            break;

        // 7.6.6  Type 4
        // x-dimension x-space_1 ... x-space_(n-1)

        case Rep_VaryingX: {
            Ulong  dimen = rep->getDimen();
            writeUInt(dimen - 2);

            Ulong  lastOffset = 0;
            for (Ulong j = 1;  j < dimen;  ++j) {
                Ulong  offset = rep->getVaryingXoffset(j);
                assert (offset >= lastOffset);
                writeUInt(offset - lastOffset);
                lastOffset = offset;
            }
            break;
        }

        // 7.6.7  Type 5
        // x-dimension grid x-space_1 ... x-space_(n-1)
        //
        // Like Type 4, but divide each space by grid before writing it.

        case Rep_GridVaryingX: {
            Ulong  dimen = rep->getDimen();
            Ulong  grid = rep->getGrid();
            writeUInt(dimen - 2);
            writeUInt(grid);

            Ulong  lastOffset = 0;
            for (Ulong j = 1;  j < dimen;  ++j) {
                Ulong  offset = rep->getVaryingXoffset(j);
                assert (offset >= lastOffset);
                writeUInt((offset - lastOffset)/grid);
                lastOffset = offset;
            }
            break;
        }

        // 7.6.8  Type 6
        // y-dimension y-space_1 ... y-space_(n-1)
        //
        // Types 6 and 7 are the same as types 4 and 5 with x replaced by y.

        case Rep_VaryingY: {
            Ulong  dimen = rep->getDimen();
            writeUInt(dimen - 2);

            Ulong  lastOffset = 0;
            for (Ulong j = 1;  j < dimen;  ++j) {
                Ulong  offset = rep->getVaryingYoffset(j);
                assert (offset >= lastOffset);
                writeUInt(offset - lastOffset);
                lastOffset = offset;
            }
            break;
        }

        // 7.6.9  Type 7
        // y-dimension grid y-space_1 ... y-space_(n-1)

        case Rep_GridVaryingY: {
            Ulong  dimen = rep->getDimen();
            Ulong  grid = rep->getGrid();
            writeUInt(dimen - 2);
            writeUInt(grid);

            Ulong  lastOffset = 0;
            for (Ulong j = 1;  j < dimen;  ++j) {
                Ulong  offset = rep->getVaryingYoffset(j);
                assert (offset >= lastOffset);
                writeUInt((offset - lastOffset)/grid);
                lastOffset = offset;
            }
            break;
        }

        // 7.6.10  Type 8
        // n-dimension m-dimension n-displacement m-displacement

        case Rep_TiltedMatrix:
            writeUInt(rep->getMatrixNdimen() - 2);
            writeUInt(rep->getMatrixMdimen() - 2);
            writer.writeGDelta(rep->getMatrixNdelta());
            writer.writeGDelta(rep->getMatrixMdelta());
            break;

        // 7.6.11  Type 9
        // dimension displacement

        case Rep_Diagonal:
            writeUInt(rep->getDimen() - 2);
            writer.writeGDelta(rep->getDiagonalDelta());
            break;

        // 7.6.12  Type 10
        // dimension displacement_1 ... displacement_(n-1)

        case Rep_Arbitrary: {
            Ulong  dimen = rep->getDimen();
            writeUInt(dimen - 2);

            Delta  lastPos(0, 0);
            for (Ulong j = 1;  j < dimen;  ++j) {
                Delta  delta = rep->getDelta(j);
                writer.writeGDelta(delta - lastPos);            // may throw
                lastPos = delta;
            }
            break;
        }

        // 7.6.13  Type 11
        // dimension grid displacement_1 ... displacement_(n-1)

        case Rep_GridArbitrary: {
            Ulong  dimen = rep->getDimen();
            Ulong  grid = rep->getGrid();

            writeUInt(dimen - 2);
            writeUInt(grid);
            Delta  lastPos(0, 0);
            for (Ulong j = 1;  j < dimen;  ++j) {
                Delta  delta = rep->getDelta(j);
                writer.writeGDelta((delta - lastPos)/grid);     // may throw
                lastPos = delta;
            }
            break;
        }
    }

}



void
OasisCreator::writePointList (const PointList& ptlist, bool isPolygon)
    /* throw (overflow_error) */
{
    // A point-list begins with an unsigned-integer denoting its type.

    Ulong  ltype = ptlist.getType();
    writeUInt(ltype);

    // After that is the vertex-count, the number of deltas appearing in
    // the file.  The initial point (0,0) is implicit in the file.  For
    // types 0 and 1 (ManhattanHorizFirst and ManhattanVertFirst) that are
    // used for polygons, the last point is also implicit.

    Ulong  numDeltas = ptlist.size() - 1;
    if (isPolygon
            &&  (ltype == PointList::ManhattanHorizFirst
                 || ltype == PointList::ManhattanVertFirst))
        --numDeltas;
    writeUInt(numDeltas);

    // Then come the deltas for the points.  The form of the delta (1-delta,
    // 2-delta, etc.) depends on the list type.

    PointList::const_iterator  iter = ptlist.begin();
    PointList::const_iterator  end  = ptlist.end();
    ++iter;     // initial point is implicit

    Delta  prevPoint(0, 0);
    switch (ltype) {
        case PointList::ManhattanHorizFirst:
        case PointList::ManhattanVertFirst:
            if (isPolygon)
                --end;
            for ( ;  iter != end;  ++iter) {
                Delta  pt = *iter;
                writer.writeOneDelta(pt - prevPoint);
                prevPoint = pt;
            }
            break;

        case PointList::Manhattan:
            for ( ;  iter != end;  ++iter) {
                Delta  pt = *iter;
                writer.writeTwoDelta(pt - prevPoint);
                prevPoint = pt;
            }
            break;

        case PointList::Octangular:
            for ( ;  iter != end;  ++iter) {
                Delta  pt = *iter;
                writer.writeThreeDelta(pt - prevPoint);
                prevPoint = pt;
            }
            break;

        case PointList::AllAngle:
            for ( ;  iter != end;  ++iter) {
                Delta  pt = *iter;
                writer.writeGDelta(pt - prevPoint);
                prevPoint = pt;
            }
            break;

        case PointList::AllAngleDoubleDelta: {
            Delta  prevDelta(0, 0);
            for ( ;  iter != end;  ++iter) {
                Delta  delta = *iter - prevPoint;
                Delta  ddelta = delta - prevDelta;
                writer.writeGDelta(ddelta);
                prevPoint = *iter;
                prevDelta = delta;
            }
            break;
        }
    }
}




//----------------------------------------------------------------------
//                              Modal Variables
//----------------------------------------------------------------------

// Most OASIS records have optional fields that can be omitted if they
// have the same values as the corresponding modal variable.  Bits in an
// info-byte at the beginning of the record specify the fields that
// appear.
//
// This section contains one function for each modal variable.
// The typical function has three parameters:
//   - infoByte, a pointer to the info-byte that must be generated,
//   - bit, a bitmask for the bit to set in the info-byte, and
//   - the value to be written.
//
// If the modal variable is not defined or if its value is different
// from the value to be written, it is set to the new value and the bit
// is set in the info-byte.  Otherwise nothing is done.  Because the bit
// in the info-byte is not set, the caller does not write the value to
// the file.
//
// The first six functions, for placement-[xy], geometry-[xy], and
// text-[xy], are special in two ways:
//   - Their modal variables are always defined.
//   - The value argument is in-out, not in.  If the xy-mode is
//     xy-relative, the value is relativized by subtracting the value
//     of the modal variable.

void
OasisCreator::setPlacementX (/*inout*/ int* infoByte, int bit,
                             /*inout*/ long* px)
{
    long  x = *px;
    long  placeX = modvars.getPlacementX();
    if (x == placeX)
        return;

    *infoByte |= bit;
    modvars.setPlacementX(x);
    if (modvars.xyIsRelative())
        *px = CheckedMinus(x, placeX);
}



void
OasisCreator::setPlacementY (/*inout*/ int* infoByte, int bit,
                             /*inout*/ long* py)
{
    long  y = *py;
    long  placeY = modvars.getPlacementY();
    if (y == placeY)
        return;

    *infoByte |= bit;
    modvars.setPlacementY(y);
    if (modvars.xyIsRelative())
        *py = CheckedMinus(y, placeY);
}



void
OasisCreator::setGeometryX (/*inout*/ int* infoByte, int bit,
                            /*inout*/ long* px)
{
    long  x = *px;
    long  geomX = modvars.getGeometryX();
    if (x == geomX)
        return;

    *infoByte |= bit;
    modvars.setGeometryX(x);
    if (modvars.xyIsRelative())
        *px = CheckedMinus(x, geomX);
}



void
OasisCreator::setGeometryY (/*inout*/ int* infoByte, int bit,
                            /*inout*/ long* py)
{
    long  y = *py;
    long  geomY = modvars.getGeometryY();
    if (y == geomY)
        return;

    *infoByte |= bit;
    modvars.setGeometryY(y);
    if (modvars.xyIsRelative())
        *py = CheckedMinus(y, geomY);
}



void
OasisCreator::setTextX (/*inout*/ int* infoByte, int bit, /*inout*/ long* px)
{
    long  x = *px;
    long  textX = modvars.getTextX();
    if (x == textX)
        return;

    *infoByte |= bit;
    modvars.setTextX(x);
    if (modvars.xyIsRelative())
        *px = CheckedMinus(x, textX);
}



void
OasisCreator::setTextY (/*inout*/ int* infoByte, int bit, /*inout*/ long* py)
{
    long  y = *py;
    long  textY = modvars.getTextY();
    if (y == textY)
        return;

    *infoByte |= bit;
    modvars.setTextY(y);
    if (modvars.xyIsRelative())
        *py = CheckedMinus(y, textY);
}



void
OasisCreator::setLayer (/*inout*/ int* infoByte, int bit, Ulong layer)
{
    if (modvars.isdef(ModalVars::MV_LAYER)
            &&  layer == modvars.getLayer())
        return;

    *infoByte |= bit;
    modvars.setLayer(layer);
}



void
OasisCreator::setDatatype (/*inout*/ int* infoByte, int bit, Ulong datatype)
{
    if (modvars.isdef(ModalVars::MV_DATATYPE)
            &&  datatype == modvars.getDatatype())
        return;

    *infoByte |= bit;
    modvars.setDatatype(datatype);
}



void
OasisCreator::setTextlayer (/*inout*/ int* infoByte, int bit, Ulong textlayer)
{
    if (modvars.isdef(ModalVars::MV_TEXTLAYER)
            && textlayer == modvars.getTextlayer())
        return;

    *infoByte |= bit;
    modvars.setTextlayer(textlayer);
}



void
OasisCreator::setTexttype (/*inout*/ int* infoByte, int bit, Ulong texttype)
{
    if (modvars.isdef(ModalVars::MV_TEXTTYPE)
            &&  texttype == modvars.getTexttype())
        return;

    *infoByte |= bit;
    modvars.setTexttype(texttype);
}



void
OasisCreator::setCTrapezoidType (/*inout*/ int* infoByte, int bit,
                                 Uint  ctrapType)
{
    if (modvars.isdef(ModalVars::MV_CTRAPEZOID_TYPE)
            &&  ctrapType == modvars.getCTrapezoidType())
        return;

    *infoByte |= bit;
    modvars.setCTrapezoidType(ctrapType);
}



void
OasisCreator::setGeometryWidth (/*inout*/ int* infoByte, int bit, long width)
{
    if (modvars.isdef(ModalVars::MV_GEOMETRY_WIDTH)
            &&  width == modvars.getGeometryWidth())
        return;

    *infoByte |= bit;
    modvars.setGeometryWidth(width);
}



void
OasisCreator::setGeometryHeight (/*inout*/ int* infoByte, int bit,
                                 long height)
{
    if (modvars.isdef(ModalVars::MV_GEOMETRY_HEIGHT)
            &&  height == modvars.getGeometryHeight())
        return;

    *infoByte |= bit;
    modvars.setGeometryHeight(height);
}



void
OasisCreator::setPathHalfwidth (/*inout*/ int* infoByte, int bit,
                                long halfwidth)
{
    if (modvars.isdef(ModalVars::MV_PATH_HALFWIDTH)
            &&  halfwidth == modvars.getPathHalfwidth())
        return;

    *infoByte |= bit;
    modvars.setPathHalfwidth(halfwidth);
}



void
OasisCreator::setRadius (/*inout*/ int* infoByte, int bit, long radius)
{
    if (modvars.isdef(ModalVars::MV_CIRCLE_RADIUS)
            &&  radius == modvars.getCircleRadius())
        return;

    *infoByte |= bit;
    modvars.setCircleRadius(radius);
}



// setPathStartExtnScheme -- set SS bits in extension-scheme byte of PATH

void
OasisCreator::setPathStartExtnScheme (/*inout*/ Ulong* extnScheme,
                                      long startExtn, long halfwidth)
{
    using namespace PathInfoByte;

    Uint  ss;
    if (modvars.isdef(ModalVars::MV_PATH_START_EXTENSION)
            &&  startExtn == modvars.getPathStartExtension())
        ss = ReuseLastExtn;
    else if (startExtn == 0)
        ss = FlushExtn;
    else if (startExtn == halfwidth)
        ss = HalfwidthExtn;
    else
        ss = ExplicitExtn;

    SetStartScheme(extnScheme, ss);
    modvars.setPathStartExtension(startExtn);
}



void
OasisCreator::setPathEndExtnScheme (/*inout*/ Ulong* extnScheme,
                                    long endExtn, long halfwidth)
{
    using namespace PathInfoByte;

    Uint  ee;
    if (modvars.isdef(ModalVars::MV_PATH_END_EXTENSION)
            &&  endExtn == modvars.getPathEndExtension())
        ee = ReuseLastExtn;
    else if (endExtn == 0)
        ee = FlushExtn;
    else if (endExtn == halfwidth)
        ee = HalfwidthExtn;
    else
        ee = ExplicitExtn;

    SetEndScheme(extnScheme, ee);
    modvars.setPathEndExtension(endExtn);
}



void
OasisCreator::setPlacementCell (/*inout*/ int* infoByte, int bit,
                                CellName* cellName)
{
    if (modvars.isdef(ModalVars::MV_PLACEMENT_CELL)
            &&  cellName == modvars.getPlacementCell())
        return;

    *infoByte |= bit;
    modvars.setPlacementCell(cellName);
}



void
OasisCreator::setTextString (/*inout*/ int* infoByte, int bit,
                             TextString* textString)
{
    if (modvars.isdef(ModalVars::MV_TEXT_STRING)) {
        TextString*  oldString = modvars.getTextString();
        if (textString == oldString)
            return;
        // XXX: We might also want to reuse if neither the old nor the
        // new name has been registered and the name strings are
        // identical.
    }

    *infoByte |= bit;
    modvars.setTextString(textString);
}



void
OasisCreator::setPropName (/*inout*/ int* infoByte, int bit,
                           PropName* propName)
{
    if (modvars.isdef(ModalVars::MV_LAST_PROPERTY_NAME)
            &&  propName == modvars.getLastPropertyName())
        return;

    *infoByte |= bit;
    modvars.setLastPropertyName(propName);
}



void
OasisCreator::setPropValueList (/*inout*/ int* infoByte, int bit,
                                const PropValueVector& pvlist)
{
    // The meaning of this bit (ReuseValueBit) is opposite to that of
    // most info-byte bits.  The bit is set if the last value list is to
    // be reused and reset if the current value list is to be written.

    if (modvars.isdef(ModalVars::MV_LAST_VALUE_LIST)
            &&  pvlist == modvars.getLastValueList())   // this is expensive
        *infoByte |= bit;
    else
        modvars.setLastValueList(pvlist);
}



void
OasisCreator::setPolygonPoints (/*inout*/ int* infoByte, int bit,
                                const PointList& ptlist)
{
    if (modvars.isdef(ModalVars::MV_POLYGON_POINTS)
            &&  ptlist == modvars.getPolygonPoints())   // this is expensive
        return;

    *infoByte |= bit;
    modvars.setPolygonPoints(ptlist);
}



void
OasisCreator::setPathPoints (/*inout*/ int* infoByte, int bit,
                             const PointList& ptlist)
{
    if (modvars.isdef(ModalVars::MV_PATH_POINTS)
            &&  ptlist == modvars.getPathPoints())      // this is expensive
        return;

    *infoByte |= bit;
    modvars.setPathPoints(ptlist);
}



void
OasisCreator::setRepetition (/*inout*/ int* infoByte, int bit,
                             /*inout*/ const Repetition** repp)
{
    const Repetition*  rep = *repp;
    if (rep == Null)    // no repetition
        return;

    *infoByte |= bit;
    if (modvars.isdef(ModalVars::MV_REPETITION)
            &&  *rep == modvars.getRepetition())        // this is expensive
        *repp = &repReuse;
    else
        modvars.setRepetition(*rep);
}


}  // namespace Oasis
