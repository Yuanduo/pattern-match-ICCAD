// oasis/writer.cc -- low-level (lexical) interface for writing OASIS files
//
// last modified:   01-Jan-2010  Fri  09:29
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <algorithm>            // for min and max
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <fcntl.h>
#include <limits>
#include <boost/scoped_array.hpp>
#include <boost/static_assert.hpp>

#include "misc/utils.h"
#include "compressor.h"
#include "rectypes.h"
#include "validator.h"
#include "writer.h"


namespace Oasis {

using namespace std;
using namespace SoftJin;
using boost::scoped_array;


const Uint  WriteIncr = 8*1024;
    // Writes to the file will be a multiple of this size.  The ideal
    // value is the block size of the filesystem.


const Uint  FileBufferSize = 1024*1024;
    // Size of the main buffer that holds data to be written to the
    // file.  Should be large.  If the application chooses CRC32
    // validation and writes a cblock bigger than about this size, the
    // writer will need a second pass over the file to compute the CRC.


const Uint  CblockBufferSize = 128*1024;
    // Size of buffer used to hold uncompressed cblock data.  Should be
    // smaller than FileBufferSize.  There is no need to make this
    // large because a cblock can be compressed one piece at a time.


const Uint  BytesForUllong = (numeric_limits<Ullong>::digits+6) / 7;
    // It takes at most this many bytes to write a Ullong in the OASIS
    // format, with 7 data bits per byte.


//----------------------------------------------------------------------
//                      Initialization and termination
//----------------------------------------------------------------------

// constructor
//   fname      pathname of file to write

OasisWriter::OasisWriter (const char* fname)
  : fileBuf(FileBufferSize),
    filename(fname)
{
    bufptr = fileBuf.buffer;
    bufend = fileBuf.end;
    fileOffset = 0;
    endRecordOffset = 0;
    fdio = -1;
    compressor = Null;
    deflater = Null;
    validator = Null;
    valScheme = Validation::None;
}



OasisWriter::~OasisWriter()
{
    delete validator;
    delete deflater;

    // If the file is still open, either endFile() was never called or
    // the close() in it failed.  Either way we don't care if this close()
    // fails.

    if (fdio >= 0)
        close(fdio);
}



// beginFile -- open the file
//   valScheme  validation scheme (None, CRC32, Checksum32)
// This must be the first method called.

void
OasisWriter::beginFile (Validation::Scheme valScheme)
{
    // Open the file read-write instead of write-only.  We may want to
    // read part of what we wrote to finish a CRC computation that
    // we suspended in the middle.

    fdio = OpenFile(filename.c_str(), O_RDWR|O_TRUNC|O_CREAT, 0666);

    // Instantiate the concrete strategy class to compute the checksum
    // or CRC.  Leave validator Null if valScheme == Validation::None.

    this->valScheme = valScheme;
    switch (valScheme) {
        case Validation::CRC32:
            validator = new Crc32Validator;
            // 14.4.2:  CRC starts from first byte of <magic-bytes>.
            crcNextOffset = 0;
            break;
        case Validation::Checksum32:
            validator = new Checksum32Validator;
            break;
        default:        // make gcc happy
            break;
    }
}



// endFile -- finish END record, flush all buffered output, and close file
// This must be the last method called.
// Precondition:
//    The argument to the last call to beginRecord() was RID_END.

void
OasisWriter::endFile()
{
    assert (endRecordOffset > 0);

    finishEndRecord();
    size_t  nbytes = bufptr - fileBuf.buffer;
    if (WriteNBytes(fdio, fileBuf.buffer, nbytes) < 0)
        abortWriter("write failed: %s", strerror(errno));
    if (close(fdio) < 0)
        abortWriter("close failed: %s", strerror(errno));

    fdio = -1;          // destructor shouldn't try to close it again
}



//----------------------------------------------------------------------
//                          Buffer management
//----------------------------------------------------------------------
// All these methods are private.

inline bool
OasisWriter::bufferIsFull() const {
    return (bufptr == bufend);
}



// writingToCblock -- true if the writer is in a cblock.

inline bool
OasisWriter::writingToCblock() const {
    return (compressor != Null);
}



// flushBuffer -- flush the current buffer
// Postcondition:
//   The current buffer is not full (i.e., bufptr != bufend)

void
OasisWriter::flushBuffer()
{
    // Only bufptr gets updated while a buffer is being written to, not
    // buffer.curr.  Copy bufptr to buffer.curr before flushing so that
    // flushCblockBuffer() and flushFileBuffer() know how much data is
    // in the buffer.  Those functions may leave some data in the buffer,
    // so copy the updated buffer.curr back to bufptr.

    if (writingToCblock()) {
        cblockBuf.curr = bufptr;
        flushCblockBuffer(false);
        bufptr = cblockBuf.curr;
    } else {
        fileBuf.curr = bufptr;
        flushFileBuffer();
        bufptr = fileBuf.curr;
    }
}



// flushFileBuffer -- write part or all of fileBuf's contents to the file.
// Postcondition:
//   fileBuf.charsUsed() < WriteIncr
//
void
OasisWriter::flushFileBuffer()
{
    // To avoid writing parts of blocks, round down the write count
    // to a multiple of WriteIncr.

    Uint  bytesToWrite = (fileBuf.charsUsed() / WriteIncr) * WriteIncr;
    if (bytesToWrite == 0)
        return;

    // Update the validation signature with the bytes about to be written.
    updateValidationSignature(fileOffset, fileBuf.buffer, bytesToWrite);

    // Write the bytes to the file, move whatever is left to the
    // beginning of the buffer, and adjust fileOffset to match.

    if (WriteNBytes(fdio, fileBuf.buffer, bytesToWrite) < 0)
        abortWriter("write failed: %s", strerror(errno));
    fileBuf.flush(bytesToWrite);
    fileOffset += bytesToWrite;
}


//----------------------------------------------------------------------
//                              Validation
//----------------------------------------------------------------------
// All these methods are private.

// updateValidationSignature -- include more data in validation signature
//   offset     file offset of the new data.  Used by the CRC32 validator.
//   data       pointer to beginning of data buffer
//   nbytes     number of bytes at data to include in signature computation

void
OasisWriter::updateValidationSignature (off_t offset, const char* data,
                                        Uint nbytes)
{
    if (valScheme == Validation::None)
        return;

    // Checksum signatures are not affected by CBLOCKs.  We need not
    // worry about offset because bytes can be checksummed in any order.

    if (valScheme == Validation::Checksum32) {
        validator->add(data, nbytes);
        return;
    }

    // CRC32 validation.  See if we suspended the computation earlier.
    // Since the CRC must be computed sequentially, the computation
    // remains suspended to the end.

    if (offset != crcNextOffset)
        return;

    // If the bytes being flushed to disk include any of the filler
    // bytes that beginCompression() put for the CBLOCK byte-counts,
    // suspend CRC computation because those bytes will be overwritten
    // later by the real counts.

    off_t  endOffset = offset + nbytes;
    if (writingToCblock()  &&  endOffset > cblockCountOffset)
        return;

    validator->add(data, nbytes);
    crcNextOffset += nbytes;
}



// updateCrcFromFile -- finish suspended CRC computation by reading file
//
// Preconditions:
//   valScheme == Validation::CRC32
//   fileOffset >= crcNextOffset
//
// Postcondition:
//   fileOffset == crcNextOffset

void
OasisWriter::updateCrcFromFile()
{
    assert (valScheme == Validation::CRC32);
    assert (fileOffset >= crcNextOffset);

    // There is nothing to do if we never suspended computing the CRC.
    if (fileOffset == crcNextOffset)
        return;

    // Allocate buffer in which to read the file.  It's better to
    // allocate large arrays in the free store rather than on the stack,
    // in case this is used in a multi-threaded program.

    const Uint  CrcBufferSize = 64*1024;
    scoped_array<char> crcbuf(new char[CrcBufferSize]);

    // Read all bytes starting from crcNextOffset (which is where we
    // suspended the CRC computation) and include them in the CRC.

    int  nr;
    seekTo(crcNextOffset);
    while ((nr = read(fdio, crcbuf.get(), CrcBufferSize)) > 0) {
        validator->add(crcbuf.get(), nr);
        crcNextOffset += nr;
    }
    if (nr < 0)
        abortWriter("read failed: %s", strerror(errno));

    // The file should end at fileOffset, the offset of the first byte
    // not yet written.

    if (crcNextOffset != fileOffset)
        abortWriter("internal error in OasisWriter::updateCrcFromFile()");
}



//----------------------------------------------------------------------
//                              Compression
//----------------------------------------------------------------------

// beginCblock -- begin compressing data for CBLOCK
//   compType   compression type.  The only value currently valid
//              is DeflateCompression.
// Precondition:   ! writingToCblock();
// Postcondition:  writingToCblock();

/*public*/ void
OasisWriter::beginCblock (Ulong compType)
{
    // Can't have a CBLOCK inside another CBLOCK.
    assert (! writingToCblock());

    // A CBLOCK record looks like this (spec 35.1):
    //    `34' comp-type uncomp-byte-count comp-byte-count comp-bytes
    // We can write the two byte-counts only at the end of the cblock.

    writeUnsignedInteger(RID_CBLOCK);
    writeUnsignedInteger(compType);

    // Free as much space in the file buffer as possible.  When the
    // cblock ends we have to fill in the uncomp-byte-count and
    // comp-byte-count that precede the compressed data.  If the part of
    // the buffer that contained the byte counts gets flushed to disk
    // before the counts are filled in, we have to suspend computing the
    // CRC (if that is the validation scheme chosen) and at the end make
    // another pass of the file to finish it.  Because this is slow, we
    // try to keep the entire cblock in the buffer by clearing the buffer
    // before beginning the cblock.
    //
    // XXX: Should measure cblock sizes in real OASIS files.  There is
    // no point in jumping through these hoops if large files have large
    // cblocks that usually overflow the buffer.  For small files it
    // doesn't matter if we make a separate pass to compute the CRC.

    flushBuffer();

    // Note the file offset where we must write the byte-counts, and
    // leave space for them.  The filler bytes must be 0 to keep them
    // from affecting the checksum, if that is the validation scheme
    // chosen.

    cblockCountOffset = currFileOffset();
    for (int j = 2*BytesForUllong;  --j >= 0;  )
        writeByte(0);

    // If this is the first cblock in the file, allocate the buffer for
    // the uncompressed data.

    if (cblockBuf.buffer == Null)
        cblockBuf.create(CblockBufferSize);

    // Set compressor to an instance of the appropriate compression
    // strategy class.  In the current spec DEFLATE is the only cblock
    // type, but that may not be true in the future.  If this is the
    // first DEFLATE cblock in the file, create the deflater.

    assert (compType == DeflateCompression);
    if (deflater == Null)
        deflater = new ZlibDeflater;
    compressor = deflater;

    // So far we have been writing into fileBuf.  Everything from now on
    // should be compressed.  Save bufptr (the current write pointer) in
    // fileBuf.curr and switch to cblockBuf.

    fileBuf.curr = bufptr;
    bufptr = cblockBuf.buffer;
    bufend = cblockBuf.end;

    // Compressed and uncompressed sizes of the cblock.
    compBytes = 0;
    uncompBytes = 0;

    // Reinitialize the compressor for this block.  The context arg
    // contains the output file name.  The compressor will use that as
    // a prefix for the error message if it throws an exception.

    char  context[100];
    formatContext(context, sizeof context);
    compressor->beginBlock(context);
}



// flushCblockBuffer -- compress all the data in cblockBuf to fileBuf.
//   endOfInput         true => the cblock has ended
//
// Preconditions:
//     writingToCblock()
//     compressor->needInput()
//
// Postconditions:
//     writingToCblock()
//     compressor->needInput()
//     cblockBuf.charsUsed() == 0   (i.e., cblockBuf is empty)

/*private*/ void
OasisWriter::flushCblockBuffer (bool endOfInput)
{
    assert (writingToCblock());
    assert (compressor->needInput());

    Uint  inbytes;      // num bytes in cblockBuf to be compressed
    Uint  outbytes;     // num bytes output by compressor in each iter

    // Pass whatever is in cblockBuf to the compressor.

    inbytes = cblockBuf.charsUsed();
    compressor->giveInput(cblockBuf.buffer, inbytes, endOfInput);
    uncompBytes += inbytes;

    // Keep collecting compressed data from the compressor until it has
    // used up all its input.  Make sure there is a reasonable amount of
    // free space in fileBuf; the compressor may not like it if there is
    // only 1 byte free.

    do {
        if (fileBuf.charsFree() < 100)          // arbitrary small value
            flushFileBuffer();
        outbytes = compressor->getOutput(fileBuf.curr, fileBuf.charsFree());
        compBytes += outbytes;
        fileBuf.curr += outbytes;
    } while (outbytes > 0);

    cblockBuf.flushAll();
    assert (compressor->needInput());
}



// endCblock -- end compressing data for cblock
// Precondition:   writingToCblock();
// Postcondition:  ! writingToCblock();

/*public*/ void
OasisWriter::endCblock()
{
    cblockBuf.curr = bufptr;
    flushCblockBuffer(true);
    compressor = Null;

    // At the start of the cblock we left 2*BytesForUllong bytes blank
    // for the uncomp-byte-count and comp-byte-count fields that precede
    // the compressed data in a cblock.  We now have to overwrite those
    // bytes with the correct values.

    // First write the two numbers into the temporary buffer byteCounts
    // so that each number occupies exactly BytesForUllong bytes.

    char  byteCounts[2*BytesForUllong];
    makeUnsignedInteger64(byteCounts, BytesForUllong, uncompBytes);
    makeUnsignedInteger64(byteCounts+BytesForUllong, BytesForUllong, compBytes);

    // The member cblockCountOffset gives the file offset where we must
    // write the first byte of byteCounts.  Define countEndOffset so
    // that the range of offsets occupied by byteCounts is
    // [cblockCountOffset, countEndOffset).
    //
    // countEndOffset   1 + offset of last byte of byteCounts.

    off_t  countEndOffset = cblockCountOffset + 2*BytesForUllong;

    // If some or all of the bytes in that range are still in the
    // buffer, overwrite them.
    //
    // startOffset      offset of first byte of byteCounts still in buffer
    // nbytes           number of bytes of byteCounts still in buffer

    if (countEndOffset > fileOffset) {
        off_t  startOffset = max(cblockCountOffset, fileOffset);
        Uint  nbytes = countEndOffset - startOffset;
        memcpy(fileBuf.buffer + (startOffset - fileOffset),
               byteCounts + (startOffset - cblockCountOffset),
               nbytes);
    }

    // If some or all of the count bytes have been flushed to disk,
    // overwrite the bytes on disk with the new values.  Also include
    // these bytes in the validation signature.  We need to do it here
    // because these bytes are bypassing fileBuf.  Note that only the
    // Checksum32 validator actually updates the signature.  The CRC32
    // validator will remain suspended until the end of the file.
    //
    // endOffset        1 + offset of last byte of byteCounts on disk
    // nbytes           number of bytes of byteCounts on disk

    if (cblockCountOffset < fileOffset) {
        off_t  endOffset = min(countEndOffset, fileOffset);
        Uint  nbytes = endOffset - cblockCountOffset;

        writeAtOffset(byteCounts, nbytes, cblockCountOffset);
        updateValidationSignature(cblockCountOffset, byteCounts, nbytes);
    }

    // Switch back to fileBuf.
    bufptr = fileBuf.curr;
    bufend = fileBuf.end;
}



//----------------------------------------------------------------------
//                            File operations
//----------------------------------------------------------------------

// writeAtOffset -- write bytes at given offset in file
//   buf        bytes to write
//   count      number of bytes to write
//   offset     offset in file at which to write the bytes

void
OasisWriter::writeAtOffset (const void* buf, size_t count, off_t offset)
{
    // We could use pwrite(), but this is as simple.  We have no
    // worries about concurrent access to the file descriptor.

    seekTo(offset);
    if (WriteNBytes(fdio, buf, count) < 0)
        abortWriter("write failed: %s", strerror(errno));
    seekTo(fileOffset);
}



void
OasisWriter::seekTo (off_t offset)
{
    if (lseek(fdio, offset, SEEK_SET) != offset)
        abortWriter("lseek failed: %s", strerror(errno));
}


//----------------------------------------------------------------------


/*public*/ void
OasisWriter::beginRecord (RecordID recID)
{
    // beginCblock() should be used for CBLOCKs
    assert (recID != RID_CBLOCK);

    // Save the END record's starting offset for finishEndRecord() below.
    if (recID == RID_END)
        endRecordOffset = currFileOffset();

    writeUnsignedInteger(recID);
}



// finishEndRecord -- write padding string and validation in END record.
// endFile() calls this.

/*private*/ void
OasisWriter::finishEndRecord()
{
    assert (endRecordOffset > 0  &&  endRecordOffset < currFileOffset());
    assert (! writingToCblock());

    // See how much space we need for the validation info.  This
    // consists of an unsigned-integer (the validation scheme) and, for
    // CRC32 and Checksum32, a 32-bit signature.  We assume that the
    // scheme fits into one byte, i.e., it is less than 128.

    assert (valScheme < 128);
    Uint  validationSize = (valScheme == Validation::None) ? 1 : 5;

    // Before the validation info we must write a padding string of NUL
    // bytes sized to make the record exactly EndRecordSize bytes.
    // Calculate how much space this string should take.  Then calculate
    // the string length, create a temporary string of that length
    // containing NULs, and write it.
    //
    // A string is represented as an unsigned-integer size followed by
    // the data bytes.  So how long the padding string should be depends
    // on whether we need one or two bytes for its size.  An OASIS
    // unsigned-integer contains 7 data bits per byte.

    Uint  currEndSize = currFileOffset() - endRecordOffset;
    if (currEndSize + validationSize >= EndRecordSize)
        abortWriter("end record size exceeds %u", EndRecordSize);

    Uint  padSize = EndRecordSize - currEndSize - validationSize;
    Uint  padStringLen = (padSize <= 128 ? padSize-1 : padSize-2);
    writeString(string(padStringLen, Cnul));

    // Write the validation scheme and then finish computing the
    // validation signature, which includes all bytes up to the scheme.
    // For CRC32 validation we might earlier have suspended computing
    // the validation because of a cblock whose header got flushed
    // before we could fill in the sizes.  If so, finish computing the
    // CRC from the file before processing the data still in the buffer.

    writeUnsignedInteger(valScheme);
    if (valScheme == Validation::CRC32)
        updateCrcFromFile();
    updateValidationSignature(fileOffset, fileBuf.buffer,
                              bufptr - fileBuf.buffer);

    // For the CRC32 and Checksum32 schemes, write the 32-bit signature
    // in little-endian byte order.

    if (valScheme != Validation::None) {
        uint32_t  sig = validator->getSignature();
        for (int j = 0;  j < 4;  ++j) {
            writeByte(sig);
            sig >>= 8;
        }
    }
}



// writeByte -- write least-significant byte of arg

void
OasisWriter::writeByte (int byte)
{
    if (bufferIsFull())
        flushBuffer();
    *bufptr++ = byte;
}



void
OasisWriter::writeBytes (const void* addr, size_t nbytes)
{
    while (nbytes > 0) {
        if (bufferIsFull())
            flushBuffer();
        size_t  n = min(nbytes, static_cast<size_t>(bufend - bufptr));
        memcpy(bufptr, addr, n);
        addr = static_cast<const char*>(addr) + n;
        bufptr += n;
        nbytes -= n;
    }
}



//----------------------------------------------------------------------
//                              Writing tokens
//----------------------------------------------------------------------

// Write unsigned and signed integers.  Section 7.2


void
OasisWriter::writeUnsignedInteger (Ulong val)
{
    // Write the number in little-endian order.  Each byte's leftmost
    // bit is the continuation bit: 0 for the last byte and 1 for the
    // earlier bytes.  The remaining 7 bits contain the data.

    while (val > 0x7f) {
        writeByte(0x80 | val);
        val >>= 7;
    }
    writeByte(val);
}



void
OasisWriter::writeSignedInteger (long val)
{
    Ulong       absval;         // abs(val)
    int         signBit;        // low bit of first byte: 1 if val < 0

    if (val >= 0) {
        absval = val;
        signBit = 0;
    } else {
        absval = -val;
        signBit = 0x1;
    }

    // A signed-integer is like an unsigned-integer (see above) except
    // that the first byte contains only 6 bits of data.  The high bit
    // of each byte is set iff more bytes follow.  The low bit of the
    // first byte is the sign bit: 0 for positive and 1 for negative
    // numbers.  The first byte thus has 6 data bits and the remaining
    // bytes have 7 data bits.

    int  moreBit = (absval > 0x3f ? 0x80 : 0);
    writeByte(moreBit | ((absval & 0x3f) << 1) | signBit);
    absval >>= 6;

    if (moreBit) {
        while (absval > 0x7f) {
            writeByte(0x80 | absval);
            absval >>= 7;
        }
        writeByte(absval);
    }
}



// writeUnsignedInteger64() and writeSignedInteger64() parallel
// readUnsignedInteger64() and readSignedInteger64() in OasisScanner.
// See the comment before those functions in scanner.cc.


void
OasisWriter::writeUnsignedInteger64 (Ullong val)
{
    while (val > 0x7f) {
        writeByte(0x80 | val);
        val >>= 7;
    }
    writeByte(val);
}



void
OasisWriter::writeSignedInteger64 (llong val)
{
    // As in the OasisScanner methods, allow the compiler to eliminate
    // most of this function on 64-bit systems.  We do not have similar
    // code in writeUnsignedInteger64() because that is already small.

    if (sizeof(llong) == sizeof(long)) {
        writeSignedInteger(static_cast<long>(val));
        return;
    }

    Ullong      absval;         // abs(val)
    int         signBit;        // low bit of first byte: 1 if val < 0

    if (val >= 0) {
        absval = val;
        signBit = 0;
    } else {
        absval = -val;
        signBit = 0x1;
    }

    // A signed-integer is like an unsigned-integer (see above) except
    // that the first byte contains only 6 bits of data.  The high bit
    // of each byte is set iff more bytes follow.  The low bit of the
    // first byte is the sign bit: 0 for positive and 1 for negative
    // numbers.  The first byte thus has 6 data bits and the remaining
    // bytes have 7 data bits.

    int  moreBit = (absval > 0x3f ? 0x80 : 0);
    writeByte(moreBit | ((absval & 0x3f) << 1) | signBit);
    absval >>= 6;

    if (moreBit) {
        while (absval > 0x7f) {
            writeByte(0x80 | absval);
            absval >>= 7;
        }
        writeByte(absval);
    }
}



// makeUnsignedInteger64 -- represent 64-bit uint with given number of bytes
//   buf        out: where to store the representation
//   nbytes     how many bytes to use for the representation.
//   val        the value to represent
//
// makeUnsignedInteger64() writes val into buf as an OASIS
// unsigned-integer.  If val needs fewer than nbytes bytes, zeros are
// added (with the continuation bit) to make it occupy exactly nbytes
// bytes.  makeUnsignedInteger64() aborts the writer if the value cannot
// be represented in the given space.
//
// This function is needed for the byte-count fields at the beginning of
// a cblock.  The writer leaves the fields blank initially.  When it
// knows the value it returns and fills in the fields, but the values it
// writes must occupy exactly the bytes it left blank, no more and no
// less.

/*private*/ void
OasisWriter::makeUnsignedInteger64 (/*out*/ char* buf, Uint nbytes, Ullong val)
{
    assert (nbytes > 0);

    Ullong  rem = val;
    for (Uint j = 0;  j < nbytes;  ++j) {
        buf[j] = 0x80 | rem;
        rem >>= 7;
    }
    buf[nbytes-1] &= 0x7f;      // reset continuation bit in last byte

    if (rem != 0)
        abortWriter("value %llu does not fit in %u bytes", val, nbytes);
}



//----------------------------------------------------------------------
// Write reals.  Section 7.3


// writeIntegerReal -- write real as integer (type 0 or 1)

void
OasisWriter::writeIntegerReal (long val)
{
    if (val >= 0) {
        writeByte(Oreal::PosInteger);
        writeUnsignedInteger(val);
    } else {
        writeByte(Oreal::NegInteger);
        writeUnsignedInteger(-val);
    }
}



// writeReciprocalReal -- write real as reciprocal of integer (type 2 or 3)

void
OasisWriter::writeReciprocalReal (long val)
{
    if (val >= 0) {
        writeByte(Oreal::PosReciprocal);
        writeUnsignedInteger(val);
    } else {
        writeByte(Oreal::NegReciprocal);
        writeUnsignedInteger(-val);
    }
}



// writeRatioReal -- write real as ratio of two integers (type 4 or 5)

void
OasisWriter::writeRatioReal (long numer, long denom)
{
    writeByte ((numer < 0) == (denom < 0) ? Oreal::PosRatio : Oreal::NegRatio);
    writeUnsignedInteger(abs(numer));
    writeUnsignedInteger(abs(denom));
}



// writeFloatReal -- write real as single-precision float (type 6)

void
OasisWriter::writeFloatReal (float val)
{
    // We assume that the C/C++ type `float' has the same representation
    // (IEEE 754) as OASIS, apart from endianness.  So we simply copy
    // the variable to the file, after reversing bytes if the processor
    // is big-endian.  OASIS wants the bytes in little-endian order.

    BOOST_STATIC_ASSERT (sizeof(float) == 4);
    writeByte(Oreal::Float32);
    if (ProcessorIsLittleEndian())
        writeBytes(&val, 4);
    else {
        char  cval[4];
        const char*  src = reinterpret_cast<const char*>(&val);
        reverse_copy(src, src+4, cval);
        writeBytes(cval, 4);
    }
}



// writeDoubleReal -- write real as double-precision float (type 7)

void
OasisWriter::writeDoubleReal (double val)
{
    BOOST_STATIC_ASSERT (sizeof(double) == 8);
    writeByte(Oreal::Float64);
    if (ProcessorIsLittleEndian())
        writeBytes(&val, 8);
    else {
        char   cval[8];
        const char*  src = reinterpret_cast<const char*>(&val);
        reverse_copy(src, src+8, cval);
        writeBytes(cval, 8);
    }
}



// writeReal -- write a real in the most compact form that is convenient.
// Two overloaded methods.

void
OasisWriter::writeReal (const Oreal& val)
{
    // If val is rational, it is an integer, reciprocal of integer, or
    // ratio of two integers.  We don't need to check for denom == -1
    // because Oreal's class invariant is that the denominator is
    // positive.

    if (val.isRational()) {
        long  numer = val.getNumerator();
        long  denom = val.getDenominator();
        if (denom == 1)
            writeIntegerReal(numer);
        else if (numer == 1)
            writeReciprocalReal(denom);
        else if (numer == -1)
            writeReciprocalReal(-denom);
        else
            writeRatioReal(numer, denom);
    }
    else if (val.isFloatSingle())
        writeFloatReal(val.getValue());
    else {
        // The upper layers sometimes find it more convenient to create
        // an Oreal as a double instead of looking for the optimal
        // representation.  So if it's a double, check if we can use one of
        // the more specialised forms.
        //
        writeReal(val.getValue());
    }
}



void
OasisWriter::writeReal (double val)
{
    long        lval;
    float       fval;

    // See if val is an integer.
    if (DoubleIsIntegral(val, &lval)) {
        writeIntegerReal(lval);
        return;
    }

    // See if val is the reciprocal of an integer.  We know that val != 0.
    if (fabs(val) < 1.0  &&  DoubleIsIntegral(1.0/val, &lval)) {
        writeReciprocalReal(lval);
        return;
    }

    // I don't know any reliable way to convert doubles to ratios,
    // so skip that.

    // See if val can be represented exactly in a (32-bit) float.
    if (DoubleIsFloat(val, &fval)) {
        writeFloatReal(fval);
        return;
    }

    // If all else fails, write the double as is.
    writeDoubleReal(val);
}



//----------------------------------------------------------------------
// Write strings.  Section 7.4


void
OasisWriter::writeString (const string& str)
{
    writeUnsignedInteger(str.size());
    writeBytes(str.data(), str.size());
}



void
OasisWriter::writeString (const char* str)
{
    Ulong  len = strlen(str);
    writeUnsignedInteger(len);
    writeBytes(str, len);
}



//----------------------------------------------------------------------
// Write deltas (displacements).  Section 7.5


// writeTwoDelta -- write a 2-delta (a Manhattan displacement)

void
OasisWriter::writeTwoDelta (Delta::Direction dirn, Ulong offset)
{
    assert (Delta::isManhattan(dirn));

    if (offset != (offset << 2) >> 2)
        abortWriter("2-delta offset %lu is too large to write", offset);
    writeUnsignedInteger((offset << 2) | dirn);
}



void
OasisWriter::writeTwoDelta (Delta delta)
{
    assert (delta.isManhattan());

    Ulong  offset = abs(delta.x + delta.y);     // either .x or .y is 0
    if (offset != (offset << 2) >> 2)
        abortWriter("2-delta offset %lu is too large to write", offset);

    Delta::Direction  dirn = delta.getDirection();
    writeUnsignedInteger((offset << 2) | dirn);
}



// writeThreeDelta -- write a 3-delta (an octangular displacement)

void
OasisWriter::writeThreeDelta (Delta::Direction dirn, Ulong offset)
{
    // A 3-delta is represented as an unsigned integer.  Bits 2..0 give
    // the direction and the remaining bits the magnitude of the
    // displacement.

    if (offset != (offset << 3) >> 3)
        abortWriter("3-delta offset %lu is too large to write", offset);
    writeUnsignedInteger((offset << 3) | dirn);
}



void
OasisWriter::writeThreeDelta (Delta delta)
{
    assert (delta.isOctangular());

    Ulong  offset = (delta.x != 0 ? abs(delta.x) : abs(delta.y));
    if (offset != (offset << 3) >> 3)
        abortWriter("3-delta offset %lu is too large to write", offset);

    Delta::Direction  dirn = delta.getDirection();
    writeUnsignedInteger((offset << 3) | dirn);
}



// writeGDelta -- write a g-delta (a general delta)

void
OasisWriter::writeGDelta (Delta delta)
{
    // First see if we can write the g-delta in the single-integer form
    // for octangular displacements.  Bit 0 is 0, bits 3..1 give the
    // direction of displacement, and the remaining bits give the magnitude
    // of the displacement.  If the displacement is too large to be
    // left-shifted by 4, fall back on the double-integer form.

    if (delta.isOctangular()) {
        Ulong  offset = (delta.x != 0 ? abs(delta.x) : abs(delta.y));
        if (offset == (offset << 4) >> 4) {
            Delta::Direction  dirn = delta.getDirection();
            writeUnsignedInteger((offset << 4) | (dirn << 1));
            return;
        }
    }

    // Use the double-integer form of the g-delta.  The first integer is
    // the X offset and the second is the Y offset.  Bit 0 of the first
    // integer is 1 (to distinguish it from the single-integer form);
    // bit 1 is 0 for East and 1 for West (like a signed-integer).  Bit 0
    // of the second integer is 0 for North, 1 for South.  This is the
    // standard representation of signed-integer with positive for North.

    Ulong  xoffset = abs(delta.x);
    if (xoffset != (xoffset << 2) >> 2)
        abortWriter("x-displacement %lu of g-delta is too large to write",
                    xoffset);
    int  bitOne = (delta.x >= 0 ? 0 : 0x2);
    writeUnsignedInteger((xoffset << 2) | bitOne | 0x1);
    writeSignedInteger(delta.y);
}


//----------------------------------------------------------------------


// formatContext -- format a string giving output filename.
//   buf        the buffer in which the string is to be formatted
//   bufsize    space available in buf
// This context string is used for error messages.  It has the form
//   file 'foo'

void
OasisWriter::formatContext (/*out*/ char* buf, size_t bufsize)
{
    SNprintf(buf, bufsize, "file '%s'", filename.c_str());
}



// abortWriter -- throw runtime_error for unrecoverable error
//   fmt        printf() format string for error message
//   ...        args, if any, for the format string
// The formatted error message can be retrieved from the exception by
// using exception::what().

void
OasisWriter::abortWriter (const char* fmt, ...)
{
    char  msgbuf[256];

    char  context[100];
    formatContext(context, sizeof context);
    Uint  n = SNprintf(msgbuf, sizeof msgbuf, "%s: ", context);

    va_list  ap;
    va_start(ap, fmt);
    if (n < sizeof(msgbuf) - 1)
        VSNprintf(msgbuf + n, sizeof(msgbuf) - n, fmt, ap);
    va_end(ap);

    ThrowRuntimeError("%s", msgbuf);
}


}  // namespace Oasis
