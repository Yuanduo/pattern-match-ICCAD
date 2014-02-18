// misc/buffer.h -- data buffers allocated in the free store
//
// last modified:   27-Nov-2004  Sat  18:55
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef MISC_BUFFER_H_INCLUDED
#define MISC_BUFFER_H_INCLUDED

#include <cassert>
#include <string>
#include "globals.h"


namespace SoftJin {

//----------------------------------------------------------------------
//                                Buffer
//----------------------------------------------------------------------

// Buffer -- manage a buffer allocated in the free store
// This class is declared as a struct because it provides no
// abstraction.  Its main job is to ensure that the heap-allocated
// buffer is deleted automatically.  The subclasses ReadBuffer and
// WriteBuffer provide some functions for convenience, but in general
// the application is expected to manipulate the pointers directly.
//
// The template argument CharT is normally char or Uchar.

template <typename CharT>
struct Buffer {
    CharT*      buffer;         // Null or beginning of buffer

                Buffer();
    explicit    Buffer (size_t bufsize);
                ~Buffer();

private:
                Buffer (const Buffer&);                 // forbidden
    void        operator= (const Buffer&);              // forbidden
};



template <typename CharT>
inline
Buffer<CharT>::Buffer() {
    buffer = Null;
}


template <typename CharT>
inline
Buffer<CharT>::Buffer (size_t bufsize) {
    buffer = new CharT[bufsize];
}


template <typename CharT>
inline
Buffer<CharT>::~Buffer() {
    delete[] buffer;
}


//----------------------------------------------------------------------
//                              ReadBuffer
//----------------------------------------------------------------------

// ReadBuffer -- buffer intended for readers (data consumers)
// `curr' points to the next byte to be read and `end' points to the
// byte after the last data byte in the buffer.  Both are valid only
// when buffer != Null.
//
// Classes that switch between two ReadBuffers copy curr to another
// variable and update only that; copying the variable back to curr
// when switching buffers.

template <typename CharT>
struct ReadBuffer : public Buffer<CharT> {
    CharT*      curr;   // next byte to read
    CharT*      end;    // end of data

                ReadBuffer() { }
    explicit    ReadBuffer (size_t bufsize);
    void        create (size_t bufsize);
    void        refill (size_t dataChars);
    size_t      availData() const;
    bool        empty() const;
};



template <typename CharT>
inline
ReadBuffer<CharT>::ReadBuffer (size_t bufsize)
    : Buffer<CharT>(bufsize)
{
    curr = end = this->buffer;
}



// create -- allocate buffer of specified size

template <typename CharT>
inline void
ReadBuffer<CharT>::create (size_t bufsize)
{
    assert (this->buffer == Null);
    this->buffer = new CharT[bufsize];
    curr = end = this->buffer;          // buffer is initially empty
}



// refill -- tell buffer it has been refilled with data
//   nchars      how many data chars are now in the buffer

template <typename CharT>
inline void
ReadBuffer<CharT>::refill (size_t nchars)
{
    assert (this->buffer != Null);
    curr = this->buffer;
    end = this->buffer + nchars;
}



// availData -- return number of characters of data in the buffer

template <typename CharT>
inline size_t
ReadBuffer<CharT>::availData() const
{
    assert (this->buffer != Null);
    return (end - curr);
}



// empty -- true if no more characters remain to be read

template <typename CharT>
inline bool
ReadBuffer<CharT>::empty() const
{
    assert (this->buffer != Null);
    return (end == curr);
}


//----------------------------------------------------------------------
//                              WriteBuffer
//----------------------------------------------------------------------

// WriteBuffer -- buffer intended for writers (data producers)
// `curr' points to where the next byte should be written, and `end'
// points to the byte after the buffer.  end's value does not
// change after the buffer is created.  Both members are valid only when
// buffer != Null.
//
// Classes that switch between two WriteBuffers copy curr to another
// variable and update only that, copying the variable back to curr
// when switching buffers.

template <typename CharT>
struct WriteBuffer : public Buffer<CharT> {
    CharT*      curr;   // where to write next byte
    CharT*      end;    // end of buffer

                WriteBuffer() { }
    explicit    WriteBuffer (size_t bufsize);
    void        create (size_t bufsize);
    void        flush (size_t numRemove);
    void        flushAll();
    size_t      charsUsed() const;
    size_t      charsFree() const;
};



template <typename CharT>
inline
WriteBuffer<CharT>::WriteBuffer (size_t bufsize)
    : Buffer<CharT>(bufsize)
{
    curr = this->buffer;
    end = this->buffer + bufsize;
}



// create -- allocate buffer of specified size

template <typename CharT>
inline void
WriteBuffer<CharT>::create (size_t bufsize)
{
    assert (this->buffer == Null);
    this->buffer = new CharT[bufsize];
    curr = this->buffer;
    end =  this->buffer + bufsize;
}



// flush -- remove the first nchars characters of data from the buffer
// Call this when some of the data has been flushed to the destination.

template <typename CharT>
inline void
WriteBuffer<CharT>::flush (size_t nchars)
{
    assert (this->buffer != Null);
    assert (nchars <= charsUsed());

    // Move whatever data is left to the beginning of the buffer.

    size_t  remainder = charsUsed() - nchars;
    if (remainder > 0)
        std::char_traits<CharT>::move(this->buffer, this->buffer + nchars,
                                      remainder);
    curr = this->buffer + remainder;
}



// flushAll -- remove all data from the buffer
// Special case of flush().  Call this when all data has been flushed.

template <typename CharT>
inline void
WriteBuffer<CharT>::flushAll()
{
    assert (this->buffer != Null);
    curr = this->buffer;
}



// charsUsed -- how many characters are now in the buffer

template <typename CharT>
inline size_t
WriteBuffer<CharT>::charsUsed() const
{
    assert (this->buffer != Null);
    return (curr - this->buffer);
}


// charsFree -- how many characters can be written before buffer becomes full

template <typename CharT>
inline size_t
WriteBuffer<CharT>::charsFree() const
{
    assert (this->buffer != Null);
    return (end - curr);
}


// C++ Note
// If you are wondering about the use of `this->' in the code above,
// it is because of the language requirement that names that depend
// on the template argument must be qualified.  The data member `buffer'
// is defined in the base class Buffer<T>, which depends on the
// template argument T.


}  // namespace SoftJin

#endif  // MISC_BUFFER_H_INCLUDED
