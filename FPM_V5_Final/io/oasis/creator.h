// oasis/creator.h -- high-level interface for creating OASIS files
//
// last modified:   01-Jan-2010  Fri  21:55
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// OasisCreator works at the level of names and records.  It uses the
// low-level OasisWriter for the actual writing.

#ifndef OASIS_CREATOR_H_INCLUDED
#define OASIS_CREATOR_H_INCLUDED

#include <memory>
#include <string>

#include "port/hash-table.h"
#include "misc/utils.h"

#include "builder.h"
#include "modal-vars.h"
#include "names.h"
#include "oasis.h"
#include "rectypes.h"
#include "writer.h"


namespace Oasis {

using std::auto_ptr;
using std::string;
using SoftJin::Uint;
using SoftJin::Ulong;
using SoftJin::HashMap;
using SoftJin::HashPointer;


// OasisCreatorOptions -- options passed to OasisCreator constructor
//
// immediateNames       bool
//
//      If this is true, each registerFooName() method of OasisCreator
//      writes the name record immediately to the file.  If this is false,
//      OasisCreator writes all name records in name tables at the end of
//      the file.
//
//      Although the OASIS specification allows name records to appear
//      anywhere in the file, some OASIS tools from other vendors
//      require each name record to appear before the name's
//      reference-number is used.  If you want to create files that such
//      tools can read, set immediateNames to true and register each
//      name before using it.
//
//      The recommended value is false if you don't want to worry about
//      compatibility with tools that do not implement the full OASIS
//      spec.  That is because setting it to true will make OasisParser
//      parse the file twice, once for the name records and once for the
//      rest.


struct OasisCreatorOptions {
    bool        immediateNames;

    OasisCreatorOptions (bool immediateNames) {
        this->immediateNames = immediateNames;
    }
};



// OasisCreator -- create OASIS files
//
// The OasisBuilder virtual methods in OasisCreator must be invoked in
// the same order that OasisParser invokes them.  See the comment for
// OasisBuilder in builder.h.  There are two exceptions.
//
//  - endElement() need not be called.  OasisCreator inherits the empty
//    method from OasisBuilder.
//
//  - The registerFooName() methods need be called only for PropStrings
//    and names that have properties.  If options.immediateNames is
//    false the registration methods may be called anytime before
//    endFile().  If options.immediateNames is true, the registration
//    methods must be called only when it is legal to write a name record.
//
//    It is a good idea to register all names before using them, but it
//    is not required.  See the comment before registerCellName() in
//    creator.cc.  Names must not be registered more than once.
//
// setXYrelative() may only be called just before beginning an element.
//
// setCompression() may be called anytime.
//
// Any public method may throw runtime_error or bad_alloc.  The
// geometry-writing functions may also throw overflow_error.
//
// All name objects (of type OasisName or its subclasses) passed to any
// OasisCreator method must live until endFile() returns.  The caller
// remains the owner of the names.
//
// Data members
//
// writer               OasisWriter
//
//      The low-level writing interface.
//
// modvars              ModalVars
//
//      Maintains the state of the modal variables.
//
// repReuse             Repetition
//
//      A Repetition object whose type is Rep_ReusePrevious.  This object
//      is written whenever the Repetition passed to any of the
//      element-writing functions is the same as the previous one.
//      This member is set in the constructor and not changed afterwards.
//
// mustCompress         bool
//
//      If true, the contents of each cell and name table are compressed.
//
// cellOffsetPropName   PropName*
//      A PropName object whose name is S_CELL_OFFSET.  This is set
//      by makeCellOffsetPropName() just before the name tables are
//      written and used by writeCellName() to provide an S_CELL_OFFSET
//      property for each CellName corresponding to a CELL record.
//
//      This PropName object may have been registered by the application
//      or created by this class.
//
//      This member is defined only when options.immediateNames is false.
//
//
// deletePropName       bool
//
//      If true, this class is the owner of the object pointed to by
//      cellOffsetPropName and should delete it in the destructor.  If
//      false, the PropName object was created by the application; so
//      the creator should not touch it.
//
// s_cell_offset        string
//
//      A std::string whose value is "S_CELL_OFFSET".  We need this in
//      a couple of methods; so we store it in the class itself to avoid
//      repeated constructions.
//
// cellOffsets          CellOffsetMap
//
//      Contains the starting file offset of each cell written.
//      The key is the CellName* for the cell.  beginCell() stores the
//      offset here and writeCellName() uses it to to set the value of
//      the property S_CELL_OFFSET.
//
// cellNameTable        auto_ptr<CellNameTable>
// textStringTable      auto_ptr<TextStringTable>
// propNameTable        auto_ptr<PropNameTable>
// propStringTable      auto_ptr<PropStringTable>
// layerNameTable       auto_ptr<LayerNameTable>
// xnameTable           auto_ptr<XNameTable>
//
//      These are analogous to the dictionaries in dicts.h, but are simpler.
//      They contain all the registered names (those passed to the one of
//      the registerFooName() methods) and assign a refnum to each.
//      We store pointers and not the actual tables so that we can declare
//      the classes in creator.cc and avoid cluttering this header file.


class OasisCreator : public OasisBuilder {
    class NameTable;
    class RefNameTable;
    class LayerNameTable;
    class XNameTable;

    typedef RefNameTable  CellNameTable;
    typedef RefNameTable  TextStringTable;
    typedef RefNameTable  PropNameTable;
    typedef RefNameTable  PropStringTable;

    typedef HashMap<CellName*, off_t, HashPointer>   CellOffsetMap;

private:
    OasisWriter writer;
    ModalVars   modvars;                // state of modal variables
    Repetition  repReuse;               // its type is Rep_ReusePrevious
    bool        mustCompress;           // true => compress cells and tables
    bool        nowCompressing;         // true => output now being compressed
    bool        deletePropName;         // true => we own cellOffsetPropName
    PropName*   cellOffsetPropName;     // for S_CELL_OFFSET property
    string      s_cell_offset;          // value is "S_CELL_OFFSET"
    CellOffsetMap        cellOffsets;   // file offset of each cell written
    OasisCreatorOptions  options;       // options passed to constructor

    // Name tables for the six types of names
    auto_ptr<CellNameTable>    cellNameTable;
    auto_ptr<TextStringTable>  textStringTable;
    auto_ptr<PropNameTable>    propNameTable;
    auto_ptr<PropStringTable>  propStringTable;
    auto_ptr<LayerNameTable>   layerNameTable;
    auto_ptr<XNameTable>       xnameTable;

public:
                OasisCreator (const char* fname, const OasisCreatorOptions&);
    virtual     ~OasisCreator();

    // OasisBuilder virtual methods.

    virtual void  beginFile (const string& version,
                             const Oreal& unit,
                             Validation::Scheme valScheme);
    virtual void  endFile();

    virtual void  beginCell (CellName* cellName);
    virtual void  endCell();


    virtual void  beginPlacement (CellName* cellName,
                                  long x, long y,
                                  const Oreal&  mag,
                                  const Oreal&  angle,
                                  bool flip,
                                  const Repetition*  rep);

    virtual void  beginText (Ulong textlayer, Ulong texttype,
                             long x, long y,
                             TextString* text,
                             const Repetition* rep);

    virtual void  beginRectangle (Ulong layer, Ulong datatype,
                                  long x, long y,
                                  long width, long height,
                                  const Repetition*  rep);

    virtual void  beginPolygon (Ulong layer, Ulong datatype,
                                long x, long y,
                                const PointList&  ptlist,
                                const Repetition*  rep);

    virtual void  beginPath (Ulong layer, Ulong datatype,
                             long x, long  y,
                             long halfwidth,
                             long startExtn, long endExtn,
                             const PointList&  ptlist,
                             const Repetition*  rep);

    virtual void  beginTrapezoid (Ulong layer, Ulong datatype,
                                  long x, long  y,
                                  const Trapezoid& trap,
                                  const Repetition*  rep);

    virtual void  beginCircle (Ulong layer, Ulong datatype,
                               long x, long y,
                               long radius,
                               const Repetition*  rep);

    virtual void  beginXElement (Ulong attribute, const string& data);

    virtual void  beginXGeometry (Ulong layer, Ulong datatype,
                                  long x, long y,
                                  Ulong attribute,
                                  const string& data,
                                  const Repetition*  rep);

    // virtual void  endElement();
    //     Inherit empty method from OasisBuilder; we don't need it.

    virtual void  addCellProperty (Property* prop);
    virtual void  addFileProperty (Property* prop);
    virtual void  addElementProperty (Property* prop);

    virtual void  registerCellName   (CellName*   cellName);
    virtual void  registerTextString (TextString* textString);
    virtual void  registerPropName   (PropName*   propName);
    virtual void  registerPropString (PropString* propString);
    virtual void  registerLayerName  (LayerName*  layerName);
    virtual void  registerXName      (XName*      xname);

public:
    // Other public methods
    void        setXYrelative (bool flag);
    void        setCompression (bool flag);

private:
    // Abbreviations
    void        writeSInt (long val);
    void        writeUInt (Ulong val);
    void        writeSInt64 (llong val);
    void        writeUInt64 (Ullong val);
    void        writeReal (const Oreal& val);
    void        writeString (const string& val);
    void        beginRecord (RecordID recID);
    void        writeInfoByte (int infoByte);
    void        beginBlock();
    void        endBlock();

    // Name tables
    void        writeTableInfo (const NameTable* ntab);
    void        writeNameTables();
    void        makeCellOffsetPropName();
    void        writePropNameTable();
    void        writePropStringTable();
    void        writeCellNameTable();
    void        writeTextStringTable();
    void        writeLayerNameTable();
    void        writeXNameTable();

    // Names
    void        writeCellName (CellName* cellName);
    void        writeTextString (TextString* textString);
    void        writePropName (PropName* propName);
    void        writePropString (PropString* propString);
    void        writeLayerName (LayerName* layerName);
    void        writeInterval (const Interval& interval);
    void        writeXName (XName* xname);

    // Properties
    void        writePropertyList (const PropertyList& plist);
    void        writeProperty (const Property* prop);
    void        writePropValue (const PropValue* propval);

    // Repetition and point-list
    void        writeRepetition (const Repetition* rep);
    void        writePointList (const PointList& ptlist, bool isPolygon);

    // Modal variables
    void        setPlacementX (/*inout*/ int* infoByte, int bit,
                               /*inout*/ long* px);
    void        setPlacementY (/*inout*/ int* infoByte, int bit,
                               /*inout*/ long* py);
    void        setGeometryX (/*inout*/ int* infoByte, int bit,
                              /*inout*/ long* px);
    void        setGeometryY (/*inout*/ int* infoByte, int bit,
                              /*inout*/ long* py);
    void        setTextX (/*inout*/ int* infoByte, int bit,
                          /*inout*/ long* px);
    void        setTextY (/*inout*/ int* infoByte, int bit,
                          /*inout*/ long* py);
    void        setLayer (/*inout*/ int* infoByte, int bit, Ulong layer);
    void        setDatatype (/*inout*/ int* infoByte, int bit, Ulong datatype);
    void        setTextlayer (/*inout*/ int* infoByte, int bit,
                              Ulong textlayer);
    void        setTexttype (/*inout*/ int* infoByte, int bit, Ulong texttype);
    void        setCTrapezoidType (/*inout*/ int* infoByte, int bit,
                                   Uint  ctrapType);
    void        setGeometryWidth (/*inout*/ int* infoByte, int bit,
                                  long width);
    void        setGeometryHeight (/*inout*/ int* infoByte, int bit,
                                   long height);
    void        setPathHalfwidth (/*inout*/ int* infoByte, int bit,
                                  long halfwidth);
    void        setRadius (/*inout*/ int* infoByte, int bit, long radius);
    void        setPathStartExtnScheme (/*inout*/ Ulong* extnScheme,
                                        long startExtn, long halfwidth);
    void        setPathEndExtnScheme (/*inout*/ Ulong* extnScheme,
                                      long endExtn, long halfwidth);
    void        setPlacementCell (/*inout*/ int* infoByte, int bit,
                                  CellName* cellName);
    void        setTextString (/*inout*/ int* infoByte, int bit,
                               TextString* textString);
    void        setPropName (/*inout*/ int* infoByte, int bit,
                             PropName* propName);
    void        setPropValueList (/*inout*/ int* infoByte, int bit,
                                  const PropValueVector& pvlist);
    void        setPolygonPoints (/*inout*/ int* infoByte, int bit,
                                  const PointList& ptlist);
    void        setPathPoints (/*inout*/ int* infoByte, int bit,
                               const PointList& ptlist);
    void        setRepetition (/*inout*/ int* infoByte, int bit,
                               /*inout*/ const Repetition** repp);

private:
                OasisCreator (const OasisCreator&);     // forbidden
    void        operator= (const OasisCreator&);        // forbidden
};


}  // namespace Oasis

#endif  // OASIS_CREATOR_H_INCLUDED
