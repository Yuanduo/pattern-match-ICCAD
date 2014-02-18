// misc/gzfile.h -- hide difference between normal and gzipped files
//
// last modified:   05-Jun-2008  Thu  10:22
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef MISC_GZFILE_H_INCLUDED
#define MISC_GZFILE_H_INCLUDED

#include <cassert>
#include <fcntl.h>
#include <string>
#include <sys/types.h>

namespace SoftJin {


// FileHandle -- handle to gzipped or uncompressed file
// This class provides a minimal interface that hides the difference
// between compressed (gzipped) and uncompressed files.  The
// constructor's argument ftype specifies whether the file is normal
// (uncompressed) or gzipped.  If ftype is FileTypeAuto, the type is
// deduced from the file name: the file is gzipped if and only if the
// file name has the suffix '.gz'.
//
// The accMode parameter of open() says whether the file is to be opened
// for reading or writing.  If writing, an existing file is truncated
// (as with O_TRUNC), and a new file is created if needed.  The file
// permissions are 0666 modified by the umask.  open() throws
// runtime_error if it fails.
//
// close(), read(), and write() have the same semantics as the system
// calls.  seek() is also like the system call, but with the
// restrictions of gzseek() in <zlib.h>: SEEK_END is not allowed, and in
// write mode only forward seeks are allowed.

class FileHandle {
public:
    enum FileType {             // constructor argument
        FileTypeAuto,           // deduce type from extension: .gz => gzipped
        FileTypeNormal,         // uncompressed
        FileTypeGzip            // gzipped
    };
    enum AccessMode {           // argument to open() method
        ReadAccess  = O_RDONLY,
        WriteAccess = O_WRONLY | O_CREAT | O_TRUNC
    };

private:
    std::string      fname;
    class FileImpl*  impl;      // instance of appropriate body class
    bool             isOpen;    // true => file is open

public:
                FileHandle (const char* fname, FileType ftype);
                ~FileHandle();
    void        open (AccessMode accMode);
    int         close();
    ssize_t     read (/*out*/ void* buf, size_t nbytes);
    ssize_t     write (const void* buf, size_t nbytes);
    off_t       seek (off_t offset, int whence);
};


}  // namespace SoftJin

#endif  // MISC_GZFILE_H_INCLUDED
