// port/hash-table.h -- portable interface to hash_map and hash_set
//
// last modified:   02-Jan-2010  Sat  09:46
//
// Copyright (c) 2009 SoftJin Technologies Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

// This file defines class templates HashMap and HashSet, which can be
// configured to use either the SGI classes hash_map and hash_set or the
// TR1 classes unordered_map and unordered_set.  Other kinds of hash
// tables, e.g., Dinkumware's hash_map and hash_set, are not supported.

#ifndef PORT_HASH_TABLE_H_INCLUDED
#define PORT_HASH_TABLE_H_INCLUDED


// Porting to a new compiler
//
// If the C++ compiler supports TR1, define the symbol SJ_HAVE_CPP_TR1.
// Otherwise the compiler must support SGI-compatible hash_map and
// hash_set.  You must then
//
//   - add #includes for both header files;
//
//   - define HashMap and HashSet in the SoftJin namespace by deriving
//     from hash_map and hash_set, respectively;
//
//   - add a using-declaration to bring the class template 'hash'
//     into the SoftJin namespace.


#if (__GNUC__ > 4  ||  (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
    #define SJ_HAVE_CPP_TR1  1
#endif

#if defined(SJ_HAVE_CPP_TR1)
    #include <functional>
    #include <tr1/unordered_map>
    #include <tr1/unordered_set>
#elif defined(__GNUC__)
    #include <ext/hash_map>
    #include <ext/hash_set>
#else
    #error Include the system header files for hash_map and hash_set
#endif


namespace SoftJin {

using std::equal_to;


//======================================================================
//           gcc 4.3+ or any other compiler that supports TR1
//======================================================================
#if defined(SJ_HAVE_CPP_TR1)

using std::tr1::hash;

// We do not use the allocator template arg.

template <typename KeyT,
          typename ValueT,
          typename HashT = hash<KeyT>,
          typename EqualT = equal_to<KeyT> >
class HashMap
  : public std::tr1::unordered_map<KeyT,ValueT,HashT,EqualT>
{
    typedef std::tr1::unordered_map<KeyT,ValueT,HashT,EqualT>  BaseType;

public:
    HashMap() { }
    HashMap (size_t initSize) : BaseType(initSize) { }

    template <typename InputIter>
    HashMap (InputIter first, InputIter last) : BaseType(first, last) { }
};



template <typename KeyT,
          typename HashT = hash<KeyT>,
          typename EqualT = equal_to<KeyT> >
class HashSet
  : public std::tr1::unordered_set<KeyT,HashT,EqualT>
{
    typedef std::tr1::unordered_set<KeyT,HashT,EqualT>   BaseType;

public:
    HashSet() { }
    HashSet (size_t initSize) : BaseType(initSize) { }

    template <typename InputIter>
    HashSet (InputIter first, InputIter last) : BaseType(first, last) { }
};


//======================================================================
//                        Older versions of gcc
//======================================================================
#elif defined(__GNUC__)

using __gnu_cxx::hash;


template <typename KeyT,
          typename ValueT,
          typename HashT = hash<KeyT>,
          typename EqualT = equal_to<KeyT> >
class HashMap
  : public __gnu_cxx::hash_map<KeyT,ValueT,HashT,EqualT>
{
    typedef __gnu_cxx::hash_map<KeyT,ValueT,HashT,EqualT>   BaseType;

public:
    HashMap() { }
    HashMap (size_t initSize) : BaseType(initSize) { }

    template <typename InputIter>
    HashMap (InputIter first, InputIter last) : BaseType(first, last) { }
};



template <typename KeyT,
          typename HashT = hash<KeyT>,
          typename EqualT = equal_to<KeyT> >
class HashSet
  : public __gnu_cxx::hash_set<KeyT,HashT,EqualT>
{
    typedef __gnu_cxx::hash_set<KeyT,HashT,EqualT>   BaseType;

public:
    HashSet() { }
    HashSet (size_t initSize) : BaseType(initSize) { }

    template <typename InputIter>
    HashSet (InputIter first, InputIter last) : BaseType(first, last) { }
};


//======================================================================

#else
#   error  Define HashMap and HashSet for this platform
#endif


}  // namespace SoftJin

#endif  // PORT_HASH_TABLE_H_INCLUDED
