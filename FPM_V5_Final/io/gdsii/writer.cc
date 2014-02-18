// gdsii/writer.cc -- low-level interface for writing GDSII Stream files
//
// last modified:   30-Dec-2009  Wed  08:28
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.


#include <cerrno>
#include <fcntl.h>
#include <boost/static_assert.hpp>

#include "misc/utils.h"
#include "writer.h"


namespace Gdsii {

using namespace std;
using namespace SoftJin;


const Uint  WriteIncr = 8*1024;
    // Writes to the file will be a multiple of this size.  The ideal
    // value is the block size of the filesystem.


const Uint  BufferSize = WriteIncr + 4*MaxRecordLength;
    // Size of output buffer.  Must be >= WriteIncr+MaxRecordLength
    // to ensure that WriteSize in ensureSpaceForRecord() is positive.


BOOST_STATIC_ASSERT (BufferSize >= WriteIncr + MaxRecordLength);



// constructor
//   fname      pathname of output file
//   ftype      type of file: normal or compressed
// The file is created with mode (0666 & ~umask)

GdsWriter::GdsWriter (const char* fname, FileHandle::FileType ftype)
  : wbuf(BufferSize),
    fh(fname, ftype),
    filename(fname)
{
    recStart = Null;
    currType = Null;
}



GdsWriter::~GdsWriter()
{
    // If fdout is still open, either the writer was used improperly or
    // an exception was thrown.  Either way it doesn't matter if close
    // fails.

    fh.close();
}



// beginFile -- begin writing the GDSII file.

void
GdsWriter::beginFile()
{
    fh.open(FileHandle::WriteAccess);
}



// endFile -- end writing the GDSII file.
// Writes to disk all the data in the buffer.

void
GdsWriter::endFile()
{
    // If the output buffer is not empty, write its contents to the file.

    if (wbuf.charsUsed() != 0  &&  fh.write(wbuf.buffer, wbuf.charsUsed()) < 0)
        abortWriter();

    // Close the file here rather than in the destructor so that we can
    // throw an exception if close fails.  Destructors must not throw.

    if (fh.close() < 0)
        abortWriter();
}


//----------------------------------------------------------------------
// Many records contain exactly one data item.  To simplify code
// that uses this class, supply combinations of
//    beginRecord(); writeXxx(); endRecord();
// for various Xxx.


void
GdsWriter::writeBitArrayRecord (GdsRecordType recType, Uint val)
{
    beginRecord(recType);
    writeBitArray(val);
    endRecord();
}



void
GdsWriter::writeShortRecord (GdsRecordType recType, int val)
{
    beginRecord(recType);
    writeShort(val);
    endRecord();
}



void
GdsWriter::writeIntRecord (GdsRecordType recType, int val)
{
    beginRecord(recType);
    writeInt(val);
    endRecord();
}



void
GdsWriter::writeDoubleRecord (GdsRecordType recType, double val)
{
    beginRecord(recType);
    writeDouble(val);
    endRecord();
}



void
GdsWriter::writeStringRecord (GdsRecordType recType, const char* str)
{
    beginRecord(recType);
    writeString(str, strlen(str));
    endRecord();
}



//----------------------------------------------------------------------
// Private methods


// ensureSpaceForRecord -- ensure the buffer has enough space for next record.
// startRecord() calls this method to ensure that the buffer has enough
// free space to hold the largest possible GDSII record (65534 bytes).
// There are two reasons for this:
//
//   - We need not check for a full buffer every time we write a byte.
//
//   - Since startRecord() does not know how long the record will be, it
//     leaves the length field in the header blank and lets endRecord()
//     fill it.  This means that the record header must still be in
//     the buffer when the record ends.

void
GdsWriter::ensureSpaceForRecord()
{
    const Uint  WriteSize =
                     ((BufferSize - MaxRecordLength)/WriteIncr) * WriteIncr;
        // How many bytes to pass to write().  It must be a multiple
        // of WriteIncr.

    // If the write buffer has fewer than WriteSize chars, then by the
    // definition above the buffer has space for at least
    // MaxRecordLength more chars, so the new record will fit.

    if (wbuf.charsUsed() < WriteSize)
        return;

    // Write WriteSize bytes and move the remaining bytes to the beginning
    // of the buffer.  We don't just empty the buffer because we want to
    // write only full blocks.  That is why WriteSize must be a multiple of
    // WriteIncr.

    if (fh.write(wbuf.buffer, WriteSize) < 0)
        abortWriter();
    wbuf.flush(WriteSize);
}



// abortWriter -- abort writer because a write failed

/*private*/ void
GdsWriter::abortWriter()
{
    ThrowRuntimeError("file '%s': write failed: %s",
                      filename.c_str(), strerror(errno));
}


}  // namespace Gdsii
