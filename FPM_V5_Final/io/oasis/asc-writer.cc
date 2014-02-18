// oasis/asc-writer.cc -- low-level interface for writing OASIS files in ASCII
//
// last modified:   02-Jan-2010  Sat  17:10
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <sys/types.h>          // for off_t
#include <boost/static_assert.hpp>

#include "misc/utils.h"
#include "asc-writer.h"
#include "rectypes.h"


namespace Oasis {

using namespace std;
using namespace SoftJin;


const Uint      RightMargin = 75;
    // No token will begin after this column.  Each token-writing
    // function begins a continuation line if the current column is
    // greater than this.  Note that tokens may extend beyond this
    // margin, e.g., a string literal could extend to column 1000.
    // Making sure that nothing extends past column 80 is not
    // worth the trouble.


const Uint      ContinuationIndent = 8;
    // How much to indent the second and succeeding lines of each
    // record.  Don't make it too large.  The continuation lines will
    // begin on column (startColumn + ContinuationIndent)


BOOST_STATIC_ASSERT (RightMargin >= 60);
BOOST_STATIC_ASSERT (ContinuationIndent <= 20);
    // Keep things reasonable.



// AsciiWriter constructor
//   fname          pathname of file to write
//   foldLongLines  whether long output lines should be folded.
//                  false => print each record on a single line regardless
//                  of how long it gets

AsciiWriter::AsciiWriter (const char* fname, bool foldLongLines)
  : filename(fname)
{
    this->foldLongLines = foldLongLines;
    fpout = Null;
    currColumn = 1;
    startColumn = 1;
}



AsciiWriter::~AsciiWriter()
{
    // If the file is still open, either endFile() was never called or
    // an exception was thrown.  Either way we don't care if fclose
    // fails.

    if (fpout != Null)
        fclose(fpout);
}



// beginFile -- open the file
// This must be the first method called.

void
AsciiWriter::beginFile()
{
    fpout = FopenFile(filename.c_str(), "w");
}



// endFile -- close the file
// This must be the last method called.

void
AsciiWriter::endFile()
{
    if (fclose(fpout) == EOF)
        abortWriter();
    fpout = Null;
}



//----------------------------------------------------------------------
// Private methods that write to fpout.  All the token-writing functions
// go through these.  print(), putString(), and putChar() assume that
// none of the chars printed is a control char like tab or LF.  The only
// control char that should be printed at all is LF, and functions that
// do that should reset currColumn to 1.


// print -- general print method used for most tokens

void
AsciiWriter::print (const char* fmt, ...)
{
    va_list  ap;

    va_start(ap, fmt);
    int  n = vfprintf(fpout, fmt, ap);
    va_end(ap);

    if (n < 0)
        abortWriter();
    currColumn += n;
}



// putString -- print len characters beginning at str

void
AsciiWriter::putString (const char* str, size_t len)
{
    if (fwrite(str, 1, len, fpout) != len)
        abortWriter();
    currColumn += len;
}



inline void
AsciiWriter::putChar (char ch)
{
    if (putc(ch, fpout) == EOF)
        abortWriter();
    ++currColumn;
}



inline void
AsciiWriter::newline()
{
    putChar('\n');
    currColumn = 1;
}



// startToken -- prepare for printing a new token
//   nspaces    how many spaces to leave before the token
// startToken() begins a continuation line if adding the required spaces
// would take it past the right margin.

void
AsciiWriter::startToken (Uint nspaces)
{
    // Define a constant string with enough spaces to handle the largest
    // possible indentation.  We need about 40 spaces; we allocate 100
    // to be absolutely safe.

    static const char  spacer[] =
        "                                                  "
        "                                                  ";

    // If we need to start a new line, write enough spaces to indent by
    // the required amount instead of the number requested by the caller.

    if (foldLongLines  &&  currColumn + nspaces > RightMargin) {
        newline();
        nspaces = startColumn + ContinuationIndent - 1;
    }
    assert (nspaces < sizeof spacer);
    putString(spacer, nspaces);
}



//----------------------------------------------------------------------
//                              Writing tokens
//----------------------------------------------------------------------

// beginRecord -- start printing a new record
//   recID      record-ID (RID_*) or pseudo-record-ID (RIDX_*).
//              The ascName corresponding to recID will be printed.
//   pos        position of record in OASIS file.  Must be defined only
//              if the next argument is true
//   showPos    if true, pos will be printed in square brackets before the
//              record name.  If false, pos will be ignored.
//   skipLine   whether a blank line should be left before the record.

void
AsciiWriter::beginRecord (Uint recID, const OFilePos& pos,
                          bool showPos, bool skipLine)
{
    if (skipLine)
        newline();

    // For positions inside cblocks, print both the cblock's fileOffset
    // and the offset in the expanded cblock.

    llong  lloff = pos.fileOffset;
    if (showPos) {
        if (pos.inCblock())
            print("[%lld:%06lld]  ", lloff, pos.cblockOffset);
        else
            print("[%07lld]  ", lloff);
    }
    startColumn = currColumn;
    print("%s", GetRecordAscName(recID));
}



void
AsciiWriter::endRecord() {
    newline();
}



void
AsciiWriter::writeKeyword (AsciiKeyword keyword)
{
    // Keywords are used mainly for keyword/value pairs in element
    // records.  To make these records easier to read, we want more
    // space between pairs than between a keyword and its value.  For
    // example, the first line below is easier to read than the second:
    //
    // RECTANGLE  width 10  height 20  x 40  y 50
    //
    // RECTANGLE width 10 height 20 x 40 y 50
    //
    // So we leave two spaces before a keyword instead of one.

    startToken(2);
    print("%s", GetKeywordName(keyword));
}



void
AsciiWriter::writeInfoByte (int infoByte)
{
    startToken(1);

    // Handle 0 as a special case because glibc omits the 0x prefix when
    // printing 0 with format %#.2x (it prints "00").  It is better to
    // give all the info-bytes a uniform look.

    if (infoByte == 0)
        putString("0x00", 4);
    else
        print("%#.2x", infoByte);
}



void
AsciiWriter::writeUnsignedInteger (Ulong val) {
    startToken(1);
    print("%lu", val);
}


void
AsciiWriter::writeSignedInteger (long val) {
    startToken(1);
    print("%ld", val);
}



void
AsciiWriter::writeUnsignedInteger64 (Ullong val) {
    startToken(1);
    print("%llu", val);
}


void
AsciiWriter::writeSignedInteger64 (llong val) {
    startToken(1);
    print("%lld", val);
}



void
AsciiWriter::writeReal (const Oreal& val)
{
    startToken(1);

    // If val is integral, print it like a signed-integer.  Otherwise if
    // it is rational, print it in the form numer/denom.

    if (val.isRational()) {
        long  numer = val.getNumerator();
        long  denom = val.getDenominator();
        if (denom == 1)
            print("%ld", numer);
        else
            print("%ld/%ld", numer, denom);
        return;
    }

    // Not a rational.  Print as a floating-point number, with either a
    // decimal point or an exponent, so that we can make out what kind
    // of real the OASIS file contained.  If it is a single-precision
    // float, add the suffix f as in C/C++.
    //
    // Unfortunately none of the printf() conversion specifications
    // directly gives us what we want.  %.*g removes trailing zeros,
    // but it also removes the decimal point if only zeros follow it.
    // So 5.0 will be printed as "5".  %#.*g leaves the decimal point,
    // but it also leaves all the trailing zeros.
    //
    // Our solution is to use %.*g to print into a temporary buffer, and
    // append ".0" to the result if it does not contain 'e' or a '.'.
    // Then we append f if the real is a Float32.  Float32 numbers have
    // only about 7 digits of precision; using %.15g for floats results
    // in garbage for the later digits.

    char  buf[50];
    int   precision = (val.isFloatSingle() ? 7 : 15);
    Uint  n = SNprintf(buf, sizeof buf, "%.*g", precision, val.getValue());
    assert (n < sizeof(buf) - 5);

    if (strpbrk(buf, ".e") == Null) {
        buf[n++] = '.';
        buf[n++] = '0';
    }
    if (val.isFloatSingle())
        buf[n++] = 'f';

    // buf may no longer be NUL-terminated, but we don't care.
    putString(buf, n);
}



// writeString -- write a string literal

void
AsciiWriter::writeString (const string& str)
{
    const char*  segStart = str.data();         // start of current segment
    const char*  end = str.data() + str.size(); // one past the end of str
    const char*  cp;

    startToken(1);

    // Most strings will contain only printable characters.  So instead
    // of printing char by char, we treat the string as segments of
    // printable chars separated by chars that must be escaped.

    putChar('"');
    for (cp = str.data();  cp != end;  ++cp) {
        Uchar  uch = static_cast<Uchar>(*cp);

        // Ordinary character
        if (isprint(uch)  &&  uch != '"'  &&  uch != '\\')
            continue;

        // Current char must be escaped.  Print the previous segment of
        // ordinary chars and start the next segment after this char.

        if (cp != segStart)
            putString(segStart, cp - segStart);
        segStart = cp+1;

        // Escape " and \, and print a non-printing (control) char as \ooo
        // where ooo is the octal code for the char.

        const char*  fmt = (isprint(uch) ? "\\%c" : "\\%03o");
        print(fmt, uch);
    }
    if (cp != segStart)
        putString(segStart, cp - segStart);
    putChar('"');
}



// writeOneDelta -- write a 1-delta (a signed offset with implied horiz/vert)

void
AsciiWriter::writeOneDelta (long offset) {
    writeSignedInteger(offset);
}



// writeThreeDelta -- write a 3-delta (an octangular displacement)
// This also handles 2-deltas because in the ASCII file 2-deltas
// and 3-deltas have the same representation.

/*private*/ void
AsciiWriter::writeThreeDelta (Delta::Direction dirn, Ulong offset)
{
    // OASIS specifies the direction values, which range from 0 to 7.
    // The strings here must agree with those in the scanner rules.

    static const char *const  compass[] = {
        "e", "n", "w", "s", "ne", "nw", "sw", "se"
    };

    startToken(1);
    print("%s:%lu", compass[dirn], offset);
}



void
AsciiWriter::writeTwoDelta (Delta delta)
{
    assert (delta.isManhattan());

    Ulong  offset = abs(delta.x + delta.y);    // either .x or .y is 0
    writeThreeDelta(delta.getDirection(), offset);
}



void
AsciiWriter::writeThreeDelta (Delta delta)
{
    assert (delta.isOctangular());

    Ulong  offset = (delta.x != 0 ? abs(delta.x) : abs(delta.y));
    writeThreeDelta(delta.getDirection(), offset);
}



// writeGDelta -- write a g-delta (a general delta)

void
AsciiWriter::writeGDelta (Delta delta)
{
    startToken(1);
    print("(%ld,%ld)", delta.x, delta.y);
}



// writeImplicitRefnum -- write an implicitly-assigned reference number
//
// This is for the name records like CELLNAME.  The implicit reference
// number is the number that would be assigned to the name record if the
// file were read.  We write the refnum in square brackets so that the
// scanner will ignore it.

void
AsciiWriter::writeImplicitRefnum (Ulong refnum)
{
    startToken(1);
    print("[%lu]", refnum);
}



void
AsciiWriter::writeValidationScheme (Ulong valScheme)
{
    // Write the keyword corresponding to the scheme.  But if the scheme
    // is invalid, just write it as is.  There is no point in calling
    // abortWriter() for that.  AsciiScanner can handle both keyword and
    // unsigned-integer here.

    switch (valScheme) {
        case Validation::None:       writeKeyword(KeyNone);         break;
        case Validation::CRC32:      writeKeyword(KeyCRC32);        break;
        case Validation::Checksum32: writeKeyword(KeyChecksum32);   break;

        default:
            writeUnsignedInteger(valScheme);
            break;
    }
}


//----------------------------------------------------------------------


// abortWriter -- throw runtime_error for write error

void
AsciiWriter::abortWriter()
{
    ThrowRuntimeError("file '%s': write failed: %s",
                      filename.c_str(), strerror(errno));
}


}  // namespace Oasis
