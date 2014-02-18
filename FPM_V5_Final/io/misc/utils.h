// misc/utils.h
//
// last modified:   01-Jan-2010  Fri  21:57
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef MISC_UTILS_H_INCLUDED
#define MISC_UTILS_H_INCLUDED

#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <boost/function.hpp>

#include "port/compiler.h"
#include "globals.h"


namespace SoftJin {

using std::string;


// WarningHandler -- a functor that displays a warning message.
// A warning handler is invoked with a single argument, the message.  It
// should write the message to stderr or display it in a dialog box.

typedef boost::function<void (const char*)>   WarningHandler;



// streq -- compare strings for equality
// This is clearer than the idiom  !strcmp(x,y)
// The ! there suggests that the test has failed.

inline bool
streq (const char* stra, const char* strb) {
    return (std::strcmp(stra, strb) == 0);
}



// ProcessorIsLittleEndian -- running on little-endian machine like x86?
// We don't handle machines like the PDP-11 that are neither big-endian
// nor little-endian.  Note that there is no runtime cost in calling
// this function.  gcc with -O2 optimizes out of existence the test and
// any dead code that depends on it.

inline bool
ProcessorIsLittleEndian()
{
    union {
        long  lval;
        char  cval[sizeof(long)];
    } uu;
    uu.lval = 1;
    return (uu.cval[0] != 0);
}



// StringNCopy -- safe version of strncpy()
//   dest       destination of copy
//   destSize   number of bytes available at dest
//   src        NUL-terminated source string
// Copies at most destSize bytes from src and puts a NUL in the
// last byte of dest to ensure that it is NUL-terminated.

inline char*
StringNCopy (char* dest, size_t destSize, const char* src)
{
    std::strncpy(dest, src, destSize);
    dest[destSize-1] = Cnul;
    return dest;
}



// Hashing functors for use with the class templates HashSet and
// HashMap.  HashCppString and HashCppStringPtr are for hashes whose
// keys are C++ strings.  HashPointer is for hashes whose keys are
// pointers.


size_t HashString (const string& s);

struct HashCppString {
    size_t
    operator() (const string& s) const {
        return HashString(s);
    }
};


struct HashCppStringPtr {
    size_t
    operator() (const string* s) const {
        return HashString(*s);
    }
};


struct HashPointer {
    size_t
    operator() (const void* ptr) const {
        return reinterpret_cast<Ulong>(ptr);
    }
};



// StringEqual -- compare strings for equality
// This is used as the equality-test template argument to HashMap
// when the key is a C string.

struct StringEqual {
    bool operator() (const char* s1, const char* s2) const {
        return (strcmp(s1, s2) == 0);
    }
};



// Similarly this is used when the key is a C++ string object.  We use a
// string pointer as the key when the value is a struct that includes
// the string as a field.  Instead of copying the string into the key,
// we just point to it.

struct CppStringPtrEqual {
    bool operator() (const string* s1, const string* s2) const {
        return (*s1 == *s2);
    }
};



// DeleteContainerElements -- delete all objects in container.
// Also clears the container after deleting everything.
// The container's value_type must be T* for some T and all the values in
// the container must point to objects allocated in the free store.

template <typename Container>
inline void
DeleteContainerElements (/*inout*/ Container& cont)
{
    typename Container::iterator  iter = cont.begin();
    typename Container::iterator  end  = cont.end();
    for ( ; iter != end;  ++iter)
        delete *iter;
    cont.clear();
}


Uint            VSNprintf (/*out*/ char* buf, size_t bufsize,
                           const char* fmt, va_list ap);
Uint            SNprintf (/*out*/ char* buf, size_t bufsize,
                          const char* fmt, ...);

void            SetProgramName (const char* pname);
const char*     GetProgramName();
void            Error (const char* fmt, ...) SJ_PRINTF_ARGS(1,2);
void            FatalError (const char* fmt, ...)
                                             SJ_PRINTF_ARGS(1,2) SJ_NORETURN;
void            ThrowRuntimeError (const char* fmt, ...)
                                             SJ_PRINTF_ARGS(1,2) SJ_NORETURN;

char*           CharToString (/*out*/ char* buf, size_t bufsize, Uchar ch);
char*           NewString (const char* str);
char*           NewString (const char* str, size_t len);
char*           StringNCopy (char* dest, size_t destSize, const char* src);
char*           MakePrintableString (const char* str, size_t nchars,
                                     /*out*/ char* buf, size_t bufSize);

bool            DoubleIsIntegral (double val, /*out*/ long* plval);
bool            DoubleIsFloat (double val, /*out*/ float* pfval);

ssize_t         WriteNBytes (int fd, const void* buf, size_t count);
ssize_t         ReadNBytes (int fd, /*out*/ void* buf, size_t count);
int             OpenFile (const char* pathname, int flags, mode_t mode = 0666);
FILE*           FopenFile (const char* pathname, const char* mode);


} // namespace SoftJin

#endif  // MISC_UTILS_H_INCLUDED
