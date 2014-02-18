// gdsii/asc-writer.h -- writer for ASCII version of GDSII files
//
// last modified:   31-Aug-2004  Tue  11:00
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef GDSII_ASC_WRITER_H_INCLUDED
#define GDSII_ASC_WRITER_H_INCLUDED

#include <cstdio>
#include <string>
#include <sys/types.h>          // for off_t
#include "misc/stringpool.h"
#include "rectypes.h"


namespace Gdsii {

using SoftJin::Uint;
using SoftJin::StringPool;


// AsciiWriter -- low-level interface for writing ASCII version of GDSII.
// This is the ASCII equivalent of GdsWriter.
//
// currIndent   Uint
//
//      The current indentation level.  The writer indents each record
//      appropriately to make the file structure clearer.  It increments
//      the indentation level after BGNSTR and the records that begin
//      elements (BOUNDARY, TEXT, etc), and decrements the indentation
//      level after ENDEL and ENDSTR.
//
// currOffset   off_t
//
//      The offset in the GDSII file of the next record that begins.
//      This is for the GDSII->ASCII converter.  When converting a GDSII
//      file to ASCII for debugging, it helps to see the file offsets of
//      the records.  This variable is used only when showOffsets is true.
//
// showOffsets  bool
//
//      If true, beginRecord() will print the currOffset before the
//      record name.
//
// namePool     StringPool
// recordNames  array[GRT_MaxType+1] of char*
//
//      These are used for lowercase versions of the record names in
//      rectypes.cc, which are in uppercase.  Lowercase names are easier
//      to read.  The strings themselves are stored in namePool; the
//      array has pointers to the strings.
//
// currType     const GdsRecordTypeInfo*
//
//      Valid only between calls to beginRecord() and endRecord().
//      Gives the type of the record being written.


class AsciiWriter {
    std::FILE*  fpout;          // for output file
    std::string filename;       // pathname of output file
    Uint        currIndent;     // indentation level
    off_t       currOffset;     // offset of next record
    bool        showOffsets;    // true => print offset of each record
    StringPool  namePool;
    char*       recordNames[GRT_MaxType+1];
    const GdsRecordTypeInfo*  currType; // record type now being written

public:
                AsciiWriter (const char* fname, bool showOffsets);
                ~AsciiWriter();

    void        beginFile();
    void        beginRecord (const GdsRecordTypeInfo* rti);
    void        continueRecord (const char* padding);
    void        endRecord();
    void        endFile();

    void        setOffset (off_t offset);

    void        writeChar (char);
    void        writeString (const char*);
    void        writeInt (const char* fmt, int val);
    void        writeDouble (const char* fmt, double val);
    void        writeStringLiteral (const char* str, Uint maxlen);

private:
    const char* getRecordName (const GdsRecordTypeInfo* rti);
    void        writeBytes (const void* data, size_t numBytes);
    void        writeIndentation();
    void        abortWriter();

                AsciiWriter (const AsciiWriter&);       // forbidden
    void        operator= (const AsciiWriter&);         // forbidden
};



inline void
AsciiWriter::setOffset (off_t offset) {
    currOffset = offset;
}


inline void
AsciiWriter::writeChar (char ch) {
    if (std::putc(ch, fpout) == EOF)
        abortWriter();
}


inline void
AsciiWriter::writeString (const char* str) {
    if (std::fputs(str, fpout) == EOF)
        abortWriter();
}


inline void
AsciiWriter::writeInt (const char* fmt, int val) {
    if (std::fprintf(fpout, fmt, val) < 0)
        abortWriter();
}


inline void
AsciiWriter::writeDouble (const char* fmt, double val) {
    if (std::fprintf(fpout, fmt, val) < 0)
        abortWriter();
}



} // namespace Gdsii

#endif  // GDSII_ASC_WRITER_H_INCLUDED
