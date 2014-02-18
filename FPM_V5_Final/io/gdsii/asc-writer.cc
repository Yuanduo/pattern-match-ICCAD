// gdsii/asc-writer.cc -- writer for ASCII version of GDSII files
//
// last modified:   30-Dec-2009  Wed  20:00
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.


#include <cctype>
#include <cerrno>
#include <cstring>
#include <boost/static_assert.hpp>

#include "misc/utils.h"
#include "asc-writer.h"


namespace Gdsii {

using namespace std;
using namespace SoftJin;


AsciiWriter::AsciiWriter (const char* fname, bool showOffsets)
  : filename(fname),
    namePool(600)               // we need around 500-550 bytes for the names
{
    this->showOffsets = showOffsets;
    fpout = Null;
    currIndent = 0;
    currOffset = 0;
    currType = Null;

    // Make lowercase versions of the record names.  Skip the names for
    // the invalid record types

    for (Uint j = 0;  j <= GRT_MaxType;  ++j) {
        recordNames[j] = namePool.newString(GdsRecordTypeInfo::getName(j));
        for (char* cp = recordNames[j];  *cp != Cnul;  ++cp)
            *cp = tolower(*cp);
    }
}



AsciiWriter::~AsciiWriter()
{
    // If the file is still open, either the writer was used improperly
    // or an exception was thrown.  Either way it doesn't matter if
    // fclose fails.

    if (fpout != Null)
        fclose(fpout);
}



// beginFile -- begin writing the ASCII file.

void
AsciiWriter::beginFile()
{
    fpout = FopenFile(filename.c_str(), "w");
}



// endFile -- finish writing the ASCII file.

void
AsciiWriter::endFile()
{
    // Close the file here rather than in the destructor so that we can
    // throw an exception if close fails.  Destructors must not throw.

    if (fclose(fpout) == EOF)
        abortWriter();
    fpout = Null;
}



/*private*/ inline const char*
AsciiWriter::getRecordName (const GdsRecordTypeInfo* rti) {
    return recordNames[rti->recType];
}



/*private*/ inline void
AsciiWriter::writeBytes (const void* data, size_t numBytes) {
    if (fwrite(data, numBytes, 1, fpout) != 1)
        abortWriter();
}



// writeStringLiteral -- print string surrounded by " "
//   str        the string literal.  Need not be NUL-terminated.
//   maxlen     max length of string.  The string terminates at the first
//              NUL char or after maxlen chars, whichever comes earlier.
// printStringLiteral() prints string foo as "foo" and escapes " and
// non-printing characters in foo.  Strings in GDSII Stream format can
// contain unprintable characters, but not NULs since they are NUL-padded.
// However, they need not be NUL-terminated; that is why we need maxlen.

void
AsciiWriter::writeStringLiteral (const char* str, Uint maxlen)
{
    const char*  segStart;      // start of current segment of str
    const char*  end;           // one past the end of str
    const char*  cp;

    segStart = str;
    end = static_cast<char*> (memchr(str, Cnul, maxlen));
    if (end == Null)
        end = str + maxlen;

    // Most strings will contain only printable characters.  So instead
    // of printing char by char, we treat the string as segments of
    // printable chars separated by chars that must be escaped.

    writeChar('"');
    for (cp = str;  cp != end;  ++cp) {
        Uchar  uch = static_cast<Uchar>(*cp);

        // Ordinary character
        if (isprint(uch)  &&  uch != '"'  &&  uch != '\\')
            continue;

        // Current char must be escaped.  Print the previous segment of
        // ordinary chars and start the next segment after this char.

        if (cp != segStart)
            writeBytes(segStart, cp - segStart);
        segStart = cp+1;

        // Escape " and \, and print a non-printing (control) char as \ooo
        // where ooo is the octal code for the char.

        if (isprint(uch)) {
            writeChar('\\');
            writeChar(uch);
        } else {
            char  buf[5];
            SNprintf(buf, sizeof buf, "\\%03o", uch);
            writeString(buf);
        }
    }
    if (cp != segStart)
        writeBytes(segStart, cp - segStart);
    writeChar('"');
}



// beginRecord -- print indentation and record name
//   rti        type of record being started

void
AsciiWriter::beginRecord (const GdsRecordTypeInfo* rti)
{
    // If requested, print the offset of the record in square brackets.
    // NOTE: The extra whitespace in continueRecord() below depends on
    // the format.

    if (showOffsets) {
        char  buf[25];
        SNprintf(buf, sizeof buf, "[%08lld]  ", static_cast<llong>(currOffset));
        writeString(buf);
    }

    writeIndentation();
    writeString(getRecordName(rti));
    currType = rti;             // save this for endRecord()
}



// continueRecord -- add a continuation line to a record
//   padding    extra whitespace to make continuation lines line up
//              with the first line of the record

void
AsciiWriter::continueRecord (const char* padding)
{
    writeChar('\n');
    // The amount of whitespace here depends on OffsetFormat in beginRecord().
    if (showOffsets)
        writeString("            ");
    writeIndentation();
    writeString(padding);
}



// writeIndentation -- print spaces to indent line to current indent level.

/*private*/ void
AsciiWriter::writeIndentation()
{
    // Indent by at most two levels because that is all valid GDSII
    // files can have.

    switch (currIndent) {
        case 0:  break;
        case 1:  writeString("    ");        break;
        default: writeString("        ");    break;
    }
}



// endRecord -- end the current record

void
AsciiWriter::endRecord()
{
    assert (currType != Null);

    // Indent the records in a structure by one level and the records
    // within an element in a structure by two.  Note that we print an
    // ENDSTR record indented with respect to the BGNSTR; similarly for
    // ENDEL and the records that begin an element.  To follow C style
    // and print the end-record at the same indentation level as the
    // begin-record, decrement indent in beginRecord() before printing
    // the indentation.
    // XXX: Can replace the switch by an array indexed by recType.

    writeChar('\n');
    switch (currType->getRecordType()) {
        case GRT_BGNSTR:
        case GRT_AREF:
        case GRT_BOUNDARY:
        case GRT_BOX:
        case GRT_NODE:
        case GRT_PATH:
        case GRT_SREF:
        case GRT_TEXT:
            ++currIndent;
            break;

        case GRT_ENDSTR:
        case GRT_ENDEL:
            --currIndent;
            break;

        // keep gcc from warning about unused enumerators
        default:
            break;
    }

    currType = Null;
}



// abortWriter -- abort writer because a write failed

/*private*/ void
AsciiWriter::abortWriter()
{
    ThrowRuntimeError("file '%s': write failed: %s",
                      filename.c_str(), strerror(errno));
}


} // namespace Gdsii
