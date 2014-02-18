// oasis/scanner.cc -- low-level interface for reading OASIS files
//
// last modified:   31-Dec-2009  Thu  17:17
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <algorithm>            // for min, reverse_copy
#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <memory>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <boost/static_assert.hpp>

#include "compressor.h"
#include "scanner.h"
#include "validator.h"


namespace Oasis {

using namespace std;
using namespace SoftJin;


//======================================================================
//                              StringBuffer
//======================================================================

// StringBuffer is a temporary holding area for strings.  All it provides
// is a buffer that is expanded on demand.
//
// Because std::string does not provide access to its contents buffer,
// the scanner cannot read an OASIS string directly into a std::string.
// Instead it reads the string characters into a StringBuffer and then
// constructs the std::string from the StringBuffer's contents.


// default constructor
// Creates a zero-sized buffer.

inline
OasisScanner::StringBuffer::StringBuffer() {
    buf = Null;
    bufsize = 0;
}


// constructor
//   size       initial size of buffer
// Creates a buffer of the specified size.

inline
OasisScanner::StringBuffer::StringBuffer (size_t size) {
    buf = new char[size];
    bufsize = size;
}


inline
OasisScanner::StringBuffer::~StringBuffer() {
    delete[] buf;
}


inline char*
OasisScanner::StringBuffer::getBuffer() {
    return buf;
}


// reserve -- ensure that the buffer is at least as big as the given size.

inline void
OasisScanner::StringBuffer::reserve (size_t size)
{
    if (size <= bufsize)
        return;

    char*  newbuf = new char[size];
    delete[] buf;
    buf = newbuf;
    bufsize = size;
}



//======================================================================
//                              OasisScanner
//======================================================================


const size_t  FileBufferSize = 256*1024;
    // Size of the main buffer that holds data read from the file.


const size_t  CblockBufferSize = 256*1024;
    // Size of buffer used to hold uncompressed cblock data.



OasisScanner::OasisScanner (const char* fname, WarningHandler warner)
  : fileBuf(FileBufferSize),
    stringBuf(512),                     // start with some reasonable size
    filename(fname),
    warnHandler(warner)
{
    fdin = OpenFile(fname, O_RDONLY);
    data = dataEnd = fileBuf.buffer;

    fileOffset = 0;
    inflater = Null;
    decompressor = Null;

    // decompressBlock() allocates cblockBuf's buffer when the first
    // cblock is seen.  The following members are defined only in a
    // cblock, i.e., when decompressor != Null:
    // cblockStart, cblockOffset, uncompBytesLeft, compBytesLeft
}



OasisScanner::~OasisScanner()
{
    close(fdin);
    delete inflater;
}


//----------------------------------------------------------------------
// Buffering

inline bool
OasisScanner::readingFromCblock() const {
    return (decompressor != Null);
}


// setBuffer -- switch to the given ReadBuffer (fileBuf or cblockBuf)

inline void
OasisScanner::setBuffer (ReadBuffer<char>& buf) {
    data = buf.curr;
    dataEnd = buf.end;
}


// availData -- return number of characters remaining in the current buffer

inline size_t
OasisScanner::availData() const {
    return (dataEnd - data);
}


// bufferIsEmpty -- true if the current buffer is empty (availData() == 0)

inline bool
OasisScanner::bufferIsEmpty() const {
    return (data == dataEnd);
}



// currFileOffset -- get file offset of the next byte to be read from fileBuf.
// Precondition: The scanner must not be reading from a cblock.

inline off_t
OasisScanner::currFileOffset() const
{
    assert (! readingFromCblock());

    // fileOffset is the offset of the first byte of fileBuf.  The next
    // byte to be read is *data and not *fileBuf.curr because curr is
    // updated only when refilling or switching buffers, not while a
    // buffer is being read.

    return (fileOffset + (data - fileBuf.buffer));
}



// fillFileBuffer -- read data from file into file buffer
//
// Precondition:   file buffer is empty
// Postcondition:  file buffer is non-empty

void
OasisScanner::fillFileBuffer()
{
    assert (fileBuf.empty());

    ssize_t  nr = read(fdin, fileBuf.buffer, FileBufferSize);
    if (nr < 0)
        abortScanner("read failed: %s", strerror(errno));
    if (nr == 0)
        abortScanner("unexpected EOF");

    // Update fileOffset.  Its new value is the old fileOffset plus
    // number of bytes formerly in the buffer, which was the nr value in
    // the previous call to this function.  Then tell fileBuf it has
    // been refilled with nr characters.

    fileOffset += (fileBuf.end - fileBuf.buffer);
    fileBuf.refill(nr);
}



// fillBuffer -- refill the current buffer
// If on entry the current buffer is cblockBuf and fillBuffer() discovers
// that the cblock is over, it switches back to fileBuf.
//
// Precondition:   the current buffer is empty
// Postcondition:  the current buffer is non-empty

void
OasisScanner::fillBuffer()
{
    assert (bufferIsEmpty());

    // Copy the data pointer back to the buffer.

    if (readingFromCblock())
        cblockBuf.curr = data;
    else
        fileBuf.curr = data;

    // If in a cblock, try to decompress more data into cblockBuf.  But
    // if we have reached the end of the cblock, return to reading from
    // fileBuf.

    if (readingFromCblock()) {
        fillCblockBuffer();
        if (readingFromCblock())        // cblock not yet over?
            setBuffer(cblockBuf);       // reset data and dataEnd
    }

    // If not in a cblock, refill fileBuf if it's empty.  fileBuf might
    // not be empty if we got here because a cblock just ended.

    if (! readingFromCblock()) {
        if (fileBuf.empty())
            fillFileBuffer();
        setBuffer(fileBuf);             // reset data and dataEnd
    }

    assert (! bufferIsEmpty());
}



// seekTo -- resume scanning at specified file offset
// The offset must not lie within a CBLOCK record.
// seekTo() aborts the scanner if the offset is not within the file.

void
OasisScanner::seekTo (off_t offset)
{
    // If the desired offset is in the range already in fileBuf, simply
    // adjust the data pointer.  Otherwise seek to the offset and discard
    // whatever is in the buffer.
    //
    // bufferEndOffset is the file offset of the byte past the last byte
    // in fileBuffer.  Note that the comparison with this uses <= and not <.
    // This is correct.  If offset == bufferEndOffset, fileBuf.curr gets
    // set to fileBuf.end.  The next call to readByte() or whatever will
    // trigger a call to fillBuffer().

    off_t  bufferEndOffset = fileOffset + (fileBuf.end - fileBuf.curr);
    if (offset >= fileOffset  &&  offset <= bufferEndOffset)
        fileBuf.curr = fileBuf.buffer + (offset - fileOffset);
    else {
        if (lseek(fdin, offset, SEEK_SET) != offset)
            abortScanner("lseek failed: %s", strerror(errno));
        fileOffset = offset;
        fileBuf.refill(0);
    }

    decompressor = Null;
    setBuffer(fileBuf);
}



// getFileSize -- get size of file being scanned

off_t
OasisScanner::getFileSize()
{
    struct stat  st;
    if (fstat(fdin, &st) < 0)
        abortScanner("fstat failed: %s", strerror(errno));
    return st.st_size;
}



// eof -- true if the scanner is at end of file
// Returns false if the scanner is in a cblock, even if no more data is
// available.

bool
OasisScanner::eof()
{
    return (!readingFromCblock()  &&  currFileOffset() == getFileSize());
}



// validateFile -- compute validation signature and check against input value.
//   val        validation scheme and signature stored in file
// Aborts the scanner if the computed signature does not match the input
// value.  Leaves the file pointer at the end of the file.

void
OasisScanner::validateFile (const Validation& val)
{
    // Nothing to do if the file has no validation.
    if (val.scheme == Validation::None)
        return;

    // Instantiate the appropriate strategy class for the validation
    // scheme.  We delegate computing the signature to this class.

    auto_ptr<Validator>  validator;
    if (val.scheme == Validation::CRC32)
        validator.reset(new Crc32Validator);
    else
        validator.reset(new Checksum32Validator);

    // Compute the validation signature.
    // 14.4.2 and 14.5.1:  The checksum/CRC includes "all of the bytes
    // in the OASIS file from the first byte of <magic-bytes> to the
    // END record's validation-scheme integer."
    //
    // bytesLeft is the number of bytes remaining in the checksum/CRC
    // computation.  The `- 4' excludes the 4-byte validation signature
    // from the computation.

    seekTo(0);
    off_t  bytesLeft = getFileSize() - 4;
    while (bytesLeft > 0) {
        if (fileBuf.empty())
            fillFileBuffer();
        Uint  nbytes = min<off_t>(fileBuf.availData(), bytesLeft);
        validator->add(fileBuf.curr, nbytes);
        fileBuf.curr += nbytes;
        bytesLeft -= nbytes;
    }
    setBuffer(fileBuf);

    // Compare the computed signature with the input value and
    // abort on mismatch.

    uint32_t  sig = validator->getSignature();
    if (sig != val.signature) {
        abortScanner("file is corrupted: %s is 0x%08x; should be 0x%08x",
                     val.schemeName(), sig, val.signature);
    }
}


//----------------------------------------------------------------------
// CBLOCKs


// decompressBlock -- decompress a cblock
//   compType           type of compression
//   uncompBytes        size of uncompressed data in cblock
//   compBytes          size of cblock (compressed)

void
OasisScanner::decompressBlock (Ulong compType,
                               Ullong uncompBytes, Ullong compBytes)
{
    // 35.4
    // Nested CBLOCK records may not be stored within a compressed record.

    if (readingFromCblock())
        abortScanner("CBLOCK not allowed inside another CBLOCK");

    // If there is nothing in the expanded cblock, simply skip
    // whatever overhead the compressor has added.

    if (uncompBytes == 0) {
        skipBytes(compBytes);
        return;
    }

    // The compressed size cannot be zero if the uncompressed size isn't.
    if (compBytes == 0)
        abortScanner("CBLOCK gets %llu bytes from nothing", uncompBytes);

    // If this is the first cblock in the file, allocate the buffer for
    // the uncompressed data.

    if (cblockBuf.buffer == Null)
        cblockBuf.create(CblockBufferSize);

    // Initialize the class members concerned with CBLOCKs.  cblockStart
    // and cblockOffset are used for the file position.

    cblockStart = currFileOffset();
    cblockOffset = 0;
    compBytesLeft = compBytes;
    uncompBytesLeft = uncompBytes;

    // Save the cursor position.  The buffer's curr pointer is not
    // updated while the buffer is being used; only 'data' is.  We must
    // record the current position because we will be switching to
    // cblockBuf.

    fileBuf.curr = data;

    // Make the context string to pass to Decompressor::beginBlock().
    // This contains the filename and current offset; it will appear in
    // the error message if any Decompressor method throws an exception.
    // We must do this before entering the cblock (i.e., setting
    // decompressor) because that would change the offset part of the
    // context.

    char  context[100];
    formatContext(context, sizeof context);

    // Set decompressor to an instance of the appropriate decompression
    // strategy class.  In the current spec DEFLATE is the only cblock
    // type, but that may not be true in the future.  If this is the
    // first DEFLATE cblock in the file, create the inflater.

    if (compType != DeflateCompression)
        abortScanner("invalid compression type %lu", compType);
    if (inflater == Null)
        inflater = new ZlibInflater();
    decompressor = inflater;

    // Initialize the decompressor for the cblock, reset cblockBuf to
    // the empty state, and then switch to it.  cblockBuf will get
    // filled when the next byte is read.

    decompressor->beginBlock(context);
    cblockBuf.refill(0);
    setBuffer(cblockBuf);
}



// fillCblockBuffer -- uncompress more data from the cblock into cblockBuf.
// Precondition:
//      cblockBuf.empty()
// Postcondition:
//      readingFromCblock() => !cblockBuf.empty()

void
OasisScanner::fillCblockBuffer()
{
    assert (cblockBuf.empty());

    // If there is no more decompressed data, set decompressor to Null
    // to indicate that we have exited the cblock.  The caller should
    // switch from cblockBuf back to fileBuf.
    //
    // uncompBytesLeft may be zero even when compBytesLeft is non-zero.
    // That is, the cblock may be completely decompressed even though we
    // have not given the decompressor all the compressed data.  This is
    // because the last byte of zlib's deflate output may be redundant.
    // If this happens, skip the unused byte(s).  Note that we cannot
    // use currFileOffset() to get the current offset.  That uses
    // 'data', which is updated only after we return to fillBuffer().

    if (uncompBytesLeft == 0) {
        decompressor = Null;
        if (compBytesLeft != 0) {
            off_t  currOffset = fileOffset + (fileBuf.curr - fileBuf.buffer);
            seekTo(currOffset + compBytesLeft);
        }
        return;
    }

    // cblockOffset is the offset of the first byte of cblockBuf.buffer
    // in the decompressed output.  Since we have just consumed the data
    // that is now in cblockBuf, we need to increment cblockOffset by
    // the number of data bytes in it.

    cblockOffset += cblockBuf.end - cblockBuf.buffer;

    // Get more decompressed data from the decompressor.  But first give
    // it more data if it has used up all the input passed earlier.  Keep
    // doing this until it produces some output or the block is done.
    //
    // availBytes       num bytes available in fileBuf
    // inbytes          num bytes to give to decompressor
    // outbytes         num bytes output by decompressor
    //
    // inbytes will be non-zero (this is a precondition of giveInput())
    // because fillFileBuffer() leaves the file buffer non-empty.  The
    // loop will terminate because each call to getOutput() consumes
    // input or produces output.

    Uint  outbytes;
    do {
        if (compBytesLeft != 0  &&  decompressor->needInput()) {
            if (fileBuf.empty())
                fillFileBuffer();
            Ullong  availBytes = fileBuf.availData();
            Ullong  inbytes = min(compBytesLeft, availBytes);
            decompressor->giveInput(fileBuf.curr, inbytes);
            fileBuf.curr += inbytes;
            compBytesLeft -= inbytes;
        }
        outbytes = decompressor->getOutput(cblockBuf.buffer, CblockBufferSize);
    } while (outbytes == 0  &&  compBytesLeft != 0);

    // outbytes must be in the range 1..uncompBytesLeft.  If it is 0,
    // compBytesLeft must also be 0.  So either comp-byte-count is too
    // small or uncomp-byte-count is too large.

    if (outbytes == 0)
        abortScanner("CBLOCK uncomp-byte-count too large");
    if (outbytes > uncompBytesLeft)
        abortScanner("CBLOCK uncomp-byte-count too small");

    cblockBuf.refill(outbytes);
    uncompBytesLeft -= outbytes;
}


//----------------------------------------------------------------------


// The functions here form the interface between the buffer-manipulating
// functions above and the token-assembling functions below.


// peekByte -- look at next byte in input without consuming it

int
OasisScanner::peekByte()
{
    if (bufferIsEmpty())
        fillBuffer();
    return (static_cast<Uchar>(*data));
}



// readByte -- get next byte in input

int
OasisScanner::readByte()
{
    if (bufferIsEmpty())
        fillBuffer();
    return (static_cast<Uchar>(*data++));
}



// readBytes -- read nbytes bytes into buffer beginning at addr

void
OasisScanner::readBytes (/*out*/ void* addr, size_t nbytes)
{
    while (nbytes > 0) {
        if (bufferIsEmpty())
            fillBuffer();
        size_t  n = min(nbytes, availData());
        memcpy(addr, data, n);
        addr = static_cast<char*>(addr) + n;
        data += n;
        nbytes -= n;
    }
}



// skipBytes -- skip the next nbytes bytes in the input

void
OasisScanner::skipBytes (Ullong nbytes)
{
    while (nbytes > 0) {
        if (bufferIsEmpty())
            fillBuffer();
        size_t  n = min<Ullong>(nbytes, availData());
        data += n;
        nbytes -= n;
    }
}



// getCurrBytePosition -- get position of next byte to be read.
//
// The position returned might be incorrect if the current buffer is
// empty.  That is because accessing the next byte might change the
// state from in-cblock to not-in-cblock.  This doesn't matter if the
// position is only going to be printed in an error message, but
// otherwise the caller should ensure that the buffer is non-empty.
//
// getCurrBytePosition() cannot call fillBuffer() itself because it is
// called by abortScanner().  It must not do anything that might cause
// another abort.

/*private*/ void
OasisScanner::getCurrBytePosition (/*out*/ OFilePos* pos) const
{
    if (readingFromCblock()) {
        llong  currCblockOffset = cblockOffset + (data - cblockBuf.buffer);
        pos->init(cblockStart, currCblockOffset);
    } else {
        pos->init(currFileOffset());
    }
}


//----------------------------------------------------------------------


// verifyMagic -- verify that an OASIS file begins with the magic bytes.
// Precondition:
//   fileOffset == 0  &&  !readingFromCblock()
// Postcondition:
//   fileOffset == StartRecordOffset  &&  !readingFromCblock()

void
OasisScanner::verifyMagic()
{
    assert (fileOffset == 0  &&  !readingFromCblock());

    char  magic[OasisMagicLength];
    readBytes(magic, OasisMagicLength);
    if (memcmp(magic, OasisMagic, OasisMagicLength) != 0)
        abortScanner("this is not an OASIS file");
}



// readUnsignedInteger -- read an unsigned-integer
// Spec paragraph 7.2.1

Ulong
OasisScanner::readUnsignedInteger()
{
    const Uint  MaxShift = numeric_limits<Ulong>::digits - 1;
        // Shifting by more than this is undefined.  E.g., on a 32-bit
        // system we can shift by at most 31 bits.

    const Uint  SafeShiftLimit = numeric_limits<Ulong>::digits - 6;
        // We can shift a 7-bit value by up to SafeShiftLimit-1 bits
        // without worrying about overflow.

    // Integers in OASIS are stored in a variable number of bytes.  The
    // least-significant byte appears first.  Each byte's leftmost
    // bit is the continuation bit: 0 means it's the last byte and 1
    // means more bytes follow.  The remaining 7 bits contain the data.
    //
    // Here is the algorithm.  Repeatedly read a byte, mask out the high
    // bit, and OR the remaining bits into the value after shifting to
    // the correct position, stopping when the high bit is 0.  Store the
    // byte in an int8_t variable so that its high bit can be tested
    // using sign propagation.
    //
    // We use two loops instead of one to assemble the integer.  There
    // are two reasons for this.  First, for shift values up to
    // SafeShiftLimit-1 there is no danger of overflow while shifting.
    // Second, by checking before the loop the number of characters
    // available in the buffer we can avoid checking within the loop
    // whether the buffer is empty.
    //
    // So in the first loop we omit both the overflow test and the
    // empty-buffer test.  Most of the time the second loop will not
    // get executed.

    Ulong  val = 0;    // current value of unsigned integer
    Uint  shift = 0;
    Uint  safeShift = min<Uint>(SafeShiftLimit, 7*availData());

    for ( ;  shift < safeShift;  shift += 7) {
        int8_t  byte = *data++;
        val |= static_cast<Ulong>(byte & 0x7f) << shift;
        if (byte >= 0)
            return val;
    }

    for ( ;  ;  shift += 7) {
        int8_t  byte = static_cast<int8_t>(readByte());
        Ulong  bits = byte & 0x7f;

        // Verify that shifting the bits does not make result overflow.
        // The test for 0 prevents needless aborts when the number has
        // trailing zero bytes, e.g., the sequence of bytes (in hex)
        //     85 80 80 80 80 80 80 80 00
        // is a valid (non-overflowing) representation of 5.

        if (bits != 0) {
            if (shift > MaxShift  ||  bits != ((bits << shift) >> shift))
                abortScanner("unsigned-integer is too large");  // 7.2.3
            val |= bits << shift;
        }
        if (byte >= 0)
            return val;
    }
}



// readSignedInteger -- read a signed-integer
// Spec paragraph 7.2.2

long
OasisScanner::readSignedInteger()
{
    const Uint  MaxShift       = numeric_limits<Ulong>::digits - 1;
    const Uint  SafeShiftLimit = numeric_limits<Ulong>::digits - 7;
        // SafeShiftLimit is one less than in readUnsignedInteger()
        // because we also want to prevent overflow into the sign bit.

    // A signed-integer is like an unsigned-integer (see above) except
    // that the first byte contains only 6 bits of data.  The leftmost
    // bit of each byte is set iff more bytes follow.  The rightmost bit
    // of the first byte is the sign bit: 0 for positive and 1 for
    // negative numbers.  The first byte thus has 6 data bits and the
    // remaining bytes have 7 data bits.

    int8_t  byte = static_cast<int8_t>(readByte());
    bool  isNeg = (byte & 0x1);
    long  val = (byte & 0x7f) >> 1;
    Uint  shift, safeShift;

    if (byte >= 0)
        goto EXIT;

    shift = 6;
    safeShift = min<Uint>(SafeShiftLimit, 7*availData());
        // The limit set by the number of available chars is actually
        // 7*availData() + 6, but adding the 6 is not worthwhile.

    for ( ;  shift < safeShift;  shift += 7) {
        byte = *data++;
        val |= static_cast<long>(byte & 0x7f) << shift;
        if (byte >= 0)
            goto EXIT;
    }

    for ( ;  ;  shift += 7) {
        byte = static_cast<int8_t>(readByte());
        Ulong  bits = byte & 0x7f;
        if (bits != 0) {
            if (shift > MaxShift  ||  bits != ((bits << shift) >> shift))
                abortScanner("signed-integer is too large");    // 7.2.3
            val |= bits << shift;

            // Check for overflow into the sign bit.  To simplify code
            // in the upper layers, don't allow the most negative number
            // (-2^31 or -2^63).  If we allow it, we have to worry about
            // overflow even when negating a number.

            if (val < 0)
                abortScanner("signed-integer is too large");
        }
        if (byte >= 0)
            goto EXIT;
    }

  EXIT:
    return (isNeg ? -val : val);
}


//----------------------------------------------------------------------
// Read unsigned and signed integers with at least 64 bits.
//
// readUnsignedInteger64() and readSignedInteger64() are the same as
// readUnsignedInteger() and readSignedInteger(), but with Ullong and
// llong instead of Ulong and long.
//
// We call readUnsignedInteger64() in OasisRecordReader to read
// unsigned-integers that might be file offsets: table offsets in the
// START or END record and unsigned-integer values of properties.  We do
// not use readUnsignedInteger() for these because we want to be able to
// handle files larger than 4 GB even when Ulong has only 32 bits.
//
// We call readSignedInteger64() to read signed-integer values of
// properties.  We use llong instead of long to store these, but that is
// only for symmetry with unsigned-integer values.  None of the code in
// this directory requires signed-integer property values to have 64 bits.


Ullong
OasisScanner::readUnsignedInteger64()
{
    // We do not need a separate function if Ullong has the same size as
    // Ulong.  Allow the compiler to eliminate the body of this
    // function, which becomes dead code, and generate just a jump to
    // readUnsignedInteger().

    if (sizeof(Ullong) == sizeof(Ulong))
        return (readUnsignedInteger());

    const Uint  MaxShift = numeric_limits<Ullong>::digits - 1;
    const Uint  SafeShiftLimit = numeric_limits<Ullong>::digits - 6;

    Ullong  val = 0;    // current value of unsigned integer
    Uint  shift = 0;
    Uint  safeShift = min<Uint>(SafeShiftLimit, 7*availData());

    for ( ;  shift < safeShift;  shift += 7) {
        int8_t  byte = *data++;
        val |= static_cast<Ullong>(byte & 0x7f) << shift;
        if (byte >= 0)
            return val;
    }

    for ( ;  ;  shift += 7) {
        int8_t  byte = static_cast<int8_t>(readByte());
        Ullong  bits = byte & 0x7f;
        if (bits != 0) {
            if (shift > MaxShift  ||  bits != ((bits << shift) >> shift))
                abortScanner("unsigned-integer is too large");  // 7.2.3
            val |= bits << shift;
        }
        if (byte >= 0)
            return val;
    }
}



llong
OasisScanner::readSignedInteger64()
{
    if (sizeof(llong) == sizeof(long))
        return (readSignedInteger());

    const Uint  MaxShift       = numeric_limits<Ullong>::digits - 1;
    const Uint  SafeShiftLimit = numeric_limits<Ullong>::digits - 7;

    int8_t  byte = static_cast<int8_t>(readByte());
    bool  isNeg = (byte & 0x1);
    llong  val = (byte & 0x7f) >> 1;
    Uint  shift, safeShift;

    if (byte >= 0)
        goto EXIT;

    shift = 6;
    safeShift = min<Uint>(SafeShiftLimit, 7*availData());

    for ( ;  shift < safeShift;  shift += 7) {
        byte = *data++;
        val |= static_cast<llong>(byte & 0x7f) << shift;
        if (byte >= 0)
            goto EXIT;
    }

    for ( ;  ;  shift += 7) {
        byte = static_cast<int8_t>(readByte());
        Ullong  bits = byte & 0x7f;
        if (bits != 0) {
            if (shift > MaxShift  ||  bits != ((bits << shift) >> shift))
                abortScanner("signed-integer is too large");    // 7.2.3
            val |= bits << shift;
            if (val < 0)
                abortScanner("signed-integer is too large");
        }
        if (byte >= 0)
            goto EXIT;
    }

  EXIT:
    return (isNeg ? -val : val);
}


//----------------------------------------------------------------------


// readRecordID -- read a record-ID and return it and its file position

RecordID
OasisScanner::readRecordID (/*out*/ OFilePos* recordPos)
{
    // peekByte() ensures that the buffer is non-empty.  See the comment
    // before getCurrBytePosition() for why that is important.

    (void) peekByte();
    getCurrBytePosition(recordPos);

    // Although a record-ID must be in the range 0..34, we cannot use
    // readByte() to read it because it is specified to be an
    // unsigned-integer.  These can be arbitrarily long.

    Ulong  id = readUnsignedInteger();
    if (! RecordTypeIsValid(id))
        abortScanner("invalid record type %lu", id);
    return (static_cast<RecordID>(id));
}



// readValidationSignature -- read validation signature at end of file
// Spec paragraphs 14.4.2 and 14.5.1

uint32_t
OasisScanner::readValidationSignature()
{
    // The validation signature is a 32-bit unsigned integer stored in
    // little-endian order.

    Uchar  buf[4];
    readBytes(buf, 4);
    return ((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
}



// readReal -- read a real number of the given representation type.
//   realType   type of real.  readReal() aborts the scanner if the
//              type is not one of the enumerators in class Oreal.
//   retval     out: the real number is stored here.

// Spec section 7.3

void
OasisScanner::readReal (Ulong realType, /*out*/ Oreal* retval)
{
    // For the rational subtypes (integers, reciprocals, ratios), OASIS
    // uses unsigned-integers for the numbers, and distinguishes between
    // positive and negative numbers using the type of real.  For
    // simplicity we represent all 6 types internally as rationals, and
    // use (signed) longs for the numerator and denominator.  The
    // numerator's sign gives the real number's sign.
    //
    // Because we use signed integers, we cannot represent integers in
    // the upper half of the the unsigned range, e.g., 2^31 and above on
    // a 32-bit system.  What do we do if we encounter such a number
    // in one of the 6 rational formats?
    //
    // We could follow the same procedure as for integers and just
    // abort.  But that seems too drastic because the value itself is
    // representable.  What we do instead is convert the number into a
    // type-7 real, i.e., double-precision floating-point, and emit the
    // following warning.

    const char  ConversionWarning[] =
        "integer %lu in type-%lu real too large to handle; "
        "converting real to double format (type 7)";

    const Ulong MaxLong = LONG_MAX;

    long  sign = -1;
        // Multiplier for the rational subtypes.  Used to unify handling
        // of positive and negative cases.  Set to +1 for the positive
        // cases.

    switch (realType) {
        // Types 0 and 1:  Positive and negative integers
        case Oreal::PosInteger:
            sign = 1;
            // fall through
        case Oreal::NegInteger: {
            Ulong  val = readUnsignedInteger();
            if (val <= LONG_MAX)
                *retval = sign * static_cast<long>(val);
            else {
                warn(ConversionWarning, val, realType);
                *retval = sign * static_cast<double>(val);
            }
            break;
        }

        // Types 2 and 3:  Positive and negative reciprocals of integers

        case Oreal::PosReciprocal:
            sign = 1;
            // fall through
        case Oreal::NegReciprocal: {
            Ulong  val = readUnsignedInteger();
            if (val == 0)                                       // 7.3.3
                abortScanner("invalid real: reciprocal of 0");
            if (val <= LONG_MAX)
                retval->assign(sign, static_cast<long>(val));
            else {
                warn(ConversionWarning, val, realType);
                *retval = sign / static_cast<double>(val);
            }
            break;
        }

        // Types 4 and 5:  Positive and negative ratios of integers

        case Oreal::PosRatio:
            sign = 1;
            // fall through
        case Oreal::NegRatio: {
            Ulong  numer = readUnsignedInteger();
            Ulong  denom = readUnsignedInteger();
            if (denom == 0)                                     // 7.3.3
                abortScanner("invalid real: denominator of ratio is 0");

            if (numer <= MaxLong  &&  denom <= MaxLong)
                retval->assign(sign * static_cast<long>(numer),
                               static_cast<long>(denom) );
            else {
                if (numer > MaxLong)
                    warn(ConversionWarning, numer, realType);
                else if (denom > MaxLong)
                    warn(ConversionWarning, denom, realType);

                double  dnumer = static_cast<double>(numer);
                double  ddenom = static_cast<double>(denom);
                *retval = sign * dnumer / ddenom;
            }
            break;
        }

        // Type 6:  IEEE 754 single-precision float
        // We assume that the C/C++ type `float' has the same
        // representation apart from endianness.  So we simply copy the
        // bytes into the variable, after reversing if the processor is
        // big-endian.  OASIS stores the bytes in little-endian order.

        case Oreal::Float32: {
            BOOST_STATIC_ASSERT (sizeof(float) == 4);
            float  fval;
            if (ProcessorIsLittleEndian())
                readBytes(&fval, 4);
            else {
                char  cval[4];
                readBytes(cval, 4);
                reverse_copy(cval, cval+4, reinterpret_cast<char*>(&fval));
            }
            *retval = fval;
            break;
        }

        // Type 7:  IEEE 754 double-precision float
        // As with type 6, we assume that we can just copy the bytes into
        // a variable of type double.

        case Oreal::Float64: {
            BOOST_STATIC_ASSERT (sizeof(double) == 8);
            double  dval;
            if (ProcessorIsLittleEndian())
                readBytes(&dval, 8);
            else {
                char  cval[8];
                readBytes(cval, 8);
                reverse_copy(cval, cval+8, reinterpret_cast<char*>(&dval));
            }
            *retval = dval;
            break;
        }

        default:
            abortScanner("invalid format byte %lu for real number", realType);
    }
}



// readString -- read OASIS string into argument
// Spec section 7.4

void
OasisScanner::readString (/*out*/ string* pstr)
{
    // A string is a sequence of zero or more bytes preceded by an
    // unsigned-integer representing the number of characters in the string.

    Ulong   len = readUnsignedInteger();
    if (len > pstr->max_size())
        abortScanner("string length %lu too large", len);

    // We cannot read the data directly into pstr because we don't have
    // access to the string class's data buffer.  So read the data
    // into a temporary buffer and then assign it to pstr.

    stringBuf.reserve(len);
    readBytes(stringBuf.getBuffer(), len);
    pstr->assign(stringBuf.getBuffer(), len);
}



// readGDelta -- read a g-delta
// Spec paragraph 7.5.5

Delta
OasisScanner::readGDelta()
{
    // If bit 0 of the first word is 0, the remaining bits form a
    // 3-delta.  That is, bits 3..1 encode the octangular direction.

    Ulong  val = readUnsignedInteger();
    if ((val & 0x1) == 0) {
        Delta::Direction  dirn =
                static_cast<Delta::Direction>((val >> 1) & 0x7);
        return Delta(dirn, val >> 4);
    }

    // Otherwise the remaining bits of the first word form a
    // signed-integer giving the x displacement.  The second word is a
    // signed-integer giving the y displacement.  The spec phrases it
    // differently but it comes to the same thing.

    bool  isNeg = (val & 0x2);
    long  xdisp = (isNeg ? -(val >> 2) : val >> 2);
    long  ydisp = readSignedInteger();
    return Delta(xdisp, ydisp);
}


//----------------------------------------------------------------------


// formatContext -- format a string giving input filename and position in it.
//   buf        the buffer in which the string is to be formatted
//   bufsize    space available in buf
// This context string is used for error messages from the scanner
// and compressor.  It has one of these forms:
//   file 'foo', offset NNN
//   file 'foo', uncompressed offset NNN in CBLOCK at offset MMM
// Returns the number of characters written to the buffer, excluding the
// terminating NUL.

Uint
OasisScanner::formatContext (/*out*/ char* buf, Uint bufsize)
{
    char        posbuf[256];
    OFilePos    pos;

    getCurrBytePosition(&pos);
    pos.print(posbuf, sizeof posbuf);
    Uint  n = SNprintf(buf, bufsize, "file '%s', %s: ",
                       filename.c_str(), posbuf);

    // If buf is not large enough, SNprintf() returns the number that
    // would have been written if there had been enough space.  We want
    // the actual number written.

    return (min(n, bufsize-1));
}



// abortScanner -- throw runtime_error for unrecoverable error
//   fmt        printf() format string for error message
//   ...        args, if any, for the format string

// The formatted error message can be retrieved from the exception by
// using exception::what().

void
OasisScanner::abortScanner (const char* fmt, ...)
{
    char  msgbuf[256];

    va_list  ap;
    va_start(ap, fmt);
    Uint  n = formatContext(msgbuf, sizeof msgbuf);
    if (n < sizeof(msgbuf) - 1)
        VSNprintf(msgbuf + n, sizeof(msgbuf) - n, fmt, ap);
    va_end(ap);

    ThrowRuntimeError("%s", msgbuf);
}



// warn -- warn about invalid record contents
//   rec        the invalid record
//   fmt        printf() format string for warning message
//   ...        arguments (if any) for the format string
// The formatted error message can be retrieved from the exception by
// using exception::what().

void
OasisScanner::warn (const char* fmt, ...)
{
    char  msgbuf[256];

    va_list  ap;
    va_start(ap, fmt);
    Uint  n = formatContext(msgbuf, sizeof msgbuf);
    if (n < sizeof(msgbuf) - 1)
        n += SNprintf(msgbuf + n, sizeof(msgbuf) - n, "warning: ");
    if (n < sizeof(msgbuf) - 1)
        VSNprintf(msgbuf + n, sizeof(msgbuf) - n, fmt, ap);
    va_end(ap);

    if (warnHandler != Null)
        warnHandler(msgbuf);
}


}  // namespace Oasis
