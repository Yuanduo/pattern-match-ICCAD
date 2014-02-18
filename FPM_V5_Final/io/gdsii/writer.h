// gdsii/writer.h -- low-level interface for writing GDSII Stream files
//
// last modified:   27-Nov-2004  Sat  19:00
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef  GDSII_WRITER_H_INCLUDED
#define  GDSII_WRITER_H_INCLUDED

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>

#include "misc/buffer.h"
#include "misc/gzfile.h"
#include "double.h"
#include "glimits.h"
#include "rectypes.h"


namespace Gdsii {

using SoftJin::Cnul;
using SoftJin::Uchar;
using SoftJin::Uint;
using SoftJin::FileHandle;
using SoftJin::WriteBuffer;

using GLimits::RecordHeaderLength;
using GLimits::MaxRecordLength;



// GdsWriter -- low-level interface for writing GDSII Stream files.
// This is how you use it:
//
//    GdsWriter  writer("file.gds");
//    writer.beginFile();
//    writer.beginRecord(rti);
//    writer.writeShort(42);    // or writeInt(), writeString(), ...
//    writer.endRecord();
//    // more sequences of beginRecord(), writeXxx(), endRecord()
//    writer.endFile()
//    close(fd)
//
// That is, call beginFile() at the beginning and endFile() at the end.
// For each record, call beginRecord() then one of the  writeXxx()
// methods zero or more times (zero only for GDATA_NONE records) and
// finish by calling endRecord().
//
// For empty (GDATA_NONE) records you can use the method writeEmptyRecord()
// provided for convenience.  Similarly, for records that contain exactly
// one data item you can use one of the writeXxxRecord() methods.
//
// The constructor may throw bad_alloc; all other public methods apart
// from the destructor may throw runtime_error.


class GdsWriter {
    WriteBuffer<char>  wbuf;    // output buffer
    FileHandle  fh;             // for writing to output file
    char*       recStart;       // first byte of record being written
    std::string filename;       // pathname of output file
    const GdsRecordTypeInfo*  currType; // record type now being written

public:
                GdsWriter (const char* fname, FileHandle::FileType ftype);
                ~GdsWriter();

    void        beginFile();
    void        endFile();
    void        beginRecord (const GdsRecordTypeInfo* rti);
    void        beginRecord (GdsRecordType recType);
    void        endRecord();

    void        writeBitArray (Uint val);
    void        writeShort (int val);
    void        writeInt (int val);
    void        writeDouble (double val);
    void        writeString (const char* str, Uint strLength);
    void        writeString (const char* str);

    void        writeEmptyRecord (GdsRecordType recType);
    void        writeBitArrayRecord (GdsRecordType recType, Uint val);
    void        writeShortRecord (GdsRecordType recType, int val);
    void        writeIntRecord (GdsRecordType recType, int val);
    void        writeDoubleRecord (GdsRecordType recType, double val);
    void        writeStringRecord (GdsRecordType recType, const char* str);

private:
    void        ensureSpaceForRecord();
    void        writeByte (char ch);
    void        fillBytes (char ch, Uint numCopies);
    void        writeBytes (const void* data, Uint numBytes);
    void        abortWriter();

                GdsWriter (const GdsWriter&);           // forbidden
    void        operator= (const GdsWriter&);           // forbidden
};


//----------------------------------------------------------------------
// Private methods


inline void
GdsWriter::writeByte (char ch) {
    assert (wbuf.charsFree() >= 1);
    *wbuf.curr++ = ch;
}



inline void
GdsWriter::fillBytes (char ch, Uint numCopies)
{
    assert (wbuf.charsFree() >= numCopies);
    std::memset(wbuf.curr, ch, numCopies);
    wbuf.curr += numCopies;
}



inline void
GdsWriter::writeBytes (const void* data, Uint nbytes)
{
    assert (wbuf.charsFree() >= nbytes);
    std::memcpy(wbuf.curr, data, nbytes);
    wbuf.curr += nbytes;
}


//----------------------------------------------------------------------
// Public methods



// beginRecord -- begin writing a new GDSII record.
//   rti        ptr to RecordTypeInfo struct for this record type

inline void
GdsWriter::beginRecord (const GdsRecordTypeInfo* rti)
{
    // Make sure there buffer has enough space for the largest possible
    // record.  We don't want to empty the buffer in the middle of the
    // record because we fill in the length field of the header only at
    // the end of the record.

    ensureSpaceForRecord();

    // Remember the record's starting point so that endRecord() can
    // calculate the record length; then skip past the length field.

    recStart = wbuf.curr;
    wbuf.curr += 2;

    writeByte(rti->recType);
    writeByte(rti->datatype);
    currType = rti;
}



inline void
GdsWriter::beginRecord (GdsRecordType recType) {
    beginRecord(GdsRecordTypeInfo::get(recType));
}



// endRecord -- finish writing a GDSII record.

inline void
GdsWriter::endRecord()
{
    assert (currType != Null);

    // Fill in the header's length field, which beginRecord() left blank.

    Uint  length = wbuf.curr - recStart;
    assert (length - RecordHeaderLength >= currType->minLength);
    assert (length - RecordHeaderLength <= currType->maxLength);

    recStart[0] = length >> 8;
    recStart[1] = length;
    currType = Null;
}



// writeBitArray -- write a GDSII 2-byte bit array (GDATA_BITARRAY)

inline void
GdsWriter::writeBitArray (Uint val)
{
    assert (currType != Null  &&  currType->getDataType() == GDATA_BITARRAY);
    writeByte(val >> 8);
    writeByte(val);
}


// writeShort -- write a GDSII 2-byte integer (GDATA_SHORT)

inline void
GdsWriter::writeShort (int val)
{
    assert (currType != Null  &&  currType->getDataType() == GDATA_SHORT);
    writeByte(val >> 8);
    writeByte(val);
}



// writeInt -- write a GDSII 4-byte integer (GDATA_INT)

inline void
GdsWriter::writeInt (int val)
{
    assert (currType != Null  &&  currType->getDataType() == GDATA_INT);
    writeByte(val >> 24);
    writeByte(val >> 16);
    writeByte(val >>  8);
    writeByte(val);
}



// writeDouble -- write a GDSII 8-byte floating-point number (GDATA_DOUBLE).

inline void
GdsWriter::writeDouble (double val)
{
    assert (currType != Null  &&  currType->getDataType() == GDATA_DOUBLE);

    // DoubleToGdsReal() should not fail because val has already been
    // verified to be in the range of GDSII-representable numbers.

    Uchar  buf[8];
    if (! DoubleToGdsReal(val, buf))
        assert (false);
    writeBytes(buf, 8);
}



// writeString -- write a GDSII string (GDATA_STRING) and add NUL padding.
//   str        string to write (need not be NUL-terminated)
//   len        length of str

inline void
GdsWriter::writeString (const char* str, Uint len)
{
    assert (currType != Null  &&  currType->getDataType() == GDATA_STRING);
    assert (currType->itemSize == 0  ||  len <= currType->itemSize);

    // Some records contain one or more fixed-length strings.  The
    // RecordTypeInfo for such records has itemSize > 0.  For these
    // records, length can be at most itemSize, and the string is
    // NUL-padded to the required length.  Other records (with itemSize == 0)
    // contain a single variable-length string.  For these a single NUL
    // byte is added if needed to make the length even.

    int  itemSize = currType->itemSize;
    int  padding = itemSize - static_cast<int>(len);

    writeBytes(str, len);
    if (padding > 0)
        fillBytes(Cnul, padding);
    else if (itemSize == 0  && (len & 0x1))
        writeByte(Cnul);
}



// writeString -- write a NUL-terminated string

inline void
GdsWriter::writeString (const char* str) {
    writeString(str, strlen(str));
}



// writeEmptyRecord -- write an empty record (datatype == GDATA_NONE)

inline void
GdsWriter::writeEmptyRecord (GdsRecordType recType)
{
    // We could code this as a beginRecord()/endRecord() sequence,
    // but empty records occur often enough to justify special code.

    ensureSpaceForRecord();
    writeByte(0);               // high byte of length
    writeByte(4);               // low byte of length
    writeByte(recType);
    writeByte(GDATA_NONE);
}



}  // namespace Gdsii

#endif  //  GDSII_WRITER_H_INCLUDED
