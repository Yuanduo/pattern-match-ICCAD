// oasis/scanner.h -- low-level interface for reading OASIS files
//
// last modified:   31-Dec-2009  Thu  17:17
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// The scanner deals only with the representation of data, not with
// their meanings.

#ifndef OASIS_SCANNER_H_INCLUDED
#define OASIS_SCANNER_H_INCLUDED

#include <inttypes.h>           // for uint32_t
#include <string>
#include <sys/types.h>          // for off_t

#include "port/compiler.h"
#include "misc/buffer.h"
#include "misc/utils.h"
#include "oasis.h"
#include "rectypes.h"


namespace Oasis {

using std::string;
using SoftJin::llong;
using SoftJin::Uchar;
using SoftJin::Uint;
using SoftJin::Ullong;
using SoftJin::Ulong;
using SoftJin::ReadBuffer;
using SoftJin::WarningHandler;

class ZlibInflater;             // from compressor.h
class Decompressor;             // from compressor.h



/*----------------------------------------------------------------------
OasisScanner -- tokenizer for OASIS file
This is the lowest-level (lexical) interface for reading OASIS files.
The next level up is OasisRecordReader.

Type members

    StringBuffer
        Provides a temporary holding area for the contents of
        OASIS strings.  Used only to avoid repeated new/delete in
        readString().

Data members

    fileBuf             ReadBuffer<char>
        Buffer for data read from file.  Its curr member is valid only
        when the scanner is reading from cblockBuf.  The scanner updates
        only `data' (see below) while it is reading.  It copies data to
        fileBuf.curr when refilling fileBuf or switching to cblockBuf.

    cblockBuf           ReadBuffer<char>
        Buffer to hold output of decompressor.  The scanner reads from
        this buffer when decompressor != Null.

    stringBuf           StringBuffer
        Temporary storage to hold contents of OASIS string being read.
        Used only in readString().

    data                char*
        Points to the next byte to be read, either from fileBuf or
        from cblockBuf.

    dataEnd             char*
        Points to one past the last valid byte in the current buffer.
        Invariant:  dataEnd == fileBuf.end  ||  dataEnd == cblockBuf.end

    fileOffset          off_t
        The file offset of the first byte in fileBuf.buffer.
        Invariant:  fdin's file pointer is at
                    fileOffset + (fileBuf.end - fileBuf.buffer)

    inflater            ZlibInflater*
        The owning pointer to a concrete strategy class for decompressing
        DEFLATE-compressed CBLOCKs.  The scanner instantiates this
        the first time it sees a DEFLATE-compressed CBLOCK.  If the spec
        adds other compression methods, we will need similar pointers
        to other concrete strategy instances.

    decompressor        Decompressor*
        Null if the scanner is reading from fileBuf.  Otherwise the
        scanner is reading from a CBLOCK and this points to the
        decompressor instance used to decompress the data of the
        current CBLOCK.  In the current version of the spec the only
        compression method defined is DEFLATE; so if this is non-Null
        it must have the same value as inflater.

    The following four members are defined only when the scanner is
    reading from a CBLOCK, i.e., when decompressor != Null.

    cblockStart         off_t
        The file offset where the CBLOCK's compressed data begins.

    cblockOffset        Ullong
        The CBLOCK equivalent of fileOffset -- the offset in the
        uncompressed block of the first byte of cblockBuf.buffer.

    compBytesLeft       Ullong
        Number of bytes of compressed data in the current CBLOCK
        we have not yet handed to the decompressor.

    uncompBytesLeft     Ullong
        Number of bytes of uncompressed data in the current CBLOCK
        we have yet to get from the decompressor.


    fdin                int
        File descriptor opened for reading from the input file.

    filename            string
        Pathname of input file.

    warnHandler         WarningHandler
        Callback passed to constructor.  May be Null.  If non-Null,
        the scanner will invoke it for any warning messages it wants
        to display.

------------------------------------------------------------------------*/

class OasisScanner {
    class StringBuffer {
        char*   buf;
        size_t  bufsize;
    public:
                StringBuffer();
        explicit  StringBuffer (size_t size);
                ~StringBuffer();
        char*   getBuffer();
        void    reserve (size_t size);
    };

    // Buffering
    ReadBuffer<char>  fileBuf;          // file data is read into this
    ReadBuffer<char>  cblockBuf;        // CBLOCKs are uncompressed into this
    StringBuffer      stringBuf;        // strings are read into this
    char*       data;                   // next byte will be read from here
    char*       dataEnd;                // one past last byte in current buffer

    // Current position
    off_t       fileOffset;             // file offset of fileBuf.buffer

    // CBLOCKs
    ZlibInflater*  inflater;            // a DEFLATE decompressor
    Decompressor*  decompressor;        // Null or current decompressor

    off_t       cblockStart;            // file offset of start of CBLOCK
    Ullong      cblockOffset;           // uncompressed offset of cblockBuf
    Ullong      uncompBytesLeft;        // num bytes decompressor yet to give
    Ullong      compBytesLeft;          // num bytes yet to give decompressor

    int         fdin;                   // file descriptor opened for reading
    string      filename;               // pathname of file being read
    WarningHandler  warnHandler;        // callback for warning messages

public:
    explicit    OasisScanner (const char* fname, WarningHandler);
                ~OasisScanner();
    const char* getFileName()  { return (filename.c_str()); }

    void        seekTo (off_t offset);
    off_t       getFileSize();
    bool        eof();
    void        validateFile (const Validation& val);
    void        decompressBlock (Ulong compType,
                                 Ullong uncomBytes, Ullong compBytes);
    int         peekByte();
    int         readByte();
    void        readBytes (/*out*/ void* addr, size_t nbytes);
    void        skipBytes (Ullong nbytes);

    void        verifyMagic();
    Ulong       readUnsignedInteger();
    long        readSignedInteger();
    Ullong      readUnsignedInteger64();
    llong       readSignedInteger64();
    uint32_t    readValidationSignature();
    RecordID    readRecordID (/*out*/ OFilePos* offset);
    void        readReal (/*out*/ Oreal* retval);
    void        readReal (Ulong realType, /*out*/ Oreal* retval);
    void        readString (/*out*/ string* pstr);

    long        readOneDelta();
    Delta       readTwoDelta();
    Delta       readThreeDelta();
    Delta       readGDelta();

private:
    bool        readingFromCblock() const;
    void        setBuffer (ReadBuffer<char>& buf);
    size_t      availData() const;
    bool        bufferIsEmpty() const;
    off_t       currFileOffset() const;
    void        fillFileBuffer();
    void        fillBuffer();
    void        fillCblockBuffer();
    void        getCurrBytePosition (/*out*/ OFilePos* bytePos) const;
    Uint        formatContext (/*out*/ char* buf, Uint bufsize);
    void        warn (const char* fmt, ...)  SJ_PRINTF_ARGS(2,3);
    void        abortScanner (const char* fmt, ...)
                                             SJ_PRINTF_ARGS(2,3) SJ_NORETURN;
private:
                OasisScanner (const OasisScanner&);     // forbidden
    void        operator= (const OasisScanner&);        // forbidden
};



inline void
OasisScanner::readReal (/*out*/ Oreal* retval)
{
    // 7.3.1
    // An OASIS real begins with an unsigned-integer that gives the
    // real's representation.  Read that and pass it to the other
    // readReal().

    readReal(readUnsignedInteger(), retval);
}



// readOneDelta -- read a 1-delta
// A 1-delta is a Manhattan displacement.  It is represented by a
// signed-integer with positive values for East and North, and negative
// values for West and South.  The context determines whether the delta
// is North-South or East-West.

inline long
OasisScanner::readOneDelta() {
    return (readSignedInteger());
}



// readTwoDelta -- read a 2-delta
// A 2-delta is a Manhattan displacement.  It is represented by an
// unsigned integer whose lowest 2 bits give the direction -- 00 East,
// 01 North, 10 West, and 11 South -- and remaining bits give the
// magnitude of the displacement.

inline Delta
OasisScanner::readTwoDelta()
{
    Ulong  val = readUnsignedInteger();
    Delta::Direction  dirn = static_cast<Delta::Direction>(val & 0x3);
    return Delta(dirn, val >> 2);
}



// readThreeDelta -- read a 3-delta
// A 3-delta is an octangular displacement.  It is represented by an
// unsigned integer whose lowest 3 bits give the direction of the
// displacement (see Delta::Direction) and remaining bits give the
// X and/or Y value of the displacement.

inline Delta
OasisScanner::readThreeDelta()
{
    Ulong  val = readUnsignedInteger();
    Delta::Direction  dirn = static_cast<Delta::Direction>(val & 0x7);
    return Delta(dirn, val >> 3);
}


} // namespace Oasis

#endif  // OASIS_SCANNER_H_INCLUDED
