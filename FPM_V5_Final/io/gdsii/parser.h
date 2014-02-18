// gdsii/parser.h -- high-level interface for reading GDSII Stream files
//
// last modified:   07-Mar-2010  Sun  21:36
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// The parser does not accept the following GDSII records because they
// do not appear in the EBNF grammar given in the GDSII specification.
//
//     20  TEXTNODE              53  RESERVED
//     24  SPACING               60  BORDER
//     29  UINTEGER              61  SOFTFENCE
//     30  USTRING               62  HARDFENCE
//     36  STYPTABLE             63  SOFTWIRE
//     37  STRTYPE               64  HARDWIRE
//     39  ELKEY                 65  PATHPORT
//     40  LINKTYPE              66  NODEPORT
//     41  LINKKEYS              67  USERCONSTRAINT
//     50  TAPENUM               68  SPACER ERROR
//     51  TAPECODE              69  CONTACT
//
// The parser accepts the following extensions to the GDSII specification.
// The extensions are needed to parse files created by other tools.
//
//   - For structure names, the spec allows only letters, digits, and the
//     the characters '_', '?', and '$'.  This parser allows all graphic
//     (visible) ASCII characters.
//
//   - The spec limits structure names to 32 characters, text strings to
//     512 characters, property values to 126 bytes, and ATTRTABLE
//     contents to 44 bytes.  This parser allows up to 65530 bytes for
//     all.
//
//   - Empty (0-byte) LIBNAME records are accepted.
//
//   - Empty ATTRTABLE records are accepted.
//
//   - In BOUNDARY and PATH elements the DATATYPE record is mandatory.
//     This parser, however, accepts files in which the record is
//     omitted, setting the datatype to 0.  It handles TEXTTYPE,
//     NODETYPE, and BOXTYPE records similarly.
//
//   - The spec limits XY records to 600 points for BOUNDARY elements
//     and 200 points for PATH elements.  This parser accepts up to 8191
//     points for both.
//
//   - In the GENERATIONS record, the spec says that the legal range is
//     2..99.  This parser accepts all values.
//
//   - In the FONTS record, the spec seems to say that at least one of
//     the fonts must be non-empty (the language is not clear).  This
//     parser does not check that.
//
//   - When a transform specification has both MAG and ANGLE, the grammar
//     in the spec requires MAG to precede ANGLE.  This parser accepts
//     the reverse order too.
//
//   - For PATH elements, this parser replaces invalid values of PATHTYPE
//     by 0.  If a BGNEXTN or ENDEXTN record appears in a path whose
//     type is 0, 1, or 2, the parser forces the type to 4.  It defaults
//     the begin and end extensions to 0.
//
//   - For PROPATTR and PROPVALUE records, the parser ignores several
//     restrictions of the spec.  It allows PROPATTR 0, it does not
//     require the PROPATTRs for an element to have distinct values, and
//     it does not restrict the length of property values.


#ifndef GDSII_PARSER_H_INCLUDED
#define GDSII_PARSER_H_INCLUDED

#include <new>
#include <cstdarg>
#include <string>
#include <sys/types.h>          // for off_t

#include "misc/stringpool.h"
#include "misc/utils.h"
#include "builder.h"
#include "file-index.h"
#include "scanner.h"


namespace Gdsii {

using std::bad_alloc;

using SoftJin::Ulong;
using SoftJin::FileHandle;
using SoftJin::StringPool;
using SoftJin::WarningHandler;


/*----------------------------------------------------------------------
GdsGraphBuilder -- builder for DAG of structure references.
Structures in GDSII Stream files may contain references (in SREF and
AREF records) to other structures.  The structures thus form a
directed acyclic graph (DAG).  To build the structure DAG for a GDSII
file, derive from this class and pass an instance of the derived
class to GdsParser::buildStructureGraph(), which invokes the builder's
methods as described below.

beginLibrary()
        is invoked with the library name (from the LIBNAME record)
        as its argument.

enterStructure()
        is invoked for each BGNSTR record seen, with the structure name
        (from the STRNAME record) as its argument.  This function should
        create a graph vertex for its argument and save it.  The saved
        vertex will be the source vertex for all edges created until the
        next call to enterStructure().

addSref()
        is invoked for each SREF and AREF record seen, with the
        name of the referenced structure (from the SNAME record)
        as its argument.  This should retrieve the vertex corresponding
        to its argument, creating one if needed.  It should then create
        an edge from the saved source vertex to this one.

endLibrary()
        is invoked at the end of the library.

All the arguments are NUL-terminated and point to temporary space
space that is reclaimed when the function returns.  Make copies if
you want to keep the strings.

GdsParser::buildStructureGraph() invokes the builder's methods
in this order:

    beginLibrary()
        { enterStructure() { addSref() }* }*
    endLibrary()
------------------------------------------------------------------------*/

class GdsGraphBuilder {
public:
                  GdsGraphBuilder();
    virtual       ~GdsGraphBuilder();

    virtual void  beginLibrary (const char* libname);
    virtual void  enterStructure (const char* sname);
    virtual void  addSref (const char* sname);
    virtual void  endLibrary();

private:
                GdsGraphBuilder (const GdsGraphBuilder&);       // forbidden
    void        operator= (const GdsGraphBuilder&);             // forbidden
};



/*----------------------------------------------------------------------
GdsParser -- parse a GDSII Stream file
GdsParser builds on the low-level interface of GdsScanner to present
a GDSII Stream file as a sequence of structures, each of which
contains a sequence of elements, each of which contains geometry data
and a sequence of properties.  A GdsBuilder object is the primary
interface between the parser and the application.  The parser invokes
the builder's virtual methods when it parses a structure, element,
property, etc.

All methods apart from the destructor may throw runtime_error
or bad_alloc.

warnHandler     WarningHandler

        Handler for minor errors discovered in the input file.
        If non-Null, it is invoked with the warning message.
        If Null, the minor errors are ignored.

strPool         StringPool

        String pool to store general strings temporarily.  This is used for
        two purposes.

          - Strings in GDSII records need not be NUL-terminated,
            but we provide NUL-terminated strings to the application to
            make its job easier.  We must therefore make a local copy of
            the string so that we can append the NUL.

          - To save the string while reading more records.

        Currently this is used only for the strings in SREF, AREF, and
        STRING.  We use a StringPool rather than the string class to
        avoid repeated new/delete.  The contents of this pool are guaranteed
        to remain only until the next ENDEL.

structIndex     FileIndex

        Mapping from structure names to the file offsets where their
        BGNSTR record begins.  Used to provide random access to structures.

xyPoints        GdsPointList

        The points in the last XY record parsed.  We keep reusing this
        member, instead of using a local variable, to avoid repeatedly
        constructing and destroying the vector in PointList.

currElemType    GdsRecordType

        If an element is being parsed, this contains the record type
        of the element (e.g., GRT_BOUNDARY).  Otherwise its value is
        undefined.  parseProperties() uses this to calculate the limit
        on the total length of properties; this depends on the element.
        parseStrans() uses it for any warning or error message.

locator         GdsLocator

        Identifies the file being parsed and the parser's position in
        it.  Intended mainly for the builder (setLocator() gives the
        builder a const ref to this object), but also used by
        formatMessage() for the error context.

------------------------------------------------------------------------*/


class GdsParser {
    GdsScanner          scanner;
    GdsBuilder*         builder;
    WarningHandler      warnHandler;
    FileIndex           structIndex;
    StringPool          strPool;
    GdsPointList        xyPoints;
    GdsRecordType       currElemType;
    GdsLocator          locator;

public:
                GdsParser (const char* fname, FileHandle::FileType ftype,
                           WarningHandler);
                ~GdsParser();

    void        parseFile (GdsBuilder*);
    bool        parseStructure (const char* sname, GdsBuilder*);
    FileIndex*  makeIndex();
    void        buildStructureGraph (GdsGraphBuilder* gbuilder);

private:
    void        verifyRecordType (GdsRecordType recType, const GdsRecord& rec);
    void        readNextRecord (/*out*/ GdsRecord* recp);
    void        readNextRecord (GdsRecordType recType,
                                /*out*/ GdsRecord* recp);
    int         parseLayerRecord (/*in*/ GdsRecord& rec);
    int         parseDatatypeRecord (/*in*/ GdsRecord& rec);
    GdsPathtype parsePathtypeRecord (/*in*/ GdsRecord& rec);
    GdsFormat   parseFormatRecord (/*in*/ GdsRecord& rec);
    const char* copyStringFromRecord (/*in*/ GdsRecord& rec);

    void        parseLibraryHeader();
    void        parseStructure (/*in*/ GdsRecord& bgnstrRecord);
    void        parseBoundary();
    void        parsePath();
    void        parseSref();
    void        parseAref();
    void        parseNode();
    void        parseBox();
    void        parseText();

    GdsDate     readDateFromRecord (/*inout*/ GdsRecord& rec);
    const char* parseStrnameRecord (/*in*/ GdsRecord& rec);
    void        registerStructure (const GdsRecord& rec,
                                   const char* sname, off_t offset);
    void        parseElementOptions (/*out*/ GdsRecord* recp,
                                     /*out*/ GdsElementOptions* options);
    void        parseXYRecord (/*in*/ GdsRecord& xyRec,
                               int minPoints, int maxPoints);
    void        parseProperties();
    void        parseStrans (/*inout*/ GdsRecord* recp,
                             /*out*/ GdsTransform* strans);
    void        parsePresentationRecord (/*in*/ GdsRecord& rec,
                                         /*out*/ GdsTextOptions* options);
    const char* parseMaskRecord (/*in*/ GdsRecord& rec);

    void        abortParser (const GdsRecord& rec, const char* fmt, ...)
                                        SJ_PRINTF_ARGS(3,4)  SJ_NORETURN;
    void        abortParserNoContext (const char* fmt, ...)
                                        SJ_PRINTF_ARGS(2,3)  SJ_NORETURN;
    void        warn (const GdsRecord& rec, const char* fmt, ...);
    void        formatMessage (/*out*/ char* buf, size_t bufsize,
                               const GdsRecord*  recp,
                               const char*  msgPrefix,
                               const char*  fmt,
                               va_list  ap);
private:
                GdsParser (const GdsParser&);           // forbidden
    void        operator= (const GdsParser&);           // forbidden
};


}  // namespace Gdsii

#endif  // GDSII_PARSER_H_INCLUDED
