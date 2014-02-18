// oasis/asc-writer.h -- low-level interface for writing OASIS files in ASCII
//
// last modified:   02-Jan-2010  Sat  17:10
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef OASIS_ASC_WRITER_H_INCLUDED
#define OASIS_ASC_WRITER_H_INCLUDED

#include <cstdio>
#include <string>
#include <inttypes.h>           // for uint32_t

#include "port/compiler.h"
#include "keywords.h"
#include "oasis.h"


namespace Oasis {

using std::FILE;
using std::string;
using SoftJin::llong;
using SoftJin::Uint;
using SoftJin::Ullong;
using SoftJin::Ulong;


// AsciiWriter -- low-level interface for writing OASIS files in ASCII.
// This is used by AsciiRecordWriter.
//
// filename             string
//
//      Pathname of output file; used only for error messages
//
// fpout                FILE*
//
//      Opened for writing on output file.
//
// foldLongLines        bool
//
//      If true, the output routines will insert newlines to try to keep
//      lines from becoming too long.  If false, each record is printed
//      on a single line regardless of how long it becomes.  That is
//      more convenient when processing the output with a script.
//
// currColumn           Uint
//
//      Number of column where the next char will be written.  The first
//      column is 1.  This is used for a half-hearted attempt to keep
//      output lines from getting too wide.  Updated by all the
//      low-level methods that write to fpout.
//
// startColumn          Uint
//
//      The column on which the record started, i.e., where the record
//      name was printed.  Set by beginRecord().  This will be 1 if
//      record positions are not being printed, and between 10 and 20
//      (depending on the fileOffset and whether the position is in a
//      cblock) if they are.  Continuation lines of a record are
//      indented a fixed amount relative to startColumn.


class AsciiWriter {
    string      filename;
    FILE*       fpout;
    bool        foldLongLines;
    Uint        currColumn;
    Uint        startColumn;

public:
                AsciiWriter (const char* fname, bool foldLongLines);
                ~AsciiWriter();

    void        beginFile();
    void        endFile();

    void        beginRecord (Uint recID, const OFilePos& pos,
                             bool showPos, bool skipLine);
    void        endRecord();

    void        writeKeyword (AsciiKeyword keyword);
    void        writeInfoByte (int infoByte);
    void        writeUnsignedInteger (Ulong val);
    void        writeSignedInteger (long val);
    void        writeUnsignedInteger64 (Ullong val);
    void        writeSignedInteger64 (llong val);
    void        writeReal (const Oreal& val);
    void        writeString (const string& str);

    void        writeOneDelta (long offset);
    void        writeTwoDelta (Delta delta);
    void        writeThreeDelta (Delta delta);
    void        writeGDelta (Delta delta);

    void        writeImplicitRefnum (Ulong refnum);
    void        writeValidationScheme (Ulong valScheme);

private:
    void        print (const char* fmt, ...) SJ_PRINTF_ARGS(2,3);
    void        putString (const char* str, size_t len);
    void        putChar (char ch);
    void        newline();
    void        startToken (Uint nspaces);

    void        writeThreeDelta (Delta::Direction dirn, Ulong offset);
    void        abortWriter();

private:
                AsciiWriter (const AsciiWriter&);       // forbidden
    void        operator= (const AsciiWriter&);         // forbidden
};


} // namespace Oasis

#endif  // OASIS_ASC_WRITER_H_INCLUDED
