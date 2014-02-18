// oasis/parser.h -- high-level interface for reading OASIS files
//
// last modified:   25-Dec-2009  Fri  10:33
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef OASIS_PARSER_H_INCLUDED
#define OASIS_PARSER_H_INCLUDED

#include "misc/utils.h"         // for WarningHandler
#include "builder.h"

namespace Oasis {

using SoftJin::WarningHandler;


// OasisParserOptions -- options for OasisParser
//
// The flag strictConformance says whether the parser should strictly
// conform to the spec.  Some programs generate non-conforming OASIS
// files; strictConformance must be set to false when parsing such
// files.  Below are some of the checks dropped when it is false.
//
//    - Standard properties are not checked to verify that they appear
//      in the right context.
//
//    - PROPERTY records can contain property names instead of refnums
//      even when the PROPNAME table has the strict flag set, and can
//      contain string values instead of refnums even when the
//      PROPSTRING table has the strict flag set.
//
//    - All characters in the range 0x1 - 0x7f are allowed in a-strings.
//      (The spec restricts the range to 0x20 - 0x7e.)
//
//    - Unused bits are ignored in the info-byte of TEXT, POLYGON,
//      CIRCLE, and XGEOMETRY records.  Normally the parser aborts if
//      any of the unused bits is set.
//
//    - Bytes after the END record are ignored.
//
// The flag wantValidation allows parsing of OASIS files with mangled
// END records.  When this is false, OasisParser ignores the validation
// scheme and signature in the END record.  In the call to
// OasisBuilder::beginFile() it sets valScheme to Validation::None.
//
// Normally the parser begins by parsing both START and END records.
// But if wantValidation is false and table-offsets is in the START
// record, it parses only START.  Thus the END record need not begin
// exactly 256 bytes before the end of the file as the spec requires,
// and need not contain anything but the record ID.
//
// The flags wantText, wantLayerName, and wantExtensions specify whether
// the parser should parse TEXT, TEXTSTRING, LAYERNAME, and extension
// records (and any following PROPERTY records).  Applications that are
// not interested in any of those records should set the corresponding
// flag to false.  This might speed up parsing.  Some OASIS tools set
// strict=0, offset=0 in the table offsets when there are no <name>
// records of a particular kind.  (The strict flag should be 1 instead.)
// Normally the parser makes a preliminary pass to collect <name>
// records if any name table is non-strict.  But if the application does
// not want records of some kind, the parser need not care whether the
// corresponding table is strict because it is going to ignore those
// records.  The name-parsing pass can thus be avoided in some cases.
//
// If any of the wantFoo flags is false, most validity checks on the
// corresponding records are dropped, even if strictConformance is true.
// For example, if wantText is false the parser does not verify that at
// most one of the TEXTSTRING record types 5 and 6 appears in the file.
// But OasisRecordReader and OasisScanner still check for errors that
// will affect parsing the rest of the file.  For example,
// OasisRecordReader throws runtime_error if a LAYERNAME record has an
// invalid interval type, even if wantLayerName is false.


struct OasisParserOptions {
    bool  strictConformance;    // false => allow minor deviations from spec
    bool  wantValidation;       // false => ignore validation in END
    bool  wantText;             // false => ignore TEXT and TEXTSTRING
    bool  wantLayerName;        // false => ignore LAYERNAME
    bool  wantExtensions;       // false => ignore XNAME, XELEMENT, XGEOMETRY

public:
    OasisParserOptions() {
        strictConformance = true;
        wantValidation = true;
        wantText = true;
        wantLayerName = true;
        wantExtensions = true;
    }

    void
    resetAll() {
        strictConformance = false;
        wantValidation = false;
        wantText = false;
        wantLayerName = false;
        wantExtensions = false;
    }
};



// The OASIS parser class is large, with over 20 data members and over
// 80 methods.  Because almost all those methods are private, we use the
// Handle/Body (or Pimpl) idiom to hide that mess from the users of this
// library.  The body class which has all the code is ParserImpl in
// parser.cc.  The handle class is OasisParser, declared here.
// OasisParser keeps a pointer to a ParserImpl object and forwards all
// requests to it.
//
// See the OasisBuilder documentation in builder.h for how to use
// parseFile() and parseCell().  validateFile() checks the CRC/checksum
// in the file, if any, and throws a runtime_error if it's wrong.

class ParserImpl;

class OasisParser {
    ParserImpl* impl;   // all requests are forwarded to this body instance

public:
                OasisParser (const char* fname, WarningHandler warner,
                             const OasisParserOptions& options);
                ~OasisParser();
    void        validateFile();
    Validation  parseValidation();
    void        parseFile (OasisBuilder* builder);
    bool        parseCell (const char* cellname, OasisBuilder* builder);

private:
                OasisParser (const OasisParser&);       // forbidden
    void        operator= (const OasisParser&);         // forbidden
};


}  // namespace Oasis

#endif  // OASIS_PARSER_H_INCLUDED
