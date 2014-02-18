// gdsii/builder.h -- coupling between GdsParser and client code
//
// last modified:   19-Jan-2010  Tue  09:35
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// A parser for GDSII files can be used in many ways.  So instead of
// creating a data structure directly, we use a Builder class (see the
// Design Patterns book) to decouple the parser from the rest of the
// application.  GdsParser's constructor is passed a pointer to a
// GdsBuilder.  As the parser recognizes each part of the input file, it
// invokes the builder's method for that part.
//
// To use the parser, derive a class from GdsBuilder, override the
// methods for the file parts that interest you, and pass an instance
// of the class to GdsParser.
//
// The GdsBuilder interface is designed so that it can reproduce a
// conforming input file exactly if needed.  This is important for
// applications such as compressing a GDSII file.  Wherever the GDSII
// Stream specification defines optional records, we provide a way for
// GdsBuilder to determine whether the record appeared in the file.
//
// GdsBuilder's secondary role is as the interface to GdsCreator, which
// is derived from GdsBuilder.  GdsCreator is the high-level interface
// for creating GDSII Stream files.
//
// XXX: Currently GDSII 8-byte reals are not exactly reproducible.
// When we convert them to doubles we lose 3 bits.  Two possible solutions:
//    - Use long double instead of double.
//    - Define a GdsReal struct that holds both the translated double
//      value and the original bytes.


#ifndef GDSII_BUILDER_H_INCLUDED
#define GDSII_BUILDER_H_INCLUDED

#include <algorithm>    // for swap()
#include <cassert>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#include "misc/geometry.h"
#include "glimits.h"
#include "locator.h"


namespace Gdsii {

using namespace GLimits;
using std::string;
using std::vector;
using SoftJin::Point2d;
using SoftJin::Uint;


// The GdsBuilder class is declared at the bottom of the file.  Before
// we get to it we must define all the enums and classes used in
// declaring the parameters of its methods.


// GdsPathtype -- legal values for the contents of a PATHTYPE record.
// Note that PathtypeCustom is 4, not 3.  Don't ask me why.

enum GdsPathtype {
    PathtypeFlush   = 0,        // square end flush with endpoint (default)
    PathtypeRound   = 1,        // end is a semicircle centered at endpoint
    PathtypeExtend  = 2,        // square end extending by half width
    PathtypeCustom  = 4         // square ends; BGNEXTN & ENDEXTN are valid
};



// GdsDate -- timestamp in the format used in the GDSII file.

struct GdsDate {
    short       year, month, day;
    short       hour, minute, second;

    GdsDate() { }

    GdsDate (short yy, short mon, short dd, short hh, short min, short ss) {
        year = yy;   month = mon;    day = dd;
        hour = hh;   minute = min;   second = ss;
    }

    bool        setTime (time_t time);
    bool        getTime (/*out*/ time_t* ptime) const;
};



// GdsAclEntry -- ACL entry in LIBSECUR record.
// This is probably meaningless to modern applications.

struct GdsAclEntry {
    short       groupid;
    short       userid;
    short       rights;

    GdsAclEntry() { }

    GdsAclEntry (short groupid, short userid, short rights) {
        this->groupid = groupid;
        this->userid  = userid;
        this->rights  = rights;
    }
};



// GdsFormat -- contents of FORMAT record; specifies file format.
// In a filtered file the FORMAT record is followed by one or more
// MASK records giving the layers selected by the filter.
// No idea what the EDSIII format is or whether it is used today.

enum GdsFormat {
    GDSII_Archive    = 0,
    GDSII_Filtered   = 1,
    EDSIII_Archive   = 2,
    EDSIII_Filtered  = 3,
};



// GdsUnits -- contents of UNITS record.

struct GdsUnits {
    double      dbToUser;       // size of database unit in user units
    double      dbToMeter;      // size of database unit in meters

    GdsUnits() { }

    GdsUnits (double dbToUser, double dbToMeter) {
        this->dbToUser  = dbToUser;
        this->dbToMeter = dbToMeter;
    }
};



//----------------------------------------------------------------------
// Classes for a single point (GdsPoint) and a sequence of
// points (GdsPointList).  GdsPoint is used only in GdsPointList.  When
// the parser needs to pass a single point, it passes the X and Y
// coordinates separately.  That is more convenient than using GdsPoint.

typedef Point2d<int>   GdsPoint;


// GdsPointList -- vector of (x,y) coordinates in XY record.
// This is a model of Random Access Container (in spite of the List suffix).

class GdsPointList : public vector<GdsPoint> {
public:
                GdsPointList() : vector<GdsPoint>() { }

    void        addPoint (const GdsPoint& pt) { push_back(pt);             }
    void        addPoint (int x, int y)       { push_back(GdsPoint(x, y)); }
};


//======================================================================


// Every library, structure, or element in a GDSII file has optional
// records.  Because we may use the parser in applications that have to
// create a bit-for-bit copy of the input, we need to remember which of
// these optional records appeared.  All data that appear in optional
// records are collected in classes named GdsXxxOptions derived from the
// class GdsOptions.  Some elements have an optional <strans>; the data
// in this is in GdsTransform, which is also derived from GdsOptions.
//
// The constructor for each of these classes initializes the instance
// as if none of the optional records appeared.


// GdsOptions -- base class to track which optional records appear in the file.
// Any optional data passed to a builder method should be packaged in
// a class derived from this.  The member `recordBits' is a bitmask
// that says which optional fields are specified.  Derived classes
// should define three things for each optional field:
//
//    - a REC_FOO enumerator, where FOO is the name of the
//      record containing the field, e.g., REC_ELFLAGS.
//
//    - a setFoo() method to set the field's value.  This should invoke
//      add(REC_FOO) to record that the field is defined.
//
//    - a getFoo() method to retrieve the value of the field.  If the
//      field has no default value, the method must
//      `assert(contains(REC_FOO))' to ensure that the field is defined.
//
// Clients can use `contains(REC_FOO)' to see if the field appeared in
// the input file.  Note that if the GDSII Stream specification defines
// a default value for some field, the field will be defined even if
// contains() returns false for the field.

class GdsOptions {
    int         recordBits;             // which fields appeared in input
public:
                GdsOptions() {
                    recordBits = 0;
                }
    void        add (int bit) {                 // record that field appeared
                    recordBits |= bit;
                }
    bool        contains (int bit) const {      // check if field appeared
                    return (recordBits & bit);
                }
};



// GdsTransform -- contents of STRANS, ANGLE, and MAG records.
// Transforms are used in SREF, AREF, and TEXT elements.

class GdsTransform : public GdsOptions {
public:
    enum {
        REC_STRANS = 0x01,      // STRANS record appeared
        REC_ANGLE  = 0x02,      // ANGLE  record appeared (implies REC_STRANS)
        REC_MAG    = 0x04,      // MAG    record appeared (implies REC_STRANS)
    };

private:
    bool        absAngle;       // true => angle not affected by parent ref
    bool        absMag;         // true => mag not affected by parent ref
    bool        reflectX;       // true => reflect about X axis before rotation
    double      angle;          // counter-clockwise, in degrees
    double      mag;            // magnification factor

public:
                GdsTransform() {
                    // The GDSII spec gives defaults for all fields.
                    angle = 0.0;
                    mag = 1.0;
                    reflectX = absAngle = absMag = false;
                }
    void        setStrans (bool reflectX, bool absMag, bool absAngle) {
                    this->reflectX = reflectX;
                    this->absMag   = absMag;
                    this->absAngle = absAngle;
                    add(REC_STRANS);
                }

    // setStrans() must be called before setAngle() or setMag().
    // This is because the grammar requires STRANS for both MAG and ANGLE.

    void        setAngle (double angle) {
                    assert (contains(REC_STRANS));
                    this->angle = angle;
                    add(REC_ANGLE);
                }
    void        setMag (double mag) {
                    assert (contains(REC_STRANS));
                    // Allow zero mag because some GDSII files use them
                    assert (mag >= 0.0);
                    this->mag = mag;
                    add(REC_MAG);
                }

    // isIdentity -- true if transform has no effect

    bool        isIdentity() const {
                    return (angle == 0.0  &&  mag == 1.0
                            &&  !reflectX  &&  !absAngle  &&  !absMag);
                }

    bool        isReflected() const     { return reflectX; }
    bool        angleIsAbsolute() const { return absAngle; }
    bool        magIsAbsolute() const   { return absMag;   }
    double      getAngle() const        { return angle;    }
    double      getMag() const          { return mag;      }
};




//----------------------------------------------------------------------
// GdsLibraryOptions -- all optional records in library header.
//
// <libheader>  ::= HEADER BGNLIB [LIBDIRSIZE] [SRFNAME] [LIBSECUR]
//                  LIBNAME [REFLIBS] [FONTS] [ATTRTABLE] [GENERATIONS]
//                  [<FormatType>] UNITS
// <FormatType> ::= FORMAT  |  FORMAT {MASK}+ ENDMASKS

class GdsLibraryOptions : public GdsOptions {
public:
    enum {
        REC_LIBSECUR     = 0x0001,
        REC_MASK         = 0x0002,
        REC_REFLIBS      = 0x0004,
        REC_ATTRTABLE    = 0x0008,
        REC_FONTS        = 0x0010,
        REC_SRFNAME      = 0x0020,
        REC_FORMAT       = 0x0040,
        REC_GENERATIONS  = 0x0080,
        REC_LIBDIRSIZE   = 0x0100,
    };

private:
    vector<GdsAclEntry> libsecur;
    vector<string>      masks;
    vector<string>      reflibs;
    string      attrtable;
    string      fonts[4];
    string      srfname;
    GdsFormat   format;
    int         generations;
    int         libdirsize;

public:
                GdsLibraryOptions() {
                    // The GDSII spec says that if there is no FORMAT
                    // record the file is assumed to be an Archive, but
                    // it doesn't say whether GDSII_Archive or
                    // EDSIII_Archive.  So we don't initialize it.
                    generations = 3;
                }

    //--------------------------------------------------
    // set methods

    void        addAclEntry (GdsAclEntry acl) {
                    assert (libsecur.size() < MaxAclEntries);
                    libsecur.push_back(acl);
                    add(REC_LIBSECUR);
                }
    void        addMask (const char* mask) {
                    masks.push_back(string(mask));
                    add(REC_MASK);
                }
    void        addReflib (const char* reflib) {
                    assert (reflibs.size() < MaxReflibs);
                    reflibs.push_back(string(reflib));
                    add(REC_REFLIBS);
                }
    void        setAttrtable (const char* attrtable) {
                    this->attrtable = attrtable;
                    add(REC_ATTRTABLE);
                }
    void        setFonts (const char* f0, const char* f1,
                          const char* f2, const char* f3)
                {
                    assert (strlen(f0) > 0);
                    assert (strlen(f0) <= MaxFontNameLength);
                    assert (strlen(f1) <= MaxFontNameLength);
                    assert (strlen(f2) <= MaxFontNameLength);
                    assert (strlen(f3) <= MaxFontNameLength);
                    fonts[0] = f0;   fonts[1] = f1;
                    fonts[2] = f2;   fonts[3] = f3;
                    add(REC_FONTS);
                }
    void        setSrfname (const char* srfname) {
                    this->srfname = srfname;
                    add(REC_SRFNAME);
                }
    void        setFormat (GdsFormat format) {
                    this->format = format;
                    add(REC_FORMAT);
                }
    void        setGenerations (int gen) {
                    generations = gen;
                    add(REC_GENERATIONS);
                }
    void        setLibdirsize (int size) {
                    libdirsize = size;
                    add(REC_LIBDIRSIZE);
                }

    //--------------------------------------------------
    // get methods

    Uint        numAclEntries() const { return libsecur.size(); }
    Uint        numMasks()      const { return masks.size();    }
    Uint        numReflibs()    const { return reflibs.size();  }

    GdsAclEntry getAclEntry (Uint n) const {
                    assert (contains(REC_LIBSECUR));
                    assert (n < libsecur.size());
                    return libsecur[n];
                }
    const char* getMask (Uint n) const {
                    assert (contains(REC_MASK));
                    assert (n < masks.size());
                    return masks[n].c_str();
                }
    const char* getReflib (Uint n) const {
                    assert (contains(REC_REFLIBS));
                    assert (n < reflibs.size());
                    return reflibs[n].c_str();
                }
    const char* getAttrtable() const {
                    assert (contains(REC_ATTRTABLE));
                    return attrtable.c_str();
                }
    const char* getFont (Uint n) const {
                    assert (contains(REC_FONTS));
                    assert (n < 4);
                    return fonts[n].c_str();
                }
    const char* getSrfname() const {
                    assert (contains(REC_SRFNAME));
                    return srfname.c_str();
                }
    GdsFormat   getFormat() const {
                    assert (contains(REC_FORMAT));
                    return format;
                }
    int         getGenerations() const {
                    // no assert because this has a default value
                    return generations;
                }
    int         getLibdirsize() const {
                    assert (contains(REC_LIBDIRSIZE));
                    return libdirsize;
                }
};



//----------------------------------------------------------------------
// GdsStructureOptions -- optional fields for structures
// The only field is the contents of the STRCLASS record, which the spec
// says must be zero for files created by non-Cadence programs.

class GdsStructureOptions : public GdsOptions {
public:
    enum {
        REC_STRCLASS = 0x01,
    };

private:
    Uint        strclass;

public:
                GdsStructureOptions() { }

    void        setStrclass (Uint strclass) {
                    this->strclass = strclass;
                    add(REC_STRCLASS);
                }
    Uint        getStrclass() const {
                    assert (contains(REC_STRCLASS));
                    return strclass;
                }
};



//----------------------------------------------------------------------
// GdsElementOptions -- optional fields common to all elements.
// Holds the values in the ELFLAGS and PLEX records.

class GdsElementOptions : public GdsOptions {
public:
    enum {
        REC_ELFLAGS = 0x01,
        REC_PLEX    = 0x02,
    };
private:
    bool        isExt;          // external data (from ELFLAGS)
    bool        isTempl;        // template data (from ELFLAGS)
    int         plex;

public:
                GdsElementOptions() {
                    isExt = isTempl = false;
                    // No default value for plex.
                }

    void        setElflags (bool isExt, bool isTempl) {
                    this->isExt = isExt;
                    this->isTempl = isTempl;
                    add(REC_ELFLAGS);
                }
    void        setPlex (int plex) {
                    this->plex = plex;
                    add(REC_PLEX);
                }

    bool        isExternal() const {
                    assert (contains(REC_ELFLAGS));
                    return isExt;
                }
    bool        isTemplate() const {
                    assert (contains(REC_ELFLAGS));
                    return isTempl;
                }
    int         getPlex() const {
                    assert (contains(REC_PLEX));
                    return plex;
                }
};



//----------------------------------------------------------------------
// GdsPathOptions -- optional fields for PATH elements

struct GdsPathOptions : public GdsElementOptions {
public:
    enum {
        // Note that GdsElementOptions uses values 0x01, 0x02.
        REC_PATHTYPE = 0x10,
        REC_WIDTH    = 0x20,
        REC_BGNEXTN  = 0x40,
        REC_ENDEXTN  = 0x80,
    };

private:
    GdsPathtype pathtype;
    int         width;          // negative => absolute (not affected by mag)
    int         bgnextn;        // only valid for PathtypeCustom
    int         endextn;        // only valid for PathtypeCustom

public:
                GdsPathOptions() {
                    pathtype = PathtypeFlush;
                    width = 0;
                    bgnextn = endextn = 0;
                    // The spec does not give defaults for bgnextn and
                    // endextn, but 0 looks reasonable.
                }
    void        setPathtype (GdsPathtype pathtype) {
                    this->pathtype = pathtype;
                    add(REC_PATHTYPE);
                }
    void        setWidth (int width) {
                    this->width = width;
                    add(REC_WIDTH);
                }
    void        setBeginExtn (int extn) {
                    assert (pathtype == PathtypeCustom);
                    bgnextn = extn;
                    add(REC_BGNEXTN);
                }
    void        setEndExtn (int extn) {
                    assert (pathtype == PathtypeCustom);
                    endextn = extn;
                    add(REC_ENDEXTN);
                }

    GdsPathtype getPathtype()  const { return pathtype; }
    int         getWidth()     const { return width;    }
    int         getBeginExtn() const { return bgnextn;  }
    int         getEndExtn()   const { return endextn;  }
};



//----------------------------------------------------------------------
// GdsTextOptions -- optional fields for TEXT elements.

class GdsTextOptions : public GdsElementOptions {
public:
    enum {
        // Note that GdsElementOptions uses values 0x01, 0x02.
        REC_PRESENTATION = 0x10,
        REC_PATHTYPE     = 0x20,
        REC_WIDTH        = 0x40
    };

    enum {                      // values for vertJustify
        JustifyTop    = 0,
        JustifyMiddle = 1,
        JustifyBottom = 2
    };
    enum {                      // values for horizJustify
        JustifyLeft   = 0,
        JustifyCenter = 1,
        JustifyRight  = 2
    };

private:
    Uint        font;
    Uint        horizJustify;
    Uint        vertJustify;
    GdsPathtype pathtype;
    int         width;          // negative => absolute (not affected by mag)

public:
                GdsTextOptions() {
                    // The GDSII spec gives default values for all the fields.
                    font = 0;
                    horizJustify = 0;
                    vertJustify = 0;
                    pathtype = PathtypeFlush;
                    width = 0;
                }

    void        setPresentation (Uint font, Uint vjust, Uint hjust) {
                    assert (font  <= 3);
                    assert (vjust <= 2);
                    assert (hjust <= 2);
                    this->font = font;
                    horizJustify = hjust;
                    vertJustify  = vjust;
                    add(REC_PRESENTATION);
                }
    void        setPathtype (GdsPathtype pathtype) {
                    this->pathtype = pathtype;
                    add(REC_PATHTYPE);
                }
    void        setWidth (int width) {
                    this->width = width;
                    add(REC_WIDTH);
                }

    Uint        getFont()         const { return font;         }
    Uint        getHorizJustify() const { return horizJustify; }
    Uint        getVertJustify()  const { return vertJustify;  }
    GdsPathtype getPathtype()     const { return pathtype;     }
    int         getWidth()        const { return width;        }
};


//======================================================================


// GdsBuilder -- callbacks for GdsParser.
// GdsParser::parseFile() invokes the builder methods in the order
// given by the Extended BNF description below.  The notation  { foo }*
// means zero or more occurrences of foo.  The indentation in the RHS of
// the productions is only for clarity.
//
// <file>      ::= setLocator()
//                 gdsVersion()
//                 beginLibrary()
//                     { <structure> }*
//                 endLibrary()
//
// <structure> ::= beginStructure()
//                     { <element> }*
//                 endStructure()
//
// <element>   ::= <beginElem>
//                     { addProperty() }*
//                 endElement()
//
// <beginElem> ::= beginBoundary()
//               | beginPath()
//               | beginSref()
//               | beginAref()
//               | beginNode()
//               | beginBox()
//               | beginText()
//
// GdsParser::parseStructure() invokes setLocator() and then the
// methods as for <structure>.
//
// The argument values in these methods should be obvious if you know
// GDSII.  Anything declared const char* is a NUL-terminated string
// whose contents may be erased after the method returns.  The structures
// passed by reference are also temporary.  Make a copy if you want to
// save the values.
//
// The initial call to setLocator() provides the builder with a
// pointer to a GdsLocator object, which gives the parser's location
// and context.  A GdsLocator contains a file name, structure name and
// file offset.  The parser updates these before it calls any other
// builder method.  The builder method can use this location data if it
// needs to display a message.
//
// The file name in the locator is whatever is passed to the GdsParser
// constructor.
//
// The structure name in the locator is Null for the calls to
// gdsVersion(), beginLibrary(), and endLibrary(); otherwise it is the
// name of the structure being parsed.
//
// Except for endLibrary(), the offset in the locator is the
// (uncompressed) file offset of the first record of whatever is being
// reported:
//
// gdsVersion()         offset of HEADER (always 0)
// beginLibrary()       offset of BGNLIB
// beginStructure()     offset of BGNSTR
// beginBoundary()      offset of BOUNDARY ... similarly for other elements
// addProperty()        offset of PROPATTR
// endElement()         offset of ENDEL  (probably useless)
// endStructure()       offset of ENDSTR (probably useless)
//
// The locator offset for endLibrary() is the offset of the byte (if
// any) following ENDLIB; this is the logical size of the input file.
// The file may have padding after the ENDLIB.
//
// The default implementation of all methods do nothing.  Derived
// classes need override only methods they are interested in.
//
// Filters
//
// GdsBuilders can be linked to form something like a Unix pipeline.
// The stages in this pipeline work on cells, elements, and properties
// rather than bytes.  Typically each virtual method invokes the
// corresponding method of the next stage in the pipeline.  Each stage
// can transform what it gets in any way so long as it invokes the next
// builder's methods in the order given by EBNF grammar above.  For
// example, it can delete selected cells or elements.
//
// Each builder has a pointer, 'next', which points to the next builder
// in the pipeline.  This is analogous to stdout in Unix filters.
// Builders that act as the sink in a pipeline ignore it.  It may be set
// and read using setNext() and getNext().


class GdsBuilder {
protected:
    GdsBuilder*   next;         // next stage in pipeline of builders

public:
    explicit
    GdsBuilder (GdsBuilder* next = Null) {
        this->next = next;
    }

    virtual       ~GdsBuilder() { }

    void          setNext (GdsBuilder* next) { this->next = next; }
    GdsBuilder*   getNext() const            { return next;       }

    //-------------------------------------------------------

    virtual void  setLocator (const GdsLocator* locator);
    virtual void  gdsVersion (int version);

    virtual void  beginLibrary (const char* libName,
                                const GdsDate& modTime,
                                const GdsDate& accTime,
                                const GdsUnits& units,
                                const GdsLibraryOptions& options);
    virtual void  endLibrary();

    virtual void  beginStructure (const char* structureName,
                                  const GdsDate& createTime,
                                  const GdsDate& modTime,
                                  const GdsStructureOptions& options);
    virtual void  endStructure();


    virtual void  beginBoundary (int layer, int datatype,
                                 const GdsPointList& points,
                                 const GdsElementOptions& options);

    virtual void  beginPath (int layer, int datatype,
                             const GdsPointList& points,
                             const GdsPathOptions& options);

    virtual void  beginSref (const char* sname,
                             int x, int y,
                             const GdsTransform& strans,
                             const GdsElementOptions& options);

    virtual void  beginAref (const char* sname,
                             Uint numCols, Uint numRows,
                             const GdsPointList& points,
                             const GdsTransform& strans,
                             const GdsElementOptions& options);

    virtual void  beginNode (int layer, int nodetype,
                             const GdsPointList& points,
                             const GdsElementOptions& options);

    virtual void  beginBox (int layer, int boxtype,
                            const GdsPointList& points,
                            const GdsElementOptions& options);

    virtual void  beginText (int layer, int texttype,
                             int x, int y,
                             const char* text,
                             const GdsTransform& strans,
                             const GdsTextOptions& options);

    virtual void  addProperty (int attr, const char* value);

    virtual void  endElement();
};


}  // namespace Gdsii

#endif  // GDSII_BUILDER_H_INCLUDED
