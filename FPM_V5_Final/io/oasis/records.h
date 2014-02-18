// oasis/records.h -- raw contents of OASIS records
//
// last modified:   30-Dec-2009  Wed  17:17
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// This file defines structs to hold the contents of OASIS records.
// For each record type there is a struct whose members correspond to
// the record's fields.  These records are used in the interface to
// the classes {Ascii,Oasis}Record{Reader,Writer}.

#ifndef OASIS_RECORDS_H_INCLUDED
#define OASIS_RECORDS_H_INCLUDED

#include <cassert>
#include <string>
#include <vector>
#include "oasis.h"
#include "names.h"
#include "rectypes.h"

namespace Oasis {

using std::string;
using std::vector;
using SoftJin::Uint;
using SoftJin::Ulong;


//======================================================================
//                           RawRepetition
//======================================================================

// RawRepetition -- raw form of repetition data in element records.
// This is a variant of the Repetition class defined in oasis.h, which
// is used by upper layers.  We cannot use Repetition here because that
// represents offsets using (signed) deltas rather than the unsigned
// numbers in the file.  Hence it cannot handle half the values that the
// OasisScanner can handle.  This layer will be used for low-level
// applications like OASIS->ASCII conversion, for which we would like
// to handle as wide a range of input data as possible.
//
// The other differences between RawRepetition and Repetition:
//   - Repetition adds the implicit point at (0, 0); this does not.
//   - Repetition stores all offsets with respect to the origin; this
//     stores offset with respect to the previous point as in
//     the file.
//   - Repetition stores the actual dimensions of the arrays; this
//     stores the dimension values that appear in the file, each of
//     which is two less than the actual value.
//
// All data members are public because no abstraction is provided.
// This is just a container for the data read from the file.

class RawRepetition {
public:
    RepetitionType  repType;

    union {
        Ulong   xdimen;         // for Matrix
        Ulong   ndimen;         // for TiltedMatrix
        Ulong   dimen;          // for all 1-dimensional reps
    };
    union {
        Ulong   ydimen;         // for Matrix
        Ulong   mdimen;         // for TiltedMatrix
        Ulong   grid;
    };
    Ulong       xspace;
    Ulong       yspace;

    vector<Delta>  deltas;
    vector<Ulong>  spaces;

public:
    // The compiler-generated default and copy constructors, copy
    // assignment operator and destructor are fine.

    //------------------------------------------------------------
    // Pseudo-constructors for initializing the raw repetition

    void        makeReuse();
    void        makeMatrix (Ulong xdimen, Ulong ydimen,
                            Ulong xspace, Ulong yspace);
    void        makeUniformX (Ulong xdimen, Ulong xspace);
    void        makeUniformY (Ulong ydimen, Ulong yspace);

    void        makeVaryingX (Ulong xdimen);
    void        makeVaryingY (Ulong ydimen);
    void        makeGridVaryingX (Ulong xdimen, Ulong grid);
    void        makeGridVaryingY (Ulong ydimen, Ulong grid);
    void        addSpace (Ulong space);

    void        makeTiltedMatrix (Ulong ndimen, Ulong mdimen,
                                  Delta nspace, Delta mspace);
    void        makeDiagonal (Ulong dimen, Delta disp);

    void        makeArbitrary (Ulong dimen);
    void        makeGridArbitrary (Ulong dimen, Ulong grid);
    void        addDelta (Delta disp);

    //------------------------------------------------------------
    // Accessors for data stored in non-obvious places

    Delta       getMatrixNdelta() const;
    Delta       getMatrixMdelta() const;
    Delta       getDiagonalDelta() const;
};


// All the RawRepetition methods are inline, even the ones that would
// normally be considered too large for inlining, because they are used
// in only one or two places.  The make* pseudo-constructors are used
// only in OasisRecordReader::readRepetition().


inline void
RawRepetition::makeReuse() {
    repType = Rep_ReusePrevious;
}


inline void
RawRepetition::makeMatrix (Ulong xdimen, Ulong ydimen,
                           Ulong xspace, Ulong yspace)
{
    repType = Rep_Matrix;
    this->xdimen = xdimen;    this->xspace = xspace;
    this->ydimen = ydimen;    this->yspace = yspace;
}



inline void
RawRepetition::makeUniformX (Ulong xdimen, Ulong xspace)
{
    repType = Rep_UniformX;
    this->dimen  = xdimen;
    this->xspace = xspace;
}



inline void
RawRepetition::makeUniformY (Ulong ydimen, Ulong yspace)
{
    repType = Rep_UniformY;
    this->dimen  = ydimen;
    this->yspace = yspace;
}


inline void
RawRepetition::makeVaryingX (Ulong xdimen)
{
    repType = Rep_VaryingX;
    this->dimen  = xdimen;
    spaces.clear();
    spaces.reserve(xdimen);
}


inline void
RawRepetition::makeGridVaryingX (Ulong xdimen, Ulong grid)
{
    repType = Rep_GridVaryingX;
    this->dimen = xdimen;
    this->grid = grid;
    spaces.clear();
    spaces.reserve(xdimen);
}


inline void
RawRepetition::makeVaryingY (Ulong ydimen)
{
    repType = Rep_VaryingY;
    this->dimen = ydimen;
    spaces.clear();
    spaces.reserve(ydimen);
}


inline void
RawRepetition::makeGridVaryingY (Ulong ydimen, Ulong grid)
{
    repType = Rep_GridVaryingY;
    this->dimen = ydimen;
    this->grid = grid;
    spaces.clear();
    spaces.reserve(ydimen);
}


inline void
RawRepetition::addSpace (Ulong space)
{
    assert (repType == Rep_VaryingX  ||  repType == Rep_GridVaryingX
        ||  repType == Rep_VaryingY  ||  repType == Rep_GridVaryingY);
    spaces.push_back(space);
}


inline void
RawRepetition::makeTiltedMatrix (Ulong ndimen, Ulong mdimen,
                                 Delta ndisp, Delta mdisp)
{
    repType = Rep_TiltedMatrix;
    this->ndimen = ndimen;
    this->mdimen = mdimen;
    deltas.clear();
    deltas.push_back(ndisp);
    deltas.push_back(mdisp);

}


inline Delta
RawRepetition::getMatrixNdelta() const
{
    assert (repType == Rep_TiltedMatrix);
    return deltas[0];
}


inline Delta
RawRepetition::getMatrixMdelta() const
{
    assert (repType == Rep_TiltedMatrix);
    return deltas[1];
}


inline void
RawRepetition::makeDiagonal (Ulong dimen, Delta disp)
{
    repType = Rep_Diagonal;
    this->dimen = dimen;
    deltas.clear();
    deltas.push_back(disp);
}


inline Delta
RawRepetition::getDiagonalDelta() const
{
    assert (repType == Rep_Diagonal);
    return deltas[0];
}


inline void
RawRepetition::makeArbitrary (Ulong dimen)
{
    repType = Rep_Arbitrary;
    this->dimen = dimen;
    deltas.clear();
    deltas.reserve(dimen);
}


inline void
RawRepetition::makeGridArbitrary (Ulong dimen, Ulong grid)
{
    repType = Rep_GridArbitrary;
    this->dimen = dimen;
    this->grid = grid;
    deltas.clear();
    deltas.reserve(dimen);
}


inline void
RawRepetition::addDelta (Delta disp)
{
    assert (repType == Rep_Arbitrary  ||  repType == Rep_GridArbitrary);
    deltas.push_back(disp);
}


//======================================================================
//                           Record Classes
//======================================================================

// For each OASIS record type (more or less), we define a struct to hold
// the raw contents of the record.  'Raw' means that implicit values are
// not filled in from the modal variables and reference-numbers are not
// resolved into names.  That happens in the layer above, in
// OasisParser.  These structs are used in the interface of the
// intermediate-layer classes, {Ascii,Oasis}Record{Reader,Writer}.
//
// OasisRecord is the base class for all the structs.
//
// The recID field's value is one of the RID_* enumerators when reading
// or writing an OASIS (binary) file.  It may also be one of the RIDX_*
// enumerators when reading or writing the ASCII version.  That is why
// it is declared Uint and not RecordID.
//
// In records returned by AsciiRecordReader, only the fileOffset member
// of recPos is defined.  It contains the starting line number of the
// record.
//
// OasisRecord has a virtual destructor to make it easier for
// RecordManager (at the bottom of this file) to delete its cache of
// records.


struct OasisRecord {
    Uint        recID;          // record-ID
    OFilePos    recPos;         // file position of record's first byte

    OasisRecord();
    virtual ~OasisRecord();

    // Although the compiler-generated copy constructor will work for
    // OasisRecord and all its subclasses, we don't need to copy
    // records.  This prevents accidental copies.
private:
    OasisRecord (const OasisRecord&);           // forbidden
    void operator= (const OasisRecord&);        // forbidden
};



// TableOffsets -- the table-offsets field of START and END records

struct TableOffsets {
    struct TableInfo {
        Ulong   strict;
        Ullong  offset;
    };
    TableInfo   cellName;
    TableInfo   textString;
    TableInfo   propName;
    TableInfo   propString;
    TableInfo   layerName;
    TableInfo   xname;
};



struct StartRecord : public OasisRecord {
    string      version;
    Oreal       unit;
    Ulong       offsetFlag;     // 0 => offsets member has valid data
    TableOffsets  offsets;

    StartRecord();
    virtual ~StartRecord();
};



struct EndRecord : public OasisRecord {
    Ulong       valScheme;
    uint32_t    valSignature;
    Ulong       offsetFlag;     // !0 => offsets member has valid data
    TableOffsets  offsets;

    EndRecord();
    virtual ~EndRecord();
};



// NameRecord is used for CELLNAME, TEXTSTRING, PROPNAME, and PROPSTRING.

struct NameRecord : public OasisRecord {
    string      name;
    Ulong       refnum;         // defined only for the RID_*_R record types

    NameRecord();
    virtual ~NameRecord();
};



struct LayerNameRecord : public OasisRecord {
    struct RawInterval {
        Ulong   type;
        Ulong   bound_a;
        Ulong   bound_b;        // defined only for type 4
    };

    string      name;
    RawInterval layers;
    RawInterval types;

    LayerNameRecord();
    virtual ~LayerNameRecord();
};



struct CellRecord : public OasisRecord {
    string      name;           // defined for RID_CELL_NAMED
    Ulong       refnum;         // defined for RID_CELL_REF

    CellRecord();
    virtual ~CellRecord();
};


// For all the record classes that contain an infoByte member, the
// bits in infoByte specify which of the other members are defined.


struct PlacementRecord : public OasisRecord {
    int         infoByte;
    Ulong       refnum;
    string      name;
    Oreal       mag;            // not defined for RID_PLACEMENT
    Oreal       angle;          // not defined for RID_PLACEMENT
    long        x, y;
    RawRepetition  rawrep;

    PlacementRecord();
    virtual ~PlacementRecord();
};



struct TextRecord : public OasisRecord {
    int         infoByte;
    Ulong       refnum;
    string      text;
    Ulong       textlayer;
    Ulong       texttype;
    long        x, y;
    RawRepetition  rawrep;

    TextRecord();
    virtual ~TextRecord();
};



struct RectangleRecord : public OasisRecord {
    int         infoByte;
    Ulong       layer;
    Ulong       datatype;
    Ulong       width;
    Ulong       height;
    long        x, y;
    RawRepetition  rawrep;

    RectangleRecord();
    virtual ~RectangleRecord();
};



struct PolygonRecord : public OasisRecord {
    int         infoByte;
    Ulong       layer;
    Ulong       datatype;
    long        x, y;
    PointList   ptlist;
    RawRepetition  rawrep;

    PolygonRecord();
    virtual ~PolygonRecord();
};



struct PathRecord : public OasisRecord {
    int         infoByte;
    Ulong       extnScheme;
    Ulong       layer;
    Ulong       datatype;
    Ulong       halfwidth;
    long        startExtn;
    long        endExtn;
    long        x, y;
    PointList   ptlist;
    RawRepetition  rawrep;

    PathRecord();
    virtual ~PathRecord();
};



struct TrapezoidRecord : public OasisRecord {
    int         infoByte;
    Ulong       layer;
    Ulong       datatype;
    Ulong       width;
    Ulong       height;
    long        delta_a;
    long        delta_b;
    long        x, y;
    RawRepetition  rawrep;

    TrapezoidRecord();
    virtual ~TrapezoidRecord();
};



struct CTrapezoidRecord : public OasisRecord {
    int         infoByte;
    Ulong       layer;
    Ulong       datatype;
    Ulong       ctrapType;
    Ulong       width;
    Ulong       height;
    long        x, y;
    RawRepetition  rawrep;

    CTrapezoidRecord();
    virtual ~CTrapezoidRecord();
};




struct CircleRecord : public OasisRecord {
    int         infoByte;
    Ulong       layer;
    Ulong       datatype;
    Ulong       radius;
    long        x, y;
    RawRepetition  rawrep;

    CircleRecord();
    virtual ~CircleRecord();
};



// PropertyRecord is used for RID_PROPERTY and RID_PROPERTY_REPEAT, but
// none of the data members here is defined for RID_PROPERTY_REPEAT;
// only the data members inherited from OasisRecord are.

struct PropertyRecord : public OasisRecord {
    int         infoByte;
    string      name;
    Ulong       refnum;
    PropValueVector  values;

    PropertyRecord();
    virtual ~PropertyRecord();
};



struct XNameRecord : public OasisRecord {
    Ulong       attribute;
    string      name;
    Ulong       refnum;         // defined only for RID_XNAME_R

    XNameRecord();
    virtual ~XNameRecord();
};



struct XElementRecord : public OasisRecord {
    Ulong       attribute;
    string      data;

    XElementRecord();
    virtual ~XElementRecord();
};



struct XGeometryRecord : public OasisRecord {
    int         infoByte;
    Ulong       attribute;
    string      data;
    Ulong       layer;
    Ulong       datatype;
    long        x, y;
    RawRepetition  rawrep;

    XGeometryRecord();
    virtual ~XGeometryRecord();
};



struct CblockRecord : public OasisRecord {
    Ulong       compType;
    Ullong      uncompBytes;
    Ullong      compBytes;

    CblockRecord();
    virtual ~CblockRecord();
};



// RecordManager -- container for cache of all records
// Tiny helper class to make the constructors of OasisRecordReader and
// AsciiRecordReader exception-safe.

class RecordManager {
    OasisRecord*  records[RIDX_MaxID+1];
public:
                  RecordManager();
                  ~RecordManager();
    void          createRecords();
    OasisRecord*  getRecord (Uint recID);
};



inline OasisRecord*
RecordManager::getRecord (Uint recID) {
    assert (recID <= RIDX_MaxID);
    return records[recID];
}


} // namespace Oasis

#endif  // OASIS_RECORDS_H_INCLUDED
