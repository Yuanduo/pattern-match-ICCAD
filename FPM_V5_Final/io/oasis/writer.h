// oasis/writer.h -- low-level (lexical) interface for writing OASIS files
//
// last modified:   28-Dec-2009  Mon  21:10
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef OASIS_WRITER_H_INCLUDED
#define OASIS_WRITER_H_INCLUDED

#include <cassert>
#include <cstring>
#include <limits>
#include <string>
#include <sys/types.h>          // for off_t

#include "port/compiler.h"
#include "misc/buffer.h"
#include "oasis.h"
#include "rectypes.h"


namespace Oasis {

using std::string;
using SoftJin::llong;
using SoftJin::Ullong;
using SoftJin::Ulong;
using SoftJin::WriteBuffer;

class Compressor;       // compressor.h
class ZlibDeflater;     // compressor.h
class Validator;        // validator.h


/*----------------------------------------------------------------------

OasisWriter -- low-level interface for writing OASIS files
This class hides the buffering, CRC/checksum computation, compression
of CBLOCKs, and the file representation of the primitive types
(integers, reals, strings, and deltas).

Data members

    fileBuf             WriteBuffer
        Buffer for writing to file.  When the writer is not in a
        CBLOCK (compressor == Null), data is written directly into
        this buffer and flushed to the file when the buffer becomes full.
        fileBuf.curr is updated only when switching to cblockBuf.

    cblockBuf           WriteBuffer
        Buffer for accumulating data before it is compressed into
        a CBLOCK.  Used when the writer is in a CBLOCK (compressor != Null).
        When the buffer is full its contents are passed to the compressor,
        which writes the compressed output to fileBuf.
        cblockBuf.curr is updated only when switching to fileBuf.

    bufptr              char*
        Pointer into fileBuf.buffer or cblockBuf.buffer.  Points to
        where the next output character will be written.  Only this
        pointer is updated when bytes are being written.  When
        switching buffers, its value is saved in the curr member of
        the original buffer and then the curr value from the new buffer
        is copied to it.

    bufend              char*
        The end of the current buffer: either fileBuf.end or cblockBuf.end.
        If bufptr == bufend, the current buffer is full and must be flushed.

    fileOffset          off_t
        The offset in the file of the first byte of file.buffer.
        This is also the offset of the file pointer referenced by fdio.

    endRecordOffset     off_t
        Defined only when the upper layer has begun writing the END
        record, i.e it has invoked beginRecord(RID_END).  It is the
        file offset of the first byte of the END record.  finishEndRecord()
        uses it to calculate how much padding to insert to make the
        END record's size exactly EndRecordSize.

    deflater            ZlibDeflater*
        The owning pointer to a concrete strategy class for DEFLATE
        compression.  The scanner instantiates this lazily, the first
        time the upper layer asks for a DEFLATE-compressed CBLOCK.

    compressor
        The current compression strategy class.  Null means that the
        writer is not in a CBLOCK.  Since the current spec defines DEFLATE
        as the only compression method, this must have the same value
        as deflater if it is non-Null.

    cblockCountOffset   off_t
        Valid only when the writer is compressing data, i.e., when
        compressor != Null.  It is the offset in the file at which the
        two CBLOCK counts (uncomp-bytes and comp-bytes) must be written.
        The writer can fill in these counts only when the CBLOCK ends.
        The bytes at this offset may still be in the buffer or may have
        been flushed to disk.

    compBytes           Ulong
        Valid only when compressor != Null.  The current compressed size
        of the CBLOCK.  How many bytes of compressed data the compressor
        has output so far.

    uncompBytes         Ulong
        Valid only when compressor != Null.  The current uncompressed
        size of the CBLOCK.  This is the number of bytes passed to
        the compressor; it does not include the bytes now sitting
        in cblockBuf.


    valScheme           Validation::Scheme
        What validation data to put at the end of the file:
        None, CRC32, or Checksum32.

    validator           Validator*
        Null if valScheme is Validation::None.  Otherwise points to
        an instance of the appropriate validation strategy class.

    crcNextOffset       off_t
        Valid only when valScheme is Validation::CRC32.  It is the
        offset of the first byte not included in the CRC computation.
        If crcNextOffset < fileOffset, it means that CRC computation was
        halted because the start of a CBLOCK had to be flushed to disk
        with the two byte counts still unwritten.  When the file ends,
        the writer returns to this offset and finishes calculating the
        CRC by reading what it wrote.

        Invariant:
        fileOffset >= crcNextOffset

------------------------------------------------------------------------*/

class OasisWriter {
    // Buffers
    WriteBuffer<char>  fileBuf;
    WriteBuffer<char>  cblockBuf;
    char*       bufptr;
    char*       bufend;

    off_t       fileOffset;
    string      filename;
    int         fdio;
    off_t       endRecordOffset;

    // Compression
    ZlibDeflater*  deflater;
    Compressor*    compressor;
    off_t       cblockCountOffset;
    Ullong      compBytes;
    Ullong      uncompBytes;

    // Validation
    Validation::Scheme  valScheme;      // None, CRC32, Checksum32
    Validator*  validator;
    off_t       crcNextOffset;

public:
    explicit    OasisWriter (const char* fname);
                ~OasisWriter();

    void        beginFile (Validation::Scheme valScheme);
    void        endFile();

    off_t       currFileOffset() const;
    void        beginRecord (RecordID recID);
    void        beginCblock (Ulong compType);
    void        endCblock();

    void        writeByte (int byte);
    void        writeBytes (const void* startAddr, size_t nbytes);

    void        writeUnsignedInteger (Ulong val);
    void        writeSignedInteger (long val);
    void        writeUnsignedInteger64 (Ullong val);
    void        writeSignedInteger64 (llong val);

    void        writeIntegerReal (long val);
    void        writeReciprocalReal (long val);
    void        writeRatioReal (long numerator, long denominator);
    void        writeFloatReal (float val);
    void        writeDoubleReal (double val);
    void        writeReal (double val);
    void        writeReal (const Oreal& val);

    void        writeString (const string& str);
    void        writeString (const char* str);

    void        writeOneDelta (long offset);
    void        writeOneDelta (Delta delta);
    void        writeTwoDelta (Delta::Direction dirn, Ulong offset);
    void        writeTwoDelta (Delta delta);
    void        writeThreeDelta (Delta::Direction dirn, Ulong offset);
    void        writeThreeDelta (Delta delta);
    void        writeGDelta (Delta delta);

private:
    void        flushBuffer();
    void        flushFileBuffer();
    void        updateValidationSignature (off_t offset, const char* data,
                                           Uint nbytes);
    void        updateCrcFromFile();
    void        flushCblockBuffer (bool endOfInput);
    void        writeAtOffset (const void* buf, size_t count, off_t offset);
    void        seekTo (off_t offset);
    void        finishEndRecord();
    bool        bufferIsFull() const;
    bool        writingToCblock() const;
    void        makeUnsignedInteger64 (/*out*/ char* buf, Uint nbytes,
                                       Ullong val);

    void        formatContext (/*out*/ char* buf, size_t bufsize);
    void        abortWriter (const char* fmt, ...)
                                         SJ_PRINTF_ARGS(2,3) SJ_NORETURN;

private:
                OasisWriter (const OasisWriter&);       // forbidden
    void        operator= (const OasisWriter&);         // forbidden
};



// currFileOffset -- get file offset of next byte to be written.
// Precondition: the writer is not currently compressing the data.

inline off_t
OasisWriter::currFileOffset() const {
    assert (compressor == Null);
    return (fileOffset + (bufptr - fileBuf.buffer));
}


// writeOneDelta -- write a 1-delta (a signed offset with implied horiz/vert)

inline void
OasisWriter::writeOneDelta (long offset) {
    writeSignedInteger(offset);
}


inline void
OasisWriter::writeOneDelta (Delta delta) {
    assert (delta.isManhattan());
    writeSignedInteger(delta.x != 0 ? delta.x : delta.y);
}


} // namespace Oasis

#endif  // OASIS_WRITER_H_INCLUDED
