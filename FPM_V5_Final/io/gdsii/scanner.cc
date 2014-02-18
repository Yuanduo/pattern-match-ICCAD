// gdsii/scanner.cc -- low-level interface for reading GDSII Stream files
//
// last modified:   30-Dec-2009  Wed  08:16
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cerrno>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <boost/static_assert.hpp>

#include "misc/utils.h"
#include "glimits.h"
#include "scanner.h"


namespace Gdsii {

using namespace std;
using namespace SoftJin;


// constructor
//   fname      pathname of input file
//   ftype      type of file: normal or compressed

GdsScanner::GdsScanner (const char* fname, FileHandle::FileType ftype)
  : rbuf(BufferSize),
    fh(fname, ftype),
    filename(fname)
{
    fh.open(FileHandle::ReadAccess);
    fileOffset = 0;
}



GdsScanner::~GdsScanner()
{
    fh.close();
}



// getNextRecord -- read the next record from the file.
// rec->body will be valid only until the next call to this function().
// Throws runtime_error
//   - if a record has an invalid header
//   - if EOF is encountered
//   - if read() fails
// Precondition:
//   The ENDLIB record has not been read.

void
GdsScanner::getNextRecord (/*out*/ GdsRecord* rec)
{
    const Uint  HeaderLength = GLimits::RecordHeaderLength;
    Uint        recLength;      // record length from file (includes header)

    // Each GDSII record begins with a 4-byte header which includes a
    // 2-byte length field.  Ensure we have at least the header of the
    // next record in the buffer.

    if (rbuf.availData() < HeaderLength) {
        fillBuffer();
        if (rbuf.availData() < HeaderLength)
            abortScanner("unexpected EOF");
    }

    // Get the length of the record (including the header) and check it.
    // GdsRecord::initialize() ensures that the length is even.

    recLength = (rbuf.curr[0] << 8) + rbuf.curr[1];
    if (recLength < HeaderLength) {
        abortScanner("invalid record length %u; must be at least %u",
                     recLength, HeaderLength);
    }

    // Now that we know the record's length, ensure that the complete
    // record is in the buffer.

    if (rbuf.availData() < recLength) {
        fillBuffer();
        if (rbuf.availData() < recLength)
            abortScanner("unexpected EOF");
    }

    // Initialize the record's fields.  Ignore the data-type field
    // because the record type determines that.  Note that for speed
    // initRecord() does not copy the record body; it just stores a
    // pointer into our buffer.  That is why we ensure above that the
    // complete record is in the buffer.  That is also why we do our own
    // buffering instead of using stdio.

    Uint  recType = rbuf.curr[2];
    Uint  bodyLength = recLength - HeaderLength;
    Uchar*  body = rbuf.curr + HeaderLength;
    initRecord(rec, recType, bodyLength, body, currByteOffset());

    rbuf.curr += recLength;
}



// seekTo -- restart scanning at specified file offset.
// The argument must be the offset at which a record begins.
// Note that seeking to the middle of a compressed file is slow.

void
GdsScanner::seekTo (off_t offset)
{
    // If the offset specified is already in the buffer, just adjust
    // rbuf.curr.  Otherwise seek so that we begin reading at offset and
    // discard the buffer contents.
    //
    // Note that the second half of the test uses <=, not <.  GdsParser
    // calls seekTo(0) at the start of the parse, when the buffer is
    // empty (i.e., rbuf.end == rbuf.buffer).  We want that call to have no
    // effect so that it will work even if the input is a pipe.

    if (offset >= fileOffset
            &&  offset <= fileOffset + (rbuf.end - rbuf.buffer))
        rbuf.curr = rbuf.buffer + (offset - fileOffset);
    else {
        if (fh.seek(offset, SEEK_SET) != offset)
            abortScanner("file seek failed: %s", strerror(errno));
        rbuf.refill(0);
        fileOffset = offset;
        // Don't worry about keeping future reads block-aligned.
        // It doesn't seem to slow down the reads.
    }
}


//----------------------------------------------------------------------
// Private methods


// fillBuffer -- fill the buffer with data from the file.
// Throws an exception if a read error occurs.
// Precondition:
//     The buffer contains less than MaxRecordLength bytes.

void
GdsScanner::fillBuffer()
{
    BOOST_STATIC_ASSERT (BufferSize >= 1*GLimits::MaxRecordLength);
                                // The 1* prevents a warning from gcc.
    assert (rbuf.availData() < GLimits::MaxRecordLength);

    // Move the remaining bytes in the buffer to the left end.

    size_t  nbytes = rbuf.availData();          // we still have this much
    memmove(rbuf.buffer, rbuf.curr, nbytes);
    fileOffset += rbuf.curr - rbuf.buffer;      // new starting offset

    // Fill the remainder of the buffer.  On a Pentium4 running Linux
    // 2.4.20 there seems to be no advantage in making the read size a
    // multiple of the page size or in aligning the read buffer.

    ssize_t  nr = fh.read(rbuf.buffer + nbytes, BufferSize - nbytes);
    if (nr < 0)
        abortScanner("read failed: %s", strerror(errno));
    rbuf.refill(nbytes + nr);
}



// initRecord -- initialize contents of GdsRecord
//   rec        out: pointer to record to initialize
//   recType    record type from file (byte 3 in record)
//   length     length of record body (length from file minus header length)
//   body       pointer to record body.  Not NUL-terminated.  The space
//              is owned by GdsScanner and may be overwritten on the
//              next call to getNextRecord().
//   fileOffset  offset in file where record begins
//
// initRecord() checks the values to be stored into the record and
// aborts if any of them is invalid.

void
GdsScanner::initRecord (/*out*/ GdsRecord* rec, Uint recType,
                        Uint length, Uchar* body, off_t fileOffset)
{
    // Don't accept any record type that the GDSII spec says is unused.
    if (! GdsRecordTypeInfo::recTypeIsValid(recType))
        abortScanner("invalid record type %u", recType);

    const GdsRecordTypeInfo*  rti = GdsRecordTypeInfo::get(recType);

    // Verify that the length is within bounds.

    if (length < rti->minLength)
        abortScanner("%s record body has invalid length %u; "
                     "must be at least %u",
                     rti->name, length, rti->minLength);
    if (length > rti->maxLength)
        abortScanner("%s record body has invalid length %u; may be at most %u",
                     rti->name, length, rti->maxLength);

    // Verify that the length is a multiple of the data type's size (for
    // fixed-size data types), or of 2 (for strings).  Some string
    // records contain fixed-size 44-byte strings; rti->elemSize is set
    // appropriately for these.  The XY and LIBSECUR records are
    // exceptions: XY because it must contain pairs of 4-byte integer
    // coordinates and LIBSECUR because it must contain triples (group
    // number, user number, access rights) of 2-byte integers.

    Uint  sizeUnit = rti->itemSize;
    if (sizeUnit == 0)
        sizeUnit = 2;
    else if (recType == GRT_XY)
        sizeUnit = 8;
    else if (recType == GRT_LIBSECUR)
        sizeUnit = 6;

    if ((length % sizeUnit) != 0)
        abortScanner("%s record body has invalid length %u: "
                     "must be a multiple of %u",
                     rti->name, length, sizeUnit);

    rec->typeInfo   = rti;
    rec->length     = length;
    rec->body       = body;
    rec->bp         = body;
    rec->fileOffset = fileOffset;
}



// abortScanner -- throw runtime_error for unrecoverable error
//   fmt        printf() format string for error message
//   ...        args, if any, for the format string
// Use exception::what() to retrieve the formatted error message from
// the exception.

void
GdsScanner::abortScanner (const char* fmt, ...)
{
    char  msgbuf[256];
    llong  offset = currByteOffset();
    Uint  n = SNprintf(msgbuf, sizeof msgbuf, "file '%s', offset %lld: ",
                       filename.c_str(), offset);
    va_list  ap;
    va_start(ap, fmt);
    if (n < sizeof(msgbuf) - 1)
        VSNprintf(msgbuf+n, sizeof(msgbuf)-n, fmt, ap);
    va_end(ap);

    ThrowRuntimeError("%s", msgbuf);
}


} // namespace Gdsii
