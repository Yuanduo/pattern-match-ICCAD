// gdsii/scanner.h -- low-level interface for reading GDSII Stream files
//
// last modified:   04-Jan-2010  Mon  11:05
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef GDSII_SCAN_H_INCLUDED
#define GDSII_SCAN_H_INCLUDED

#include <cassert>
#include <inttypes.h>           // for int16_t, int32_t
#include <string>
#include <sys/types.h>          // for off_t

#include "port/compiler.h"
#include "misc/buffer.h"
#include "misc/gzfile.h"
#include "double.h"
#include "rectypes.h"


namespace Gdsii {

using SoftJin::Uchar;
using SoftJin::Uint;
using SoftJin::FileHandle;
using SoftJin::ReadBuffer;


class GdsRecord;
    // Contents of one GDSII record; defined below.



// GdsScanner -- read records from a GDSII Stream file.
// This class hides
//   - how the file is read
//   - the format of the record header
//
// This is how you use it:
//
//      GdsScanner  scanner("foo.gds");
//      GdsRecord   rec;
//      do {
//          scanner.getNextRecord(&rec);
//          // get data from record using GdsRecord methods and process it
//      } while (rec.getRecordType() != GRT_ENDLIB);
//
// If the input file is gzipped, the offset stored in the member
// 'fileOffset' or returned by currByteOffset() is the offset in the
// uncompressed file.  It is not the actual file offset, which is
// undefined.
//
// Any of the methods may throw runtime_error.  The constructor may also
// throw bad_alloc.
//
// Class invariant:
//     !rbuf.empty() => fileOffset is the file offset of rbuf.buffer

class GdsScanner {
    enum {
        BufferSize = 128*1024
            // Size of scan buffer.  Must be >= GLimits::MaxRecordLength.
    };

    ReadBuffer<Uchar>  rbuf;    // input buffer
    off_t       fileOffset;     // offset in file of rbuf.buffer
    FileHandle  fh;             // for reading from input file
    std::string filename;       // pathname of input file

public:
                GdsScanner (const char* fname, FileHandle::FileType ftype);
                ~GdsScanner();

    void        getNextRecord (/*out*/ GdsRecord* rec);
    void        seekTo (off_t offset);
    off_t       currByteOffset() const;

private:
    void        fillBuffer();
    void        initRecord (/*out*/ GdsRecord* rec, Uint recType,
                            Uint length, Uchar* body, off_t fileOffset);
    void        abortScanner (const char* fmt, ...)
                                         SJ_PRINTF_ARGS(2,3)  SJ_NORETURN;
                GdsScanner (const GdsScanner&);         // forbidden
    void        operator= (const GdsScanner&);          // forbidden
};



// currByteOffset -- get the file offset of the next byte to be read.
// You can call this after reading the ENDLIB record to get the
// effective size of the input file.

inline off_t
GdsScanner::currByteOffset() const {
    return (fileOffset + (rbuf.curr - rbuf.buffer));
}



// GdsRecord -- record extracted from GDSII Stream file.
// This class (mostly) hides the data representation of items within a
// record.  It presents a record as a sequence of items.  For speed it
// reveals the representation of strings and just gives a pointer to the
// data, letting the caller deal with the fact that the string may not
// be NUL-terminated.
//
// GdsScanner::getNextRecord() returns this in an output parameter.  Use
// the nextXxx() methods of this class to extract the data items from
// the record.  The data is available only until the next call to
// getNextRecord().

class GdsRecord {
    friend class GdsScanner;

    const GdsRecordTypeInfo*  typeInfo; // info about this record's type
    int         length;         // length of record body (excludes header)
    Uchar*      body;           // pointer to body of record
    Uchar*      bp;             // start of next data item to return
    off_t       fileOffset;     // file offset of record's first header byte

public:
    const GdsRecordTypeInfo*  getTypeInfo() const;
    GdsRecordType  getRecordType() const;
    const char*    getRecordName() const;
    GdsDataType getDataType() const;
    Uint        getLength() const;
    off_t       getOffset() const;
    Uint        numItems() const;

    // Methods to retrieve the next item of the appropriate type from
    // the record.  It is up to you to call the right one and call it
    // the right number of times.  Use getRecordType() or getDataType()
    // to find which method to call; use numItems() to find how many
    // times to call it.

    Uint        nextBitArray();
    int         nextShort();
    int         nextInt();
    double      nextDouble();
    const char* nextString();

    void        reset();

private:
    int         bytesAvail() const;
};



inline const GdsRecordTypeInfo*
GdsRecord::getTypeInfo() const {
    return typeInfo;
}

inline GdsRecordType
GdsRecord::getRecordType() const {
    return typeInfo->getRecordType();
}


// getRecordName -- get the name of this record's type
// This is mainly for error messages.

inline const char*
GdsRecord::getRecordName() const {
    return typeInfo->name;
}

inline GdsDataType
GdsRecord::getDataType() const {
    return typeInfo->getDataType();
}


// getLength -- return length of record body
// This is the length of just the record body; it does not include the
// header.

inline Uint
GdsRecord::getLength() const {
    return length;
}


// getOffset -- return offset of record in file
// You can pass this value to GdsScanner::seekTo() to restart scanning
// the file at this record.

inline off_t
GdsRecord::getOffset() const {
    return fileOffset;
}


// numItems -- return the number of data items in the record.
// Precondition: the record's data type must not be GDATA_NONE and the
// record must not contain a variable-length string.
// We have enough information to handle variable-length fields but it is
// faster to leave it to the caller.

inline Uint
GdsRecord::numItems() const {
    assert (typeInfo->itemSize != 0);
    return (length / typeInfo->itemSize);
}


// bytesAvail -- how many bytes in the record body have not yet been read

/*private*/ inline int
GdsRecord::bytesAvail() const {
    return (length - (bp - body));
}



inline Uint
GdsRecord::nextBitArray()
{
    assert (getDataType() == GDATA_BITARRAY  &&  bytesAvail() == 2);
    Uint  val = (bp[0] << 8 | bp[1]);

    // There is no need to increment the pointer since BitArray records
    // have only one value.
    return val;
}



inline int
GdsRecord::nextShort()
{
    assert (getDataType() == GDATA_SHORT  &&  bytesAvail() >= 2);

    // val must be exactly 16 bits wide to ensure that the sign bit
    // is propagated.

    int16_t  val = (bp[0] << 8 | bp[1]);
    bp += 2;
    return val;
}



inline int
GdsRecord::nextInt()
{
    assert (getDataType() == GDATA_INT  &&  bytesAvail() >= 4);

    // val must be exactly 32 bits wide to ensure that the sign bit
    // is propagated if sizeof(int) ever becomes > 4.

    int32_t  val = (bp[0] << 24 | bp[1] << 16 | bp[2] << 8 | bp[3]);
    bp += 4;
    return val;
}



inline double
GdsRecord::nextDouble()
{
     assert (getDataType() == GDATA_DOUBLE  &&  bytesAvail() >= 8);
     double  val = GdsRealToDouble(bp);
     bp += 8;
     return val;
}



// nextString -- return pointer to the start of the next string.

inline const char*
GdsRecord::nextString()
{
    assert (getDataType() == GDATA_STRING);
    char*  val = reinterpret_cast<char*>(bp);

    // If itemSize == 0 then the record contains a single variable-length
    // string.  It is the caller's responsibility to ensure that this
    // method is called at most once for such records, so we don't have
    // to worry about that case.

    bp += typeInfo->itemSize;
    return val;
}



// reset -- restart reading data items from beginning of record

inline void
GdsRecord::reset() {
    bp = body;
}


} // namespace Gdsii

#endif  // GDSII_SCAN_H_INCLUDED
