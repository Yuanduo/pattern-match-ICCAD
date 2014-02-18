// misc/gzfile.cc -- hide difference between normal and gzipped files
//
// last modified:   28-Nov-2004  Sun  06:56
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cstring>
#include <new>
#include <unistd.h>
#include <zlib.h>

#include "gzfile.h"
#include "utils.h"

namespace SoftJin {

using namespace std;


// FileImpl -- virtual base class for the implementation classes.
// Its two concrete subclasses, NormalFile and GzipFile, deal with
// uncompressed and compressed files, respectively.

class FileImpl {
public:
                    FileImpl()  { }
    virtual         ~FileImpl() { }
    virtual void    open (const char* fname,
                          FileHandle::AccessMode accMode) = 0;
    virtual int     close() = 0;
    virtual ssize_t read (/*out*/ void* buf, size_t nbytes) = 0;
    virtual ssize_t write (const void* buf, size_t nbytes) = 0;
    virtual off_t   seek (off_t offset, int whence) = 0;
};


//----------------------------------------------------------------------
//                              NormalFile
//----------------------------------------------------------------------

// NormalFile -- implementation class for uncompressed files

class NormalFile : public FileImpl {
    int     fd;
public:
                    NormalFile();
    virtual         ~NormalFile() { }
    virtual void    open (const char* fname, FileHandle::AccessMode accMode);
    virtual int     close();
    virtual ssize_t read (/*out*/ void* buf, size_t nbytes);
    virtual ssize_t write (const void* buf, size_t nbytes);
    virtual off_t   seek (off_t offset, int whence);
};


NormalFile::NormalFile() {
    fd = -1;
}


void
NormalFile::open (const char* fname, FileHandle::AccessMode accMode) {
    fd = OpenFile(fname, accMode);
}


int
NormalFile::close() {
    return (::close(fd));
}


ssize_t
NormalFile::read (/*out*/ void* buf, size_t nbytes) {
    return (ReadNBytes(fd, buf, nbytes));
}


ssize_t
NormalFile::write (const void* buf, size_t nbytes) {
    return (WriteNBytes(fd, buf, nbytes));
}


off_t
NormalFile::seek (off_t offset, int whence) {
    return (lseek(fd, offset, whence));
}


//----------------------------------------------------------------------
//                              GzipFile
//----------------------------------------------------------------------

class GzipFile : public FileImpl {
    gzFile      gzf;            // zlib's file handle
public:
                    GzipFile();
    virtual         ~GzipFile() { }
    virtual void    open (const char* fname, FileHandle::AccessMode accMode);
    virtual int     close();
    virtual ssize_t read (/*out*/ void* buf, size_t nbytes);
    virtual ssize_t write (const void* buf, size_t nbytes);
    virtual off_t   seek (off_t offset, int whence);
};



GzipFile::GzipFile() {
    gzf = Null;
}



void
GzipFile::open (const char* fname, FileHandle::AccessMode accMode)
{
    // Instead of using gzopen() to open the file, use OpenFile() and
    // then gzdopen().  We get better error messages that way.

    const char*  gzMode = (accMode == FileHandle::ReadAccess) ? "rb" : "wb";
    int  fd = OpenFile(fname, accMode);
    if ((gzf = gzdopen(fd, gzMode)) == Null) {
        ::close(fd);
        throw bad_alloc();
    }
}



int
GzipFile::close()
{
    return (gzclose(gzf) == Z_OK ? 0 : -1);
}


ssize_t
GzipFile::read (/*out*/ void* buf, size_t nbytes) {
    return (gzread(gzf, buf, nbytes));
}


ssize_t
GzipFile::write (const void* buf, size_t nbytes) {
    // XXX: loop as in WriteNBytes?
    return (gzwrite(gzf, const_cast<void*>(buf), nbytes));
}


off_t
GzipFile::seek (off_t offset, int whence) {
    return (gzseek(gzf, offset, whence));
}



//----------------------------------------------------------------------
//                              FileHandle
//----------------------------------------------------------------------

// FileHandle constructor
//   fname      pathname of file to read or create
//   ftype      type of file: compressed or uncompressed

FileHandle::FileHandle (const char* fname, FileType ftype)
  : fname(fname)
{
    isOpen = false;

    // If the file type is auto, look at the extension to tell whether it
    // is gzipped.

    if (ftype == FileTypeAuto) {
        const char*  ext = strrchr(fname, '.');
        if (ext != Null  &&  streq(ext, ".gz"))
            ftype = FileTypeGzip;
        else
            ftype = FileTypeNormal;
    }

    switch (ftype) {
        case FileTypeNormal: impl = new NormalFile();   break;
        case FileTypeGzip:   impl = new GzipFile();     break;
        default:             assert (false);
    }
}



FileHandle::~FileHandle() {
    delete impl;
}


void
FileHandle::open (AccessMode accMode) {
    impl->open(fname.c_str(), accMode);
    isOpen = true;
}



int
FileHandle::close()
{
    // For the classes that use FileHandle it is convenient if close()
    // can be invoked twice.

    if (! isOpen)
        return -1;
    isOpen = false;
    return (impl->close());
}


ssize_t
FileHandle::read (/*out*/ void* buf, size_t nbytes) {
    return  (impl->read(buf, nbytes));
}


ssize_t
FileHandle::write (const void* buf, size_t nbytes) {
    return (impl->write(buf, nbytes));
}


off_t
FileHandle::seek (off_t offset, int whence) {
    return (impl->seek(offset, whence));
}


}  // namespace SoftJin
