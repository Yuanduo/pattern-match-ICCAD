// misc/ptrlist.h -- list of pointers to objects allocated on the free store
//
// last modified:   17-Aug-2004  Tue  21:16
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// This is for lists that own the objects they point to.  The objects
// pointed to are deleted when the lists are destroyed.


#ifndef MISC_PTRLIST_H_INCLUDED
#define MISC_PTRLIST_H_INCLUDED

#include <list>
#include <memory>
#include "globals.h"


namespace SoftJin {

using std::list;


template <typename T>
class PointerList : private list<T*> {
public:
    typedef typename list<T*>::pointer          pointer;
    typedef typename list<T*>::const_pointer    const_pointer;
    typedef typename list<T*>::reference        reference;
    typedef typename list<T*>::const_reference  const_reference;

    typedef typename list<T*>::size_type        size_type;
    typedef typename list<T*>::value_type       value_type;
    typedef typename list<T*>::difference_type  difference_type;

    typedef typename list<T*>::iterator         iterator;
    typedef typename list<T*>::const_iterator   const_iterator;
    typedef typename list<T*>::reverse_iterator reverse_iterator;
    typedef typename list<T*>::const_reverse_iterator  const_reverse_iterator;

    using list<T*>::begin;
    using list<T*>::end;
    using list<T*>::rbegin;
    using list<T*>::rend;

    using list<T*>::back;
    using list<T*>::clear;
    using list<T*>::empty;
    using list<T*>::erase;
    using list<T*>::front;
    using list<T*>::insert;
    using list<T*>::max_size;
    using list<T*>::pop_back;
    using list<T*>::pop_front;
    using list<T*>::push_back;
    using list<T*>::push_front;
    using list<T*>::remove;
    using list<T*>::remove_if;
    using list<T*>::size;

                PointerList() { }
                ~PointerList();
                PointerList (const PointerList&);
    PointerList&  operator= (const PointerList&);

    void        swap (PointerList& other);

    template <typename Predicate>
    void        deleteIf (Predicate pred);
};



// copy constructor
// It copies each of the objects that other points to, and puts pointers
// to the copies in this.  Assumes that there is no polymorphism, i.e.,
// that the objects that other points to actually have the type
// indicated by the template arg T.

template <typename T>
PointerList<T>::PointerList (const PointerList<T>& other)
{
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
PointerList<T>::~PointerList() {
    for (iterator iter = begin();  iter != end();  ++iter)
        delete *iter;
}


template <typename T>
inline void
PointerList<T>::swap (PointerList<T>& other) {
    list<T*>::swap(other);
}



// Copy assignment operator
// Like the copy constructor, this assumes that there the elements of
// other are of type T, not a subclass of T.

template <typename T>
inline PointerList<T>&
PointerList<T>::operator= (const PointerList<T>& other)
{
    PointerList<T>  temp(other);
    swap(temp);
    return *this;
}



// deleteIf -- delete and remove all elements matching some predicate.
// This is like list's remove_if(), except that it also deletes the
// elements it removes.

template <typename T>
template <typename Predicate>
void
PointerList<T>::deleteIf (Predicate pred)
{
    iterator  iter    = begin();
    iterator  endIter = end();
    while (iter != endIter) {
        if (pred(*iter)) {
            delete *iter;
            erase(iter++);      // increment iter before erase invalidates it
        } else {
            ++iter;
        }
    }
}



}  // namespace SoftJin

#endif  // MISC_PTRLIST_H_INCLUDED
