// misc/utils.cc
//
// last modified:   03-Jan-2010  Sun  17:28
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <unistd.h>
#include <boost/static_assert.hpp>

#include "port/fcntl.h"
#include "utils.h"


namespace SoftJin {

using namespace std;


// VSNprintf -- wrapper for vsnprintf() that checks for errors.
// Returns whatever non-negative value vsnprintf() returns.  Sets buf to
// the empty string if vsnprintf() returns a negative number.  The
// vsnprintf() man page says that that happens if an output error occurs.
//
// We use this wrapper only to format error messages and similar strings
// where it does not really matter if string gets truncated.  That is
// why we simply clear the buffer if vsnprintf() fails.

Uint
VSNprintf (/*out*/ char* buf, size_t bufsize, const char* fmt, va_list ap)
{
    int  n = vsnprintf(buf, bufsize, fmt, ap);
    if (n < 0) {
        if (bufsize > 0)
            buf[0] = Cnul;
        n = 0;
    }
    return n;
}



// SNprintf -- wrapper for snprintf()

Uint
SNprintf (/*out*/ char* buf, size_t bufsize, const char* fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    Uint  n = VSNprintf(buf, bufsize, fmt, ap);
    va_end(ap);

    return n;
}



//----------------------------------------------------------------------
// Error handling


namespace {
    const char* progname;    // basename of argv[0]
}


void
SetProgramName (const char* pname)
{
    if (const char* cp = strrchr(pname, '/'))
        pname = cp+1;
    progname = pname;
}


const char*
GetProgramName()
{
    return progname;
}



// Error -- write error message to stderr.
// Args are as for printf().
// The trailing newline should be omitted from the format string.

void
Error (const char* fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    if (progname != Null)
        fprintf(stderr, "%s: ", progname);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}



// FatalError -- like Error(), but die after printing the error message.

void
FatalError (const char* fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    if (progname != Null)
        fprintf(stderr, "%s: ", progname);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}



// ThrowRuntimeError -- throw runtime_error for unrecoverable error
//   fmt        printf() format string for error message
//   ...        args, if any, for the format string
// Use exception::what() to retrieve the formatted error message from
// the exception.

void
ThrowRuntimeError (const char* fmt, ...)
{
    char        msgbuf[256];

    va_list  ap;
    va_start(ap, fmt);
    VSNprintf(msgbuf, sizeof msgbuf, fmt, ap);
    va_end(ap);

    throw runtime_error(msgbuf);
}


//----------------------------------------------------------------------
// Characters and Strings

// CharToString -- make a printable representation of a char
//   buf        buffer for printable representation
//   bufsize    bytes available in buf; must be >= 5.
//   ch         the character we want to print
// Returns buf.
//
// If the char is printable, the representation is the char itself.
// Otherwise it is a string of the form \ooo, where ooo is the
// character's octal code.

char*
CharToString (/*out*/ char* buf, size_t bufsize, Uchar uch)
{
    const char*  fmt = isprint(uch) ? "%c" : "\\%03o";
    SNprintf(buf, bufsize, fmt, uch);
    return buf;
}



// NewString -- make copy of NUL-terminated string.
// Throws bad_alloc if the new fails.

char*
NewString (const char* str)
{
    size_t  len = strlen(str) + 1;
    char*  newstr = new char[len];
    memcpy(newstr, str, len);
    return newstr;
}



// NewString -- make copy of string that is not necessarily NUL-terminated.
// The copy is NUL-terminated.  Throws bad_alloc if the new fails.

char*
NewString (const char* str, size_t len)
{
    char*  newstr = new char[len+1];
    memcpy(newstr, str, len);
    newstr[len] = Cnul;
    return newstr;
}



size_t
HashString (const string& s)
{
    size_t  hash = 0;
    const char*  cp = s.data();
    const char*  end = cp + s.size();
    for ( ;  cp != end;  ++cp)
        hash = 19*hash + static_cast<unsigned char>(*cp);
    return hash;
}



// MakePrintableString -- make printable version of string for error message
//   str, nchars        input string and its size
//   buf, bufSize       output buffer and its size
//
// GetPrintableString() copies str to buf, replacing unprintable
// characters by their escaped octal code.  It writes at most bufSize
// chars to buf, including the NUL.  If that is not enough space for the
// entire input string, it replaces the last few characters by "...".
//
// GetPrintableString() is intended to generate a printable string for
// an error message.  That is why it limits the size of the output
// string.

char*
MakePrintableString (const char* str, size_t nchars,
                     /*out*/ char* buf, size_t bufSize)
{
    assert (bufSize > 0);

    const char*  strEnd = str + nchars;
    char*  ocp = buf;
    char*  lastEsc = buf;               // position of last \ooo
    char*  bufEnd = buf + bufSize - 1;  // -1 for Cnul

    for (  ;  str != strEnd;  ++str) {
        Uchar  uch = static_cast<Uchar>(*str);
        if (isprint(uch)) {
            if (ocp == bufEnd)
                break;
            *ocp++ = uch;
        } else {
            if (bufEnd - ocp < 4)       // need 4 chars for \ooo
                break;
            sprintf(ocp, "\\%03o", uch);
            lastEsc = ocp;
            ocp += 4;
        }
    }

    // If the string was truncated, replace the last 3 chars by "...",
    // but avoid overwriting part of an escape sequence.  If one is
    // too close to the end, put the ... at the \.

    if (str == strEnd || bufSize < 4)   // need 4 chars for ... and NUL
        *ocp = Cnul;
    else {
        char*  pos = (bufEnd - lastEsc < 7 ? lastEsc : bufEnd-3);
        strcpy(pos, "...");
    }
    return buf;
}


//----------------------------------------------------------------------


// DoubleIsIntegral -- test if a double is integral and fits in a long
//   val        value to test
//   plval      out: if IsIntegral() returns true, the equivalent
//              long value is stored here

bool
DoubleIsIntegral (double val, /*out*/ long* plval)
{
    // val is a whole number if converting it to a long and back to a
    // double gives back val.  Because conversion from double to long is
    // defined only if the double's value is in long's range, check that
    // first.

    if (val < LONG_MIN || val > LONG_MAX)
        return false;

    long  lval = static_cast<long>(val);
    if (static_cast<double>(lval) != val)
        return false;

    *plval = lval;
    return true;
}



// DoubleIsFloat -- test if a double can safely be stored in a float
//   val        value to test
//   pfval      out: if IsFloat() returns true, the equivalent
//              float value is stored here

bool
DoubleIsFloat (double val, /*out*/ float* pfval)
{
    // Two things to verify:
    //   - the double's magnitude is small enough to fit in a float
    //   - no precision is lost by assigning to a float, i.e., the
    //     least-significant 32 bits of the significand are zero.

    if (fabs(val) > FLT_MAX)
        return false;

    float  fval = val;
    if (static_cast<double>(fval) != val)
        return false;

    *pfval = fval;
    return true;
}


//----------------------------------------------------------------------


// WriteNBytes -- wrapper for write() that retries after partial writes.
//   fd         file descriptor opened for writing.
//              Must not have been opened with O_NONBLOCK.
//   buf        buffer containing data to write
//   count      how many bytes to write
// Returns count on success; sets errno and returns -1 on failure.
//
// The system call write() may return after writing any number of bytes up
// to the requested amount.  It is a nuisance to keep handling partial
// writes, so we use this wrapper instead.

ssize_t
WriteNBytes (int fd, const void* buf, size_t count)
{
    size_t  numLeft = count;
    while (numLeft > 0) {
        ssize_t  nw = write(fd, buf, numLeft);
        if (nw < 0) {
            if (errno == EINTR)
                continue;
            return nw;
        }
        buf = static_cast<const char*>(buf) + nw;
        numLeft -= nw;
    }
    return count;
}



// ReadNBytes -- wrapper for read() that retries after partial reads.
//   fd         file descriptor opened for reading.
//              Must not have been opened with O_NONBLOCK.
//   buf        buffer where data is to be stored
//   count      how many bytes to read
// Returns the number of bytes read.  This might be less than count
// if it reached EOF.  Sets errno and returns -1 on failure.
// This is like WriteNBytes() above.

ssize_t
ReadNBytes (int fd, /*out*/ void* buf, size_t count)
{
    size_t  numLeft = count;
    while (numLeft > 0) {
        ssize_t  nr = read(fd, buf, numLeft);
        if (nr < 0) {
            if (errno == EINTR)
                continue;
            return nr;
        }
        if (nr == 0)    // EOF
            return (count - numLeft);
        buf = static_cast<char*>(buf) + nr;
        numLeft -= nr;
    }
    return count;
}



// ThrowOnOpenFailure -- format error message and throw runtime_error.
// This is a helper function for OpenFile() and FopenFile() below.
//   pathname   path of file for which the open/fopen failed
//   create     true => the application was trying to create a new file.
//              false => the application was trying to open an existing file.
//              The error message is slightly different for the two cases.
//
// Uses errno in formatting the error message.  Since failing to open
// a file is typically caused by a user mistake, this function goes to
// some trouble to make the message clear.

static void
ThrowOnOpenFailure (const char* pathname, bool create)
{
    const char*  op = (create ? "create" : "open");
        // When trying to create a file, it is clearer to say
        // "cannot create FOO" than "cannot open FOO"

    const char*  errString = (create  &&  errno == ENOENT)
                                 ? "Parent directory does not exist"
                                 : strerror(errno);
        // When file creation fails with ENOENT, saying
        // "no such file or directory" is confusing.  The message apparently
        // refers to the file being created, which obviously need not exist.

    ThrowRuntimeError("cannot %s %s: %s", op, pathname, errString);
}



// OpenFile -- wrapper for open(2) that throws a runtime_error on failure.
// Opens the file in binary mode.

int
OpenFile (const char* pathname, int flags, mode_t mode)
{
    int  fd = open(pathname, flags|BinaryOpenFlag, mode);
    if (fd < 0)
        ThrowOnOpenFailure(pathname, flags & O_CREAT);
    return fd;
}



// FopenFile -- wrapper for fopen(3S) that throws a runtime_error on failure.

FILE*
FopenFile (const char* pathname, const char* mode)
{
    FILE*  fp = fopen(pathname, mode);
    if (fp == Null)
        ThrowOnOpenFailure(pathname, *mode == 'w');
    return fp;
}


} // namespace SoftJin
