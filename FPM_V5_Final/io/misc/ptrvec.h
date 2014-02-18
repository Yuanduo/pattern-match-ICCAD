// misc/ptrvec.h -- vector of pointers to objects allocated on the free store
//
// last modified:   24-Feb-2005  Thu  12:23
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// This is for vectors that own the objects they point to.  The objects
// pointed to are deleted when the vectors are destroyed.


#ifndef MISC_PTRVEC_H_INCLUDED
#define MISC_PTRVEC_H_INCLUDED

#include <vector>
#include <memory>
#include "globals.h"

namespace SoftJin {

using std::vector;



template <typename T>
class PointerVector : private vector<T*> {
public:
    typedef typename vector<T*>::pointer                pointer;
    typedef typename vector<T*>::const_pointer          const_pointer;
    typedef typename vector<T*>::reference              reference;
    typedef typename vector<T*>::const_reference        const_reference;

    typedef typename vector<T*>::size_type              size_type;
    typedef typename vector<T*>::value_type             value_type;
    typedef typename vector<T*>::difference_type        difference_type;

    typedef typename vector<T*>::iterator               iterator;
    typedef typename vector<T*>::const_iterator         const_iterator;
    typedef typename vector<T*>::reverse_iterator       reverse_iterator;
    typedef typename vector<T*>::const_reverse_iterator const_reverse_iterator;

    using vector<T*>::begin;
    using vector<T*>::end;
    using vector<T*>::rbegin;
    using vector<T*>::rend;

    using vector<T*>::back;
    using vector<T*>::capacity;
    using vector<T*>::clear;
    using vector<T*>::empty;
    using vector<T*>::erase;
    using vector<T*>::front;
    using vector<T*>::insert;
    using vector<T*>::max_size;
    using vector<T*>::pop_back;
    using vector<T*>::push_back;
    using vector<T*>::reserve;
    using vector<T*>::size;
    using vector<T*>::operator[];

                PointerVector() { }
                ~PointerVector();
                PointerVector (const PointerVector&);
    PointerVector&  operator= (const PointerVector&);

    void        swap (PointerVector& other);
    void        resize (size_type n);
};



// copy constructor
// It copies each of the objects that other points to, and puts pointers
// to the copies in this.  Assumes that there is no polymorphism, i.e.,
// that the objects that 'other' points to actually have the type
// indicated by the template arg T.

template <typename T>
PointerVector<T>::PointerVector (const PointerVector<T>& other)
  : vector<T*>()
{
    reserve(other.size());
    const_iterator  iter    = other.begin();
    const_iterator  endIter = other.end();
    for ( ;  iter != endIter;  ++iter) {
        T* ptr = *iter;
        std::auto_ptr<T>  clone(ptr == Null ? Null : new T(*ptr));
        push_back(clone.get());
        clone.release();
    }
}


template <typename T>
inline
PointerVector<T>::~PointerVector() {
    for (iterator iter = begin();  iter != end();  ++iter)
        delete *iter;
}


template <typename T>
inline void
PointerVector<T>::swap (PointerVector<T>& other) {
    vector<T*>::swap(other);
}



// Copy assignment operator
// Like the copy constructor, this assumes that the elements of
// other are of type T, not a subclass of T.

template <typename T>
inline PointerVector<T>&
PointerVector<T>::operator= (const PointerVector<T>& other)
{
    PointerVector<T>  temp(other);
    swap(temp);
    return *this;
}



// resize
// Unlike vector<T>::resize() this takes only one argument, the new
// size.  If the new size is less than the current size, the extra
// elements are deleted before resizing.  If the new size is greater,
// the new positions are filled with Nulls.

template <typename T>
void
PointerVector<T>::resize (size_type n)
{
    if (n < size()) {
        iterator  iter = begin() + n;
        iterator  endIter = end();
        for (  ;  iter != endIter;  ++iter)
            delete *iter;
    }
    vector<T*>::resize(n, Null);
}    


}  // namespace SoftJin

#endif  // MISC_PTRVEC_H_INCLUDED
