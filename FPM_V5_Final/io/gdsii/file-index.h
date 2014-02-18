// gdsii/file-index.h -- map structure names to file offsets
//
// last modified:   02-Jan-2010  Sat  10:03
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef GDSII_FILE_INDEX_H_INCLUDED
#define GDSII_FILE_INDEX_H_INCLUDED

#include "port/hash-table.h"
#include "misc/stringpool.h"
#include "misc/utils.h"

namespace Gdsii {

using SoftJin::hash;
using SoftJin::HashMap;
using SoftJin::StringEqual;
using SoftJin::StringPool;


class FileIndex {
    typedef HashMap<const char*, off_t, hash<const char*>, StringEqual>
            CellOffsetMap;

    CellOffsetMap  nameToOffset;
    StringPool  namePool;               // holds all the structure names
    bool        done;

public:
                FileIndex();
    void        clear();
    bool        addEntry (const char* sname, off_t offset);
    off_t       getOffset (const char* sname) const;

    // setDone, isDone -- set and test whether the index is complete
    void        setDone()      { done = true; }
    bool        isDone() const { return done; }

    // size -- number of entries in the index
    size_t      size() const   { return nameToOffset.size(); }

    template <class OutIter>
    void        getAllNames (OutIter oiter);

private:
                FileIndex (const FileIndex&);           // forbidden
    void        operator= (const FileIndex&);           // forbidden
};



inline
FileIndex::FileIndex()
    : nameToOffset(512)
{
    done = false;
}



// addEntry -- add a name/offset pair to the index.
//   sname      NUL-terminated name of file section
//   offset     file offset at which the section begins
// Returns true on success, false if sname already appears in the index.

inline bool
FileIndex::addEntry (const char* sname, off_t offset)
{
    // CellOffsetMap's value_type is <const char *const, off_t>.
    // CellOffsetMap::insert() returns a pair<iterator,bool> whose
    // second element is true if the new value was inserted and false if
    // the value's key was already present in the table.

    sname = namePool.newString(sname);
    CellOffsetMap::value_type  val(sname, offset);
    return (nameToOffset.insert(val).second);
}



// getOffset -- get file offset at which section begins.
//   sname      NUL-terminated name of section
// Returns -1 if the section has not been defined.

inline off_t
FileIndex::getOffset (const char* sname) const
{
    CellOffsetMap::const_iterator  iter = nameToOffset.find(sname);
    return (iter == nameToOffset.end() ? -1 : iter->second);
}



// clear -- reset the index

inline void
FileIndex::clear()
{
    namePool.clear();
    nameToOffset.clear();
    done = false;
}



// getNames -- get all names in index in some random order
//   oiter      output iterator for const char* objects
// All the char* pointers returned point into space owned by the
// FileIndex object.
// Sample usage:
//    vector<const char*> vec;
//    index.getAllNames(back_inserter(vec));

template <class OutIter>
void
FileIndex::getAllNames (OutIter oiter)
{
    CellOffsetMap::iterator  iter = nameToOffset.begin(),
                             end  = nameToOffset.end();
    for ( ;  iter != end;  ++iter) {
        *oiter = iter->first;
        ++oiter;
    }

    // Could use this instead, but select1st is non-standard:
    // transform(nameToOffset.begin(), nameToOffset.end(), oiter, select1st);
}


}  // namespace Gdsii

#endif  // GDSII_FILE_INDEX_H_INCLUDED
