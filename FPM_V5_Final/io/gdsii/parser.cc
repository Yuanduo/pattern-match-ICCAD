// gdsii/parser.cc -- high-level interface for reading GDSII Stream files
//
// last modified:   07-Mar-2010  Sun  21:40
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "misc/utils.h"
#include "glimits.h"
#include "parser.h"


namespace Gdsii {

using namespace std;
using namespace SoftJin;
using namespace GLimits;        // constants defined in GDSII Stream spec


// The default GdsGraphBuilder builder methods do nothing.
// Subclasses need override only the methods in which they are interested.

GdsGraphBuilder::GdsGraphBuilder() { }
/*virtual*/ GdsGraphBuilder::~GdsGraphBuilder() { }
/*virtual*/ void GdsGraphBuilder::beginLibrary (const char*) { }
/*virtual*/ void GdsGraphBuilder::enterStructure (const char*) { }
/*virtual*/ void GdsGraphBuilder::addSref (const char*) { }
/*virtual*/ void GdsGraphBuilder::endLibrary() { }



//======================================================================
//                              GdsParser
//======================================================================

// constructor
//   fname      pathname of input file
//   ftype      type of file: normal or compressed
//   warnHandler   if non-Null, it is invoked with a message
//              whenever the parser encounters a minor error in the input.
//
// Use something like this if your warning handler is a non-static
// member function:
//
//      class MyBuilder : public GdsBuilder {
//          // ...
//          void ShowWarning (const char* msg);
//      };
//      MyBuilder builder;
//      GdsParser parser("foo.gds",
//                       std::bind1st(std::mem_fun(&MyBuilder::ShowWarning),
//                                    &builder));

GdsParser::GdsParser (const char* fname, FileHandle::FileType ftype,
                      WarningHandler warner)
  : scanner(fname, ftype),
    warnHandler(warner),
    strPool(65536),
    locator(fname)
{
    builder = Null;
}


GdsParser::~GdsParser() { }


//----------------------------------------------------------------------
// Low-level private utility methods
// Placed here because most of them are inline.


// verifyRecordType -- verify that an input record has the expected type.
//   recType    expected record type
//   rec        input record

inline void
GdsParser::verifyRecordType (GdsRecordType recType, const GdsRecord& rec)
{
    if (recType != rec.getRecordType())
        abortParser(rec, "expected %s record instead",
                    GdsRecordTypeInfo::getName(recType));
}



// readNextRecord -- get the next record from the scanner.

inline void
GdsParser::readNextRecord (/*out*/ GdsRecord* recp) {
    scanner.getNextRecord(recp);
}



// readNextRecord -- read the next record and verify its record type.
//   recType    expected type of the record
//   recp       out: where the record should be stored

void
GdsParser::readNextRecord (GdsRecordType recType, /*out*/ GdsRecord* recp) {
    readNextRecord(recp);
    verifyRecordType(recType, *recp);
}



// parseLayerRecord -- get layer number from LAYER record and check it.

inline int
GdsParser::parseLayerRecord (/*in*/ GdsRecord& rec)
{
    assert(rec.getRecordType() == GRT_LAYER);

    int  layer = rec.nextShort();
    if (layer < 0)
        abortParser(rec, "invalid layer number %d", layer);
    return layer;
}



// parseDatatypeRecord -- get datatype from DATATYPE record and check it.

inline int
GdsParser::parseDatatypeRecord (/*in*/ GdsRecord& rec)
{
    assert(rec.getRecordType() == GRT_DATATYPE);

    int  datatype = rec.nextShort();
    if (datatype < 0)
        abortParser(rec, "invalid datatype number %d", datatype);
    return datatype;
}



// parsePathtypeRecord -- get pathtype from PATHTYPE record and check it.

inline GdsPathtype
GdsParser::parsePathtypeRecord (/*in*/ GdsRecord& rec)
{
    assert(rec.getRecordType() == GRT_PATHTYPE);

    int  pathtype = rec.nextShort();
    switch (pathtype) {
        case PathtypeFlush:
        case PathtypeRound:
        case PathtypeExtend:
        case PathtypeCustom:
            break;
        default:
            warn(rec, "replacing invalid pathtype %d by 0", pathtype);
            pathtype = 0;
            break;
    }

    return (static_cast<GdsPathtype>(pathtype));
}



// parseFormatRecord -- get file format from FORMAT record and check it.

inline GdsFormat
GdsParser::parseFormatRecord (/*in*/ GdsRecord& rec)
{
    assert(rec.getRecordType() == GRT_FORMAT);

    int  format = rec.nextShort();
    switch (format) {
        case GDSII_Archive:
        case GDSII_Filtered:
        case EDSIII_Archive:
        case EDSIII_Filtered:
            break;
        default:
            abortParser(rec, "invalid format %d", format);
    }

    return (static_cast<GdsFormat>(format));
}



// copyStringFromRecord -- make temporary NUL-terminated copy of string.
//   rec        inout: the record from which we get the string.
//              Note that the record's read cursor is modified when
//              this function reads the string.
// The copy is made in strPool and is guaranteed to remain only until
// the next ENDEL.

inline const char*
GdsParser::copyStringFromRecord (/*inout*/ GdsRecord& rec)
{
    // Some string records have a single variable-length string.  These
    // have rti->itemSize == 0.  Others have one or more fixed-length
    // strings that are padded with NULs.  These have rti->itemSize > 0.

    const GdsRecordTypeInfo*  rti = rec.getTypeInfo();
    Uint  len = (rti->itemSize > 0) ? rti->itemSize : rec.getLength();

    // We don't care if the string is padded with NULs.
    // XXX: check for embedded NULs in the string?
    return (strPool.newString(rec.nextString(), len));
}


//----------------------------------------------------------------------
// The public methods


// parseFile -- parse file sequentially from beginning to end.
//   builder    its methods are invoked each time a
//              structure/element/property etc. is parsed
// See the GdsBuilder class comments in builder.h for the order in which
// the builder's callbacks are invoked.

void
GdsParser::parseFile (GdsBuilder* builder)
{
    this->builder = builder;

    // Return to the beginning of the file and discard strings
    // accumulated in the previous parse, if any.  Note that we leave
    // structIndex alone.  There is no reason to rebuild it.

    scanner.seekTo(0);
    strPool.clear();

    // This is the overall structure of a GDSII file:
    //    <file> ::= HEADER <libheader> {<structure>}* ENDLIB
    // HEADER contains the version of the GDSII Stream format.

    GdsRecord   rec;
    readNextRecord(GRT_HEADER, &rec);
    int  version = rec.nextShort();
    locator.setOffset(0);
    builder->setLocator(&locator);
    builder->gdsVersion(version);

    // <libheader> is everything from BGNLIB (which appears immediately
    // after HEADER) up to and including the UNITS record.  Each
    // <structure> begins with BGNSTR and ends with ENDSTR.

    parseLibraryHeader();               // eats everything up to UNITS
    for (;;) {
        readNextRecord(&rec);
        if (rec.getRecordType() == GRT_ENDLIB)
            break;
        verifyRecordType(GRT_BGNSTR, rec);
        parseStructure(rec);            // eats everything up to ENDSTR
    }

    // Ignore everything after ENDLIB.  The current offset, which is the
    // logical file size, is the locator's offset for endLibrary().

    locator.setOffset(scanner.currByteOffset());
    builder->endLibrary();

    // Whether or not the index was built before we began the parse,
    // it's done now.

    structIndex.setDone();
}



// parseStructure -- parse a single structure definition
//   sname      the name of a structure that exists in the file
//   builder    its methods are invoked during the parse.  These builder
//              methods are _not_ invoked:
//                  gdsVersion(), beginLibrary(), endLibrary().
// Returns false if no structure named sname exists in the file; true
// if the structure exists and was parsed successfully.
//
// Note that there is a private method with the same name that this
// one invokes.

bool
GdsParser::parseStructure (const char* sname, GdsBuilder* builder)
{
    assert (builder != Null);
    this->builder = builder;

    makeIndex();
    off_t  offset = structIndex.getOffset(sname);
    if (offset < 0)
        return false;
    scanner.seekTo(offset);

    GdsRecord  rec;
    readNextRecord(GRT_BGNSTR, &rec);
    parseStructure(rec);
    return true;
}



// makeIndex -- prepare index mapping structure names to file offsets.
// Returns a pointer to the index.  If the index has already been made,
// this function just returns it.  The index is owned by the parser and
// is valid only so long as the parser exists.  You can pass this index
// to ConvertGdsToAscii() when you want to print a single structure.
// The index lets it skip directly to the structure.

FileIndex*
GdsParser::makeIndex()
{
    if (structIndex.isDone())
        return (&structIndex);

    // Zip through the file looking for the BGNSTR records that begin
    // each structure.  Following each should be an STRNAME record
    // containing the name of the structure.  Store the name and
    // BGNSTR's offset in the index.

    GdsRecord   rec;
    scanner.seekTo(0);

    for (;;) {
        readNextRecord(&rec);
        switch (rec.getRecordType()) {
            case GRT_BGNSTR: {
                off_t  offset = rec.getOffset();
                readNextRecord(GRT_STRNAME, &rec);
                const char*  sname = parseStrnameRecord(rec);
                registerStructure(rec, sname, offset);
                break;
            }
            case GRT_ENDLIB:
                goto END_LOOP;

            default:            // avoid complaints about unused enumerators
                break;
        }
    }
  END_LOOP:

    structIndex.setDone();
    return (&structIndex);
}



// buildStructureGraph -- build DAG showing which structure references which.
//   gbuilder   the object that actually builds the graph
// This function does not verify that the input file is valid.  It makes
// only just enough checks to ensure an invalid GDSII file does not
// cause it to generate an invalid sequence of calls to GdsGraphBuilder
// methods.

void
GdsParser::buildStructureGraph (GdsGraphBuilder* gbuilder)
{
    GdsRecord   rec;

    // Look for LIBNAME before the main loop to ensure that
    // beginLibrary() is called first.

    scanner.seekTo(0);
    do {
        readNextRecord(&rec);
    } while (rec.getRecordType() != GRT_LIBNAME);
    const char* libname = copyStringFromRecord(rec);
    gbuilder->beginLibrary(libname);

    // The main loop generates these calls:
    //     { enterStructure() { addSref() }* }*
    // The flag inStructure ensures that addSref() is not invoked before
    // enterStructure().

    bool  inStructure = false;
    for (;;) {
        readNextRecord(&rec);
        switch (rec.getRecordType()) {
            case GRT_BGNSTR: {
                off_t  offset = rec.getOffset();
                readNextRecord(GRT_STRNAME, &rec);
                const char*  sname = parseStrnameRecord(rec);
                // We might as well build the index while we are at it.
                registerStructure(rec, sname, offset);
                gbuilder->enterStructure(sname);
                inStructure = true;
                break;
            }
            case GRT_SNAME: {
                if (! inStructure)
                    abortParser(rec, "SNAME record outside a structure");
                const char*  sname = copyStringFromRecord(rec);
                gbuilder->addSref(sname);
                strPool.clear();        // don't let these strings pile up
                break;
            }
            case GRT_ENDSTR:
                inStructure = false;
                break;

            case GRT_ENDLIB:
                goto END_LOOP;

            default:            // avoid complaints about unused enumerators
                break;
        }
    }
  END_LOOP:

    gbuilder->endLibrary();
    structIndex.setDone();
}



//----------------------------------------------------------------------
// Private methods to parse pieces of the file.  This is a
// straightforward recursive-descent parser.  Typically there is a
// function for each production.
//
// Note that functions that read string records do not verify that the
// string's length is valid.  There is no need to do so because the
// scanner has already verified that the record body's length is valid.
// The length of the record body is the length of the string rounded up
// to the next even number.  Relying on the scanner's check assumes that
// all the max string lengths are even.


// parseLibraryHeader -- parse preliminary gunk before the structure defs.
// Invokes builder->beginLibrary() with all the info.
// Precondition:
//    The last record read was HEADER.
// Postcondition:
//    The last record read was UNITS.

void
GdsParser::parseLibraryHeader()
{
    GdsRecord           rec;
    GdsLibraryOptions   options;

    // <libheader> ::= BGNLIB [LIBDIRSIZE] [SRFNAME] [LIBSECUR]
    //                 LIBNAME [REFLIBS] [FONTS] [ATTRTABLE] [GENERATIONS]
    //                 [<FormatType>] UNITS

    // BGNLIB contains the last-modified and last-accessed times of the
    // library.  Each date is a 6-tuple of shorts.  For
    // builder->beginLibrary() the file pos is the offset of BGNLIB.

    readNextRecord(GRT_BGNLIB, &rec);
    locator.setOffset(rec.getOffset());
    GdsDate  modTime = readDateFromRecord(rec);
    GdsDate  accTime = readDateFromRecord(rec);
    readNextRecord(&rec);

    // LIBDIRSIZE contains the number of pages in the library directory
    // (whatever that is).

    if (rec.getRecordType() == GRT_LIBDIRSIZE) {
        options.setLibdirsize(rec.nextShort());
        readNextRecord(&rec);
    }

    // SRFNAME contains the name of the spacing rules file, if one is
    // bound to the library.

    if (rec.getRecordType() == GRT_SRFNAME) {
        // XXX: check name?
        options.setSrfname(copyStringFromRecord(rec));
        readNextRecord(&rec);
    }

    // LIBSECUR contains an array of Access Control List (ACL) data.
    // Each ACL entry consists of a group number, a user number, and
    // access rights.  Up to MaxAclEntries ACL entries may be present.
    // We explicitly add the flag REC_LIBSECUR to distinguish between
    // an empty LIBSECUR record and no LIBSECUR records at all.

    if (rec.getRecordType() == GRT_LIBSECUR) {
        options.add(GdsLibraryOptions::REC_LIBSECUR);
        for (int j = rec.numItems()/3;  --j >= 0;  ) {
            int  groupid = rec.nextShort();
            int  userid  = rec.nextShort();
            int  rights  = rec.nextShort();
            options.addAclEntry(GdsAclEntry(groupid, userid, rights));
        }
        readNextRecord(&rec);
    }

    // LIBNAME contains the library name.  It must follow UNIX pathname
    // conventions for length and valid characters and can include the
    // file extension.

    verifyRecordType(GRT_LIBNAME, rec);
    const char*  libraryName = copyStringFromRecord(rec);
    if (*libraryName == Cnul)
        warn(rec, "empty library name");
    readNextRecord(&rec);

    // REFLIBS contains the pathnames of between 2 and MaxRefLibs
    // reference libraries.  Each pathname is padded with NULs to 44
    // bytes.  The first or second name can be empty.

    if (rec.getRecordType() == GRT_REFLIBS) {
        int  numItems = rec.numItems();
        if (numItems < MinReflibs  ||  numItems > MaxReflibs)
            abortParser(rec, "invalid number of libraries: %d", numItems);
        // XXX: verify pathnames 3..numItems are nonempty?
        while (--numItems >= 0)
            options.addReflib(copyStringFromRecord(rec));
        readNextRecord(&rec);
    }

    // FONTS contains the pathnames of the text font definition files.
    // Contains exactly four names, the last three of which may be empty.
    // Each name is padded with NULs to 44 bytes.

    if (rec.getRecordType() == GRT_FONTS) {
        const char* f0 = copyStringFromRecord(rec);
        const char* f1 = copyStringFromRecord(rec);
        const char* f2 = copyStringFromRecord(rec);
        const char* f3 = copyStringFromRecord(rec);
        if (*f0 == Cnul)
            abortParser(rec, "first font must be non-empty");
        options.setFonts(f0, f1, f2, f3);
        readNextRecord(&rec);
    }

    // ATTRTABLE contains the pathname of the attribute definition file,
    // if one is bound to the library.

    if (rec.getRecordType() == GRT_ATTRTABLE) {
        options.setAttrtable(copyStringFromRecord(rec));
        readNextRecord(&rec);
    }

    // GENERATIONS contains the number of deleted or back-up structures
    // to retain.  Some GDSII files contain the invalid value 0, and it
    // is unlikely that any tool uses this record; so accept all values
    // without checking.

    if (rec.getRecordType() == GRT_GENERATIONS) {
        options.setGenerations(rec.nextShort());
        readNextRecord(&rec);
    }

    // FORMAT specifies what type of file this is -- one of the GdsFormat
    // enumerators.  For filtered files, the MASK records specify which
    // layers and datatypes appear in the file.
    // <FormatType> ::=  FORMAT  |  FORMAT {MASK}+ ENDMASKS

    if (rec.getRecordType() == GRT_FORMAT) {
        GdsFormat  format = parseFormatRecord(rec);
        options.setFormat(format);

        // The MASK records must appear iff FORMAT is one of the filtered
        // stream formats, GDSII_Filtered or EDSIII_Filtered.

        if (format == GDSII_Filtered  ||  format == EDSIII_Filtered) {
            readNextRecord(GRT_MASK, &rec);
            do {
                options.addMask(parseMaskRecord(rec));
                readNextRecord(&rec);
            } while (rec.getRecordType() == GRT_MASK);
            verifyRecordType(GRT_ENDMASKS, rec);
        }
        readNextRecord(&rec);
    }

    // UNITS contains two 8-byte reals: the number of database units in
    // one user unit and the number of database units in one meter.

    verifyRecordType(GRT_UNITS, rec);
    double  dbToUser  = rec.nextDouble();
    double  dbToMeter = rec.nextDouble();
    GdsUnits  units(dbToUser, dbToMeter);

    builder->beginLibrary(libraryName, modTime, accTime, units, options);
}



// parseStructure -- parse all records in a structure definition.
//   bgnstrRecord       the BGNSTR record that begins the structure
//                      definition
// Invokes the builder's beginStructure() and endStructure() methods
// and (indirectly) its methods for adding elements and properties.
//
// Precondition:
//   The last record read was BGNSTR.
// Postcondition:
//   The last record read was ENDSTR.

void
GdsParser::parseStructure (/*in*/ GdsRecord& bgnstrRecord)
{
    GdsRecord           rec;
    GdsStructureOptions options;                // for STRCLASS

    // <structure> ::= BGNSTR STRNAME [STRCLASS] {<element>}* ENDSTR
    // <element>   ::= {<boundary> | <path> | <sref> | <aref> |
    //                  <text> | <node> | <box>}  {<property>}*  ENDEL

    off_t       strOffset = bgnstrRecord.getOffset();
    GdsDate     createTime = readDateFromRecord(bgnstrRecord);
    GdsDate     modTime    = readDateFromRecord(bgnstrRecord);

    // Get the name of the structure from the following STRNAME record.
    // Set the locator's fields immediately so that abortParser() will
    // have a structure name to print if it gets called.

    readNextRecord(GRT_STRNAME, &rec);
    const char*  sname = parseStrnameRecord(rec);
    locator.setStructureName(sname);
    locator.setOffset(strOffset);

    // Add the structure name to the index.  registerStructure() does
    // nothing if the index has already been built.

    registerStructure(rec, sname, strOffset);

    readNextRecord(&rec);
    if (rec.getRecordType() == GRT_STRCLASS) {
        options.setStrclass(rec.nextBitArray());
        readNextRecord(&rec);
    }

    builder->beginStructure(sname, createTime, modTime, options);

    for (;;) {
        currElemType = rec.getRecordType();
        locator.setOffset(rec.getOffset());
        switch (currElemType) {
            case GRT_BOUNDARY:  parseBoundary();    break;
            case GRT_PATH:      parsePath();        break;
            case GRT_SREF:      parseSref();        break;
            case GRT_AREF:      parseAref();        break;
            case GRT_TEXT:      parseText();        break;
            case GRT_NODE:      parseNode();        break;
            case GRT_BOX:       parseBox();         break;

            case GRT_ENDSTR:
                goto END_LOOP;

            default:
                abortParser(rec, "expected element record");
        }
        strPool.clear();        // discard temporary strings at end of element
        readNextRecord(&rec);
    }
  END_LOOP:

    builder->endStructure();
    locator.setStructureName(Null);
}



// Now follow functions to parse elements.  For each element type FOO
// there is a function parseFoo() that parses the records in the element
// and invokes builder->beginFoo().  The last statement of each parseFoo()
// is a call to parseProperties(), which parses the element properties,
// invokes builder->addProperty() for each, and then reads the ENDEL.
//
// The precondition for each function is that the last record read was
// the one that begins the element (e.g. BOUNDARY).  The postcondition
// is that last record read was ENDEL.
//
// In the GDSII grammar the DATATYPE, BOXTYPE, NODETYPE, and TEXTTYPE
// records are mandatory in the elements that contain them.  However,
// some real-life GDSII files omit them.  So we make them optional, with
// the value defaulting to 0.


void
GdsParser::parseBoundary()
{
    // <boundary> ::= BOUNDARY [ELFLAGS] [PLEX] LAYER DATATYPE XY

    GdsElementOptions   options;
    GdsRecord   rec;
    int         layer;
    int         datatype;

    // Parse ELFLAGS and PLEX, and store in rec the record after that.
    parseElementOptions(&rec, &options);

    verifyRecordType(GRT_LAYER, rec);
    layer = parseLayerRecord(rec);
    readNextRecord(&rec);

    // Parse the DATATYPE record if it appears.  As noted earlier, make
    // the record optional although the grammar requires it.

    if (rec.getRecordType() == GRT_DATATYPE) {
        datatype = parseDatatypeRecord(rec);
        readNextRecord(&rec);
    } else {
        warn(rec, "DATATYPE record missing in BOUNDARY; datatype set to 0");
        datatype = 0;
    }

    // Parse the XY record.  The first and last points must coincide.

    verifyRecordType(GRT_XY, rec);
    parseXYRecord(rec, MinBoundaryPoints, MaxBoundaryPoints);
    if (xyPoints.front() != xyPoints.back())
        abortParser(rec, "first point != last point in BOUNDARY");

    // There is no need to set the locator's offset before invoking the
    // builder method because parseStructure() has already done it.

    builder->beginBoundary(layer, datatype, xyPoints, options);
    parseProperties();      // also eats the ENDEL
}



void
GdsParser::parsePath()
{
    // <path> ::= PATH [ELFLAGS] [PLEX] LAYER DATATYPE [PATHTYPE]
    //            [WIDTH] [BGNEXTN] [ENDEXTN] XY

    GdsPathOptions  options;
    GdsRecord   rec;
    int         layer;
    int         datatype;

    // Parse ELFLAGS and PLEX, and store in rec the record after that.
    parseElementOptions(&rec, &options);

    verifyRecordType(GRT_LAYER, rec);
    layer = parseLayerRecord(rec);
    readNextRecord(&rec);

    // Parse the DATATYPE record if it appears.  As noted earlier, make
    // the record optional although the grammar requires it.

    if (rec.getRecordType() == GRT_DATATYPE) {
        datatype = parseDatatypeRecord(rec);
        readNextRecord(&rec);
    } else {
        warn(rec, "DATATYPE record missing in PATH; datatype set to 0");
        datatype = 0;
    }

    GdsPathtype  pathtype = PathtypeFlush;
    if (rec.getRecordType() == GRT_PATHTYPE) {
        pathtype = parsePathtypeRecord(rec);
        options.setPathtype(pathtype);
        readNextRecord(&rec);
    }
    if (rec.getRecordType() == GRT_WIDTH) {
        options.setWidth(rec.nextInt());
        readNextRecord(&rec);
    }

    // Parse the BGNEXTN and ENDEXTN records if they appear.  The spec
    // does not say that a path with pathtype == 4 must contain BGNEXTN
    // and ENDEXTN records, but neither does it give a default value for
    // the extensions.  We default both to 0 in the GdsPathOptions
    // constructor.

    if (rec.getRecordType() == GRT_BGNEXTN) {
        if (pathtype != PathtypeCustom) {
            warn(rec, "BGNEXTN record is not valid for pathtype %d; "
                      "assuming pathtype 4 instead",
                 pathtype);
            pathtype = PathtypeCustom;
            options.setPathtype(PathtypeCustom);
        }
        options.setBeginExtn(rec.nextInt());
        readNextRecord(&rec);
    }
    if (rec.getRecordType() == GRT_ENDEXTN) {
        if (pathtype != PathtypeCustom) {
            warn(rec, "ENDEXTN record is not valid for pathtype %d; "
                      "assuming pathtype 4 instead",
                 pathtype);
            pathtype = PathtypeCustom;
            options.setPathtype(PathtypeCustom);
        }
        options.setEndExtn(rec.nextInt());
        readNextRecord(&rec);
    }

    verifyRecordType(GRT_XY, rec);
    parseXYRecord(rec, MinPathPoints, MaxPathPoints);
    builder->beginPath(layer, datatype, xyPoints, options);
    parseProperties();      // also eats the ENDEL
}



void
GdsParser::parseSref()
{
    // <sref> ::= SREF [ELFLAGS] [PLEX] SNAME [<strans>] XY

    GdsRecord           rec;
    GdsTransform        strans;
    GdsElementOptions   options;

    // Parse ELFLAGS and PLEX, and store in rec the record after that.
    parseElementOptions(&rec, &options);

    verifyRecordType(GRT_SNAME, rec);
    const char*  sname = copyStringFromRecord(rec);
    readNextRecord(&rec);
    if (rec.getRecordType() == GRT_STRANS)
        parseStrans(&rec, &strans);             // gets record after <strans>

    verifyRecordType(GRT_XY, rec);
    parseXYRecord(rec, 1, 1);
    builder->beginSref(sname, xyPoints[0].x, xyPoints[0].y, strans, options);
    parseProperties();      // also eats the ENDEL
}




void
GdsParser::parseAref()
{
    // <aref> ::= AREF [ELFLAGS] [PLEX] SNAME [<strans>] COLROW XY

    GdsRecord           rec;
    GdsTransform        strans;
    GdsElementOptions   options;

    // Parse ELFLAGS and PLEX, and store in rec the record after that.
    parseElementOptions(&rec, &options);

    verifyRecordType(GRT_SNAME, rec);
    const char*  sname = copyStringFromRecord(rec);
    readNextRecord(&rec);
    if (rec.getRecordType() == GRT_STRANS)
        parseStrans(&rec, &strans);             // gets record after <strans>

    // Get the number of rows and columns in the array and verify that both
    // are valid.  There is no need to check the upper limit because that
    // is the signed 16-bit max.
    //
    // XXX: The spec says that each is in the range 0..32767, but I don't
    // see how 0 can be valid.

    verifyRecordType(GRT_COLROW, rec);
    int  numCols = rec.nextShort();
    int  numRows = rec.nextShort();
    if (numCols < 1)
        abortParser(rec, "invalid number of columns %d", numCols);
    if (numRows < 1)
        abortParser(rec, "invalid number of rows %d", numRows);

    // An XY record for an AREF has exactly 3 points.  The first point is
    // the array reference point (origin).  The second point is displaced
    // from the first by the inter-column spacing times the number of
    // columns.  The third is displaced from the first by the inter-row
    // spacing times the number of rows.

    readNextRecord(GRT_XY, &rec);
    parseXYRecord(rec, 3, 3);

    // XXX: We allow the row/col spacing to be negative.  That is,
    // xyPoints[0] can be the top-right corner of the array.  Is that
    // okay?  Even zero spacing is apparently okay.  Real GDSII files
    // occasionally have it (don't ask me why), and the OASIS spec does
    // not forbid it.
    //
    // XXX: Should we check that the row and column directions are
    // not the same?

    // if (numCols > 1  &&  xyPoints[0] == xyPoints[1])
    //    warn(rec, "AREF has zero column-spacing");
    // if (numRows > 1  &&  xyPoints[0] == xyPoints[2])
    //    warn(rec, "AREF has zero row-spacing");

    builder->beginAref(sname, numCols, numRows, xyPoints, strans, options);
    parseProperties();      // also eats the ENDEL
}



void
GdsParser::parseNode()
{
    // <node> ::= NODE [ELFLAGS] [PLEX] LAYER NODETYPE XY

    GdsElementOptions   options;
    GdsRecord   rec;
    int         layer;
    int         nodetype;

    // Parse ELFLAGS and PLEX, and store in rec the record after that.
    parseElementOptions(&rec, &options);

    verifyRecordType(GRT_LAYER, rec);
    layer = parseLayerRecord(rec);
    readNextRecord(&rec);

    // Parse the NODETYPE record if it appears.  As noted earlier, make
    // the record optional although the grammar requires it.

    if (rec.getRecordType() == GRT_NODETYPE) {
        nodetype = rec.nextShort();
        if (nodetype < 0)
            abortParser(rec, "invalid nodetype %d", nodetype);
        readNextRecord(&rec);
    } else {
        warn(rec, "NODETYPE record missing; nodetype set to 0");
        nodetype = 0;
    }

    verifyRecordType(GRT_XY, rec);
    parseXYRecord(rec, MinNodePoints, MaxNodePoints);

    builder->beginNode(layer, nodetype, xyPoints, options);
    parseProperties();      // also eats the ENDEL
}



void
GdsParser::parseBox()
{
    // <box> ::= BOX [ELFLAGS] [PLEX] LAYER BOXTYPE XY

    GdsElementOptions   options;
    GdsRecord   rec;
    int         layer;
    int         boxtype;

    // Parse ELFLAGS and PLEX, and store in rec the record after that.
    parseElementOptions(&rec, &options);

    verifyRecordType(GRT_LAYER, rec);
    layer = parseLayerRecord(rec);
    readNextRecord(&rec);

    // Parse the BOXTYPE record if it appears.  As noted earlier, make
    // the record optional although the grammar requires it.

    if (rec.getRecordType() == GRT_BOXTYPE) {
        boxtype = rec.nextShort();
        if (boxtype < 0)
            abortParser(rec, "invalid boxtype %d", boxtype);
        readNextRecord(&rec);
    } else {
        warn(rec, "BOXTYPE record missing; boxtype set to 0");
        boxtype = 0;
    }

    verifyRecordType(GRT_XY, rec);
    parseXYRecord(rec, 5, 5);
    if (xyPoints.front() != xyPoints.back())
        abortParser(rec, "first point != last point in BOX");

    // Verify that the box coordinates form a Manhattan rectangle.  Skip
    // either the last point or the first point to make the first edge
    // vertical.  This halves the work we must do.

    GdsPoint*  pt = &*xyPoints.begin(); // get address of first point
    if (pt[0].x != pt[1].x)             // if edge 0-1 is not vertical,
        ++pt;                           // edge 1-2 must be; make it 0-1

    if (pt[0].x != pt[1].x
            ||  pt[2].x != pt[3].x
            ||  pt[0].y != pt[3].y
            ||  pt[1].y != pt[2].y)
        abortParser(rec, "BOX coordinates do not form a Manhattan rectangle");

    builder->beginBox(layer, boxtype, xyPoints, options);
    parseProperties();      // also eats the ENDEL
}



void
GdsParser::parseText()
{
    // <text> ::= TEXT [ELFLAGS] [PLEX] LAYER TEXTTYPE [PRESENTATION]
    //            [PATHTYPE] [WIDTH] [<strans>] XY STRING

    GdsTransform        strans;
    GdsTextOptions      options;
    GdsRecord   rec;
    int         layer;
    int         texttype;

    // Parse ELFLAGS and PLEX, and store in rec the record after that.
    parseElementOptions(&rec, &options);

    verifyRecordType(GRT_LAYER, rec);
    layer = parseLayerRecord(rec);
    readNextRecord(&rec);

    // Parse the TEXTTYPE record if it appears.  As noted earlier, make
    // the record optional although the grammar requires it.

    if (rec.getRecordType() == GRT_TEXTTYPE) {
        texttype = rec.nextShort();
        if (texttype < 0)
            abortParser(rec, "invalid texttype %d", texttype);
        readNextRecord(&rec);
    } else {
        warn(rec, "TEXTTYPE record missing; texttype set to 0");
        texttype = 0;
    }

    if (rec.getRecordType() == GRT_PRESENTATION) {
        parsePresentationRecord(rec, &options);
        readNextRecord(&rec);
    }
    if (rec.getRecordType() == GRT_PATHTYPE) {
        options.setPathtype(parsePathtypeRecord(rec));
        readNextRecord(&rec);
    }
    if (rec.getRecordType() == GRT_WIDTH) {
        options.setWidth(rec.nextInt());
        readNextRecord(&rec);
    }
    if (rec.getRecordType() == GRT_STRANS)
        parseStrans(&rec, &strans);             // gets record after <strans>

    verifyRecordType(GRT_XY, rec);
    parseXYRecord(rec, 1, 1);
    readNextRecord(GRT_STRING, &rec);
    const char*  text = copyStringFromRecord(rec);

    builder->beginText(layer, texttype, xyPoints[0].x, xyPoints[0].y,
                       text, strans, options);
    parseProperties();      // also eats the ENDEL
}


//----------------------------------------------------------------------
// Low-level methods follow.  Some of these read sequences of records
// common to several elements, or read and validate a single record.


// readDateFromRecord -- read date from BGNLIB or BGNSTR record.
// These records each contain two dates.  Each date is stored as
// six consecutive 2-byte integers.  This function has the side effect
// of modifying the record's read cursor.

GdsDate
GdsParser::readDateFromRecord (/*inout*/ GdsRecord& rec)
{
    GdsDate     gdate;

    gdate.year   = rec.nextShort();
    gdate.month  = rec.nextShort();
    gdate.day    = rec.nextShort();
    gdate.hour   = rec.nextShort();
    gdate.minute = rec.nextShort();
    gdate.second = rec.nextShort();
    return gdate;
}



// parseStrnameRecord -- read structure name from STRNAME record and check it.
// Stores the name in strPool (with terminating NUL) and returns that
// after checking it for validity.  Note that strPool's contents are
// temporary.

const char*
GdsParser::parseStrnameRecord (/*in*/ GdsRecord& rec)
{
    // Insert the name in the StringPool for structure names; this adds
    // the terminating NUL.  Then verify that the name is non-empty and
    // all characters in the name are valid.  Many files violate the
    // GDSII naming rules, so check only that all chars are visible
    // ASCII.

    const char*  sname = copyStringFromRecord(rec);
    if (*sname == Cnul)
        abortParser(rec, "empty structure name");

    for (const char* cp = sname;  *cp != Cnul;  ++cp) {
        Uchar  uch = static_cast<Uchar>(*cp);
        if (uch < 0x21  ||  uch > 0x7e) {
            char  cstr[10];     // printable representation of illegal char
            CharToString(cstr, sizeof cstr, uch);
            abortParser(rec, "invalid character '%s' in name", cstr);
        }
    }
    return sname;
}



// registerStructure -- add (structure-name, offset) pair to structIndex.
//   rec        the structure's STRNAME record.  Used for the error message.
//   sname      structure name (must be NUL-terminated)
//   offset     offset in file of structure's BGNSTR record
// This function does nothing if the index has already been built.

void
GdsParser::registerStructure (const GdsRecord& rec,
                              const char* sname, off_t offset)
{
    assert (rec.getRecordType() == GRT_STRNAME);

    if (structIndex.isDone())
        return;
    if (! structIndex.addEntry(sname, offset)) {
        llong  prevOffset = structIndex.getOffset(sname);
        abortParser(rec, "structure %s redefined: "
                         "original definition at offset %lld",
                    sname, prevOffset);
    }
}



// parseElementOptions -- parse the ELFLAGS and PLEX records if they appear.
//   recp       out: the record in the input stream after the ELFLAGS
//              and PLEX is stored here.
//   options    out: the values in the ELFLAGS and PLEX records are stored
//              here.
// Precondition:
//    The last record read was one that begins an element,
//    e.g., BOUNDARY, PATH, SREF.

void
GdsParser::parseElementOptions (/*out*/ GdsRecord* recp,
                                /*out*/ GdsElementOptions* options)
{
    readNextRecord(recp);
    if (recp->getRecordType() == GRT_ELFLAGS) {
        Uint  elflags = recp->nextBitArray();
        bool isExternal = (elflags & 0x02);
        bool isTemplate = (elflags & 0x01);
        if (elflags & 0xfffc)
            warn(*recp, "unused bits set (must be cleared): %#.4x", elflags);
        options->setElflags(isExternal, isTemplate);
        readNextRecord(recp);
    }

    if (recp->getRecordType() == GRT_PLEX) {
        int  plex = recp->nextInt();
        if (plex & ~0xffffff)           // high byte must be 0
            abortParser(*recp, "plex number %d too large", plex);
        options->setPlex(plex);
        readNextRecord(recp);
    }
}



// parseXYRecord -- store contents of XY record in member xyPoints.
//   xyRec      must be an XY record
//   minPoints  the minimum valid number of points for this element type
//   maxPoints  the maximum valid number of points for this element type

void
GdsParser::parseXYRecord (/*in*/ GdsRecord& xyRec,
                          int minPoints, int maxPoints)
{
    assert (xyRec.getRecordType() == GRT_XY);
    int  numPoints = xyRec.numItems() / 2;
    if (numPoints < minPoints  ||  numPoints > maxPoints) {
        abortParser(xyRec, "invalid number of points (%d) for %s element",
                    numPoints, GdsRecordTypeInfo::getName(currElemType));
    }

    xyPoints.clear();
    while (--numPoints >= 0) {
        int  x = xyRec.nextInt();
        int  y = xyRec.nextInt();
        xyPoints.addPoint(x, y);
    }
}



// parseProperties -- parse the property list of an element, if any.
// Invokes the builder's addProperty() method for each property,
// and finally its endElement() method.
// Precondition:
//      The next record to be read must be ENDEL or the PROPATTR of
//      the element's first property.
// Postcondition:
//      The last record read was ENDEL.

void
GdsParser::parseProperties()
{
    // <element>  ::= {<boundary> | <path> | ... }  {<property>}*  ENDEL
    // <property> ::= PROPATTR PROPVALUE
    //
    // The spec for PROPVALUE requires the PROPATTRs for any element to
    // be unique, and limits the total property data size for each
    // element to 512 bytes.  We ignore the uniqueness requirement
    // because some files have duplicate PROPATTRs.  We ignore the limit
    // because it does not seem to be useful.  We also allow PROPATTR 0
    // although the spec says that the range is 1..127.

    // In each iteration of the loop we recognize a PROPATTR-PROPVALUE
    // pair and pass it to the builder.  The loop ends with an ENDEL.

    GdsRecord  rec;
    for (;;) {
        readNextRecord(&rec);
        locator.setOffset(rec.getOffset());
        if (rec.getRecordType() == GRT_ENDEL)
            break;

        verifyRecordType(GRT_PROPATTR, rec);
        int  attr = rec.nextShort();
        if (attr < MinPropAttr  ||  attr > MaxPropAttr)
            abortParser(rec, "invalid attribute %d", attr);

        // Read the PROPVALUE record.  If the property value already has
        // a NUL at the end (padding to make the length even), use the
        // value directly.  Otherwise make a local copy and terminate
        // with a NUL.  Copying to a local array is even faster than
        // copying to strPool.

        readNextRecord(GRT_PROPVALUE, &rec);
        const char*  value = rec.nextString();
        Uint  len = rec.getLength();
        char  valueCopy[MaxPropValueLength + 1];

        if (len == 0  ||  value[len-1] != Cnul) {
            memcpy(valueCopy, value, len);
            valueCopy[len] = Cnul;
            value = valueCopy;
        }
        builder->addProperty(attr, value);
    }

    builder->endElement();
}



// parseStrans -- parse a transform specification
//   recp       in: must be an STRANS record.
//              out: the record after the transform is stored here.
//   strans     out: the transform is stored here
//
// Precondition:
//   The last record read was STRANS.
// Postcondition:
//   The last record read was whatever follows the <strans>.

void
GdsParser::parseStrans (/*inout*/ GdsRecord* recp,
                        /*out*/ GdsTransform* strans)
{
    // <strans> ::= STRANS [MAG] [ANGLE]

    assert (recp->getRecordType() == GRT_STRANS);

    // Save the contents of the STRANS bit array record.  Only three
    // bits (0x8006) are defined in it.  The spec says that the unused
    // bits (0x7ff9) must be clear, but there is no point in checking
    // that.

    Uint  bitArray = recp->nextBitArray();
    bool  reflectX = (bitArray & 0x8000);
    bool  absMag   = (bitArray & 0x0004);
    bool  absAngle = (bitArray & 0x0002);
    strans->setStrans(reflectX, absMag, absAngle);
    readNextRecord(recp);

    // Although the grammar requires MAG to appear before ANGLE, some
    // files have them switched.  Duplicate the code for ANGLE so that
    // both orders are accepted.

    if (recp->getRecordType() == GRT_ANGLE) {
        strans->setAngle(recp->nextDouble());
        readNextRecord(recp);
    }
    if (recp->getRecordType() == GRT_MAG) {
        double  mag = recp->nextDouble();
        // Accept mag 0.0 because some real-life GDSII files have that.
        if (mag < 0.0) {
            const char*  elemName = GdsRecordTypeInfo::getName(currElemType);
            abortParser(*recp, "invalid magnification %.10g in %s element",
                        mag, elemName);
        }
        strans->setMag(mag);
        readNextRecord(recp);
    }
    if (recp->getRecordType() == GRT_ANGLE) {
        if (strans->contains(GdsTransform::REC_ANGLE)) {
            const char*  elemName = GdsRecordTypeInfo::getName(currElemType);
            abortParser(*recp, "duplicate ANGLE record in %s element",
                        elemName);
        }
        strans->setAngle(recp->nextDouble());
        readNextRecord(recp);
    }
}



// parsePresentationRecord -- extract and validate contents of PRESENTATION.

void
GdsParser::parsePresentationRecord (/*in*/ GdsRecord& rec,
                                    /*out*/ GdsTextOptions* options)
{
    assert (rec.getRecordType() == GRT_PRESENTATION);

    Uint  bitArray = rec.nextBitArray();
    Uint  font  = (bitArray & 0x30) >> 4;
    Uint  vjust = (bitArray & 0x0c) >> 2;
    Uint  hjust = (bitArray & 0x03);

    if (vjust == 3  ||  hjust == 3)
        abortParser(rec, "invalid presentation %#.4x", bitArray);
    if (bitArray & 0xffc0)
        warn(rec, "unused bits set (must be cleared): %#.4x", bitArray);

    options->setPresentation(font, vjust, hjust);
}



// parseMaskRecord -- extract and validate contents of MASK.

const char*
GdsParser::parseMaskRecord (/*in*/ GdsRecord& rec)
{
    assert (rec.getRecordType() == GRT_MASK);

    // XXX: No validation for now.
    // The contents of the MASK record are not specified all that precisely.

    const char*  mask = copyStringFromRecord(rec);
    return mask;
}



//----------------------------------------------------------------------
// Handling errors


// abortParser -- abort parsing because of invalid record contents
//   rec        the invalid record
//   fmt        printf() format string for error message
//   ...        arguments (if any) for the format string

void
GdsParser::abortParser (const GdsRecord& rec, const char* fmt, ...)
{
    char  buf[256];

    va_list  ap;
    va_start(ap, fmt);
    formatMessage(buf, sizeof buf, &rec, "", fmt, ap);
    va_end(ap);

    ThrowRuntimeError("%s", buf);
}



// abortParserNoContext -- abort parsing because of general (non-record) error
//   fmt        printf() format string for error message
//   ...        arguments (if any) for the format string
// This is for errors that occur outside the context of a record.

void
GdsParser::abortParserNoContext (const char* fmt, ...)
{
    char  buf[256];

    va_list  ap;
    va_start(ap, fmt);
    formatMessage(buf, sizeof buf, Null, "", fmt, ap);
    va_end(ap);

    ThrowRuntimeError("%s", buf);
}



// warn -- warn about invalid record contents
//   rec        the invalid record
//   fmt        printf() format string for warning message
//   ...        arguments (if any) for the format string
// If the warning handler is non-Null, it is invoked with the
// warning message.

void
GdsParser::warn (const GdsRecord& rec, const char* fmt, ...)
{
    char  buf[256];

    va_list  ap;
    va_start(ap, fmt);
    formatMessage(buf, sizeof buf, &rec, "warning: ", fmt, ap);
    va_end(ap);

    if (warnHandler != Null)
        warnHandler(buf);
}



// formatMessage -- format error/warning message in buffer
//   buf        the buffer in which the message is to be formatted
//   bufsize    bytes available in buf
//   recp       pointer to record currently being processed, or Null
//   msgPrefix  string to put in the message between the context and
//              the message proper
//   fmt        printf() format string for message
//   ap         varargs arg pointer for the format arguments, if any
//
// The full context string at the beginning of the message looks like this:
//
//     file 'foo', in structure 'bar', BAZ record at offset NNN:
//
// The structure phrase is omitted if the structure is Null in the
// locator.  The record phrase is omitted if recp is Null.
//
// The error message may be truncated if the buffer is not big enough.

void
GdsParser::formatMessage (/*out*/ char* buf, size_t bufsize,
                          const GdsRecord*  recp,
                          const char*  msgPrefix,
                          const char*  fmt,
                          va_list  ap)
{
    Uint  n = SNprintf(buf, bufsize, "file '%s'", locator.getFileName());

    const char*  strName = locator.getStructureName();
    if (n < bufsize - 1  &&  strName != Null)
        n += SNprintf(buf+n, bufsize-n, ", structure '%s'", strName);

    if (n < bufsize - 1  &&  recp != Null) {
        llong  recOffset = recp->getOffset();
        n += SNprintf(buf+n, bufsize-n, ", %s record at offset %lld",
                      recp->getRecordName(), recOffset);
    }
    if (n < bufsize - 1)
        n += SNprintf(buf+n, bufsize-n, ": %s", msgPrefix);
    if (n < bufsize - 1)
        VSNprintf(buf+n, bufsize-n, fmt, ap);
}


}  // namespace Gdsii
