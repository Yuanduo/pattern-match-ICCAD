// oasis/dicts.h -- dictionaries for names and cells
//
// last modified:   01-Jan-2010  Fri  22:02
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// A dictionary is the internal representation of an OASIS name table.
// OasisParser uses the dictionary classes defined here to manage the
// OasisName objects it creates and to map between OasisNames and
// reference-numbers.
//
// The class hierarchy:
//
// OasisDict
//     RefNameDict
//         CellNameDict      | These four are more or less aliases for
//         TextStringDict    | RefNameDict, just as CellName, TextString,
//         PropNameDict      | etc. are typedefs for OasisName.
//         PropStringDict    |
//     LayerNameDict
//     XNameDict
// CellDict

#ifndef OASIS_DICTS_H_INCLUDED
#define OASIS_DICTS_H_INCLUDED

#include <cassert>
#include <limits>
#include <string>
#include <vector>

#include "port/hash-table.h"
#include "misc/utils.h"
#include "names.h"
#include "oasis.h"


namespace Oasis {

using std::string;
using std::vector;
using SoftJin::Uint;
using SoftJin::Ulong;
using SoftJin::HashMap;
using SoftJin::hash;
using SoftJin::HashCppStringPtr;
using SoftJin::HashPointer;
using SoftJin::CppStringPtrEqual;



//======================================================================
//                              OasisDict
//======================================================================

// OasisDict -- base class for all dictionaries
// OasisParser uses OasisDict and its subclasses LayerNameDict and
// XNameDict to store the contents of OASIS name tables.
//
// strict               bool
//      true means that the name table is specified in the END record as
//      being strict.  That is, all <name> records should be in the
//      table (there should be no strays) and all references should be
//      through the reference-numbers.
//
// enforceStrict        bool
//      Whether strictness should be enforced for the table.
//      Spec paragraph 13.10 leaves it optional.  OasisParser
//      enforces strictness only when _all_ name tables are strict.
//
// startOffset          off_t
//      File offset where the name table begins.  Zero means that the
//      table does not exist.
//
// endPos               OFilePos
//      The position of the first byte after the table.  Might not be set.
//      The parser will invoke setEndPosition() to set this only if
//      it intends calling containsPosition() later to check for
//      stray <name> records.
//
// Note the asymmetry between the starting and ending positions of the
// name table.  The starting position is a plain offset because name
// tables are not allowed to begin in a cblock.  But the end position is
// an OFilePos because they can end in the middle of a cblock.
//
// Invariants
//   enforceStrict => strict
//
//   enforceStrict => (startOffset == 0 || endPos.fileOffset > 0)
//      (To enforce strictness, either the table should be empty or
//       setEndPosition() should have been called.)


class OasisDict {
    bool        strict;         // true => table is strict
    bool        enforce;        // true => signal error on stray records
    off_t       startOffset;    // file offset of first byte of table
    OFilePos    endPos;         // file position of first byte after table

protected:
                OasisDict() {
                    strict = false;
                    enforce = false;
                    startOffset = 0;
                    endPos.fileOffset = 0;
                }
                ~OasisDict() { }

public:
    void        setStrict (bool strict) {
                    this->strict = strict;
                }
    void        setStartOffset (off_t offset) {
                    startOffset = offset;
                }
    void        setEndPosition (OFilePos pos) {
                    assert (startOffset != 0);
                    endPos = pos;
                }
    void        enforceStrict (bool flag) { enforce = flag; }
    bool        enforceStrict() const     { return enforce; }

    bool        isStrict()       const { return strict;      }
    off_t       getStartOffset() const { return startOffset; }
    bool        containsPosition (OFilePos pos) const;

private:
                OasisDict (const OasisDict&);           // forbidden
    void        operator= (const OasisDict&);           // forbidden
};



// containsPosition -- true if the given file position is in the name table.
//   pos        file position in OASIS file
// OasisParser uses this to check for stray <name> records when table
// is strict.  It returns true if the file position is inside the extent
// occupied by the name table for this dictionary.  The extent should have
// been specified by earlier calls to setStartOffset() and, if the table
// is non-empty, setEndPosition().

inline bool
OasisDict::containsPosition (OFilePos pos) const
{
    // startOffset == 0 implies table is empty.  An empty table cannot
    // contain any position.

    assert (startOffset == 0  ||  endPos.fileOffset > 0);
    return (startOffset != 0
            &&  pos.fileOffset >= startOffset
            &&  pos < endPos);
}



//======================================================================
//                              RefNameDict
//======================================================================

// RefNameDict -- dictionary for CellName, PropName, TextString, PropString.
// A RefNameDict associates an OasisName object with a reference-number
// that can be used to locate the name object.  We don't store the
// refnum in the OasisName itself because OasisParser and OasisCreator
// may associate the same OasisName object with different refnums.
//
// uniqueNames          bool
//      If true, the names in all OasisName objects added to the
//      dictionary must be unique.  True for CellNames and PropNames,
//      false for TextStrings and PropStrings.
//
// autoNumber           bool
//      Initialized to false and set to true by the add() version that
//      does auto-numbering.  Its value is checked by the other add()
//      to ensure that for any given dictionary only one of the
//      overloaded add() methods is called.
//
// explicitNumber       bool
//      Initialized to false and set to true by the add() version that
//      takes a refnum argument.  As with autoNumber, it is checked by
//      the other add().
//
// nextRefnum           Ulong
//      This is used only when autoNumber is true.  It is the refnum to
//      assign to the next OasisName object created.
//
// allNames             vector<OasisName*>
//      The owning vector for all the OasisName objects in the
//      dictionary.  The dictionary iterator iterates through this.
//
// stringToName         StringToNameMap
//      Used only when uniqueNames is true.  Used to locate an
//      OasisName object given its name.  For speed we use a plain char*
//      as the key, not a string object.  Because the characters in the
//      key should last as long as the OasisName, the key should point
//      to the contents of the string in the OasisName.
//
// refnumToName         RefnumToNameMap
//      Used to get the OasisName object with a given refnum
//
// nameToRefnum         NameToRefnumMap
//      Used to get the refnum for a given OasisName.
//
// Every name in the dictionary will be in allNames and in one or more
// of the maps.  Note that an OasisName object may be temporarily
// nameless because a placeholder OasisName is created if a refnum is
// seen before the <name> record it references.
//
// Invariants
//   !autoNumber || !explicitNumber
//      (At most one of these flags can be set.)
//
//   !allNames.empty() => (autoNumber || explicitNumber)
//      (One of these flags must be set after the first name is added.)
//
//   autoNumber => (nextRefNum == allNames.size())
//      (With auto_numbering, numbers are assigned in sequence
//       starting from 0.  The next number to be assigned is
//       1 + the numbers already assigned.)


class RefNameDict : public OasisDict {
protected:
    typedef  HashMap<Ulong, OasisName*>   RefnumToNameMap;
    typedef  HashMap<OasisName*, Ulong, HashPointer>   NameToRefnumMap;
    typedef  HashMap<const string*, OasisName*,
                     HashCppStringPtr, CppStringPtrEqual>  StringToNameMap;

private:
    bool        uniqueNames;            // true for CellName, PropName
    bool        autoNumber;             // refnums automatically assigned?
    bool        explicitNumber;         // refnums explicitly assigned?
    Ulong       nextRefnum;             // next refnum when auto-numbering

    vector<OasisName*>  allNames;       // all names in the dictionary
    StringToNameMap     stringToName;   // get OasisName* given its name
    RefnumToNameMap     refnumToName;   // get OasisName* given refnum
    NameToRefnumMap     nameToRefnum;   // get refnum given OasisName*

public:
    explicit    RefNameDict (bool uniqueNames);
                ~RefNameDict();

    OasisName*  add (const string& str);
    OasisName*  add (const string& str, Ulong refnum);
    OasisName*  lookupName (const string& name, bool create);
    OasisName*  lookupRefnum (Ulong refnum, bool create);
    bool        getRefnum (OasisName* oname, /*out*/ Ulong* refnump) const;
    Ulong       getRefnum (OasisName* oname) const;

    // Iterators for all names in the dictionary

    typedef vector<OasisName*>::iterator        iterator;
    typedef vector<OasisName*>::const_iterator  const_iterator;

    iterator            begin()       { return allNames.begin(); }
    const_iterator      begin() const { return allNames.begin(); }
    iterator            end()         { return allNames.end();   }
    const_iterator      end()   const { return allNames.end();   }

private:
    OasisName*  doAdd (const string& str, Ulong refnum);
};



// getRefnum -- get the reference-number for an OasisName.
// This is a wrapper for the other getRefnum().  It's more convenient
// to use when the OasisName is guaranteed to have a refnum.

inline Ulong
RefNameDict::getRefnum (OasisName* oname) const
{
    Ulong  refnum;
    if (! getRefnum(oname, &refnum))
        assert(false);
    return refnum;
}


//----------------------------------------------------------------------


// Cellnames and propnames are unique; textstrings and propstrings are
// not.  Create a trivial subclass for each type of dictionary to
// prevent code that creates dictionaries from passing the wrong value
// for the constructor's parameter uniqueNames.


class CellNameDict : public RefNameDict {
public:
    CellNameDict() : RefNameDict(true) { }
};


class TextStringDict : public RefNameDict {
public:
    TextStringDict() : RefNameDict(true) { }
};


class PropNameDict : public RefNameDict {
public:
    PropNameDict() : RefNameDict(true) { }

};


class PropStringDict : public RefNameDict {
public:
    PropStringDict() : RefNameDict(false) { }
};




//======================================================================
//                              LayerNameDict
//======================================================================


// LayerNameDict -- dictionary for LayerName objects
// LayerName objects do not have reference-numbers and are not
// referenced by any other object.  All we need do is iterate over all
// the LayerName objects.

class LayerNameDict : public OasisDict {
    vector<LayerName*>  allNames;

public:
                LayerNameDict();
                ~LayerNameDict();
    LayerName*  add (const string& name, LayerName::IntervalType ivalType,
                     const Interval& layers, const Interval& types);

    typedef vector<LayerName*>::iterator        iterator;
    typedef vector<LayerName*>::const_iterator  const_iterator;

    iterator            begin()       { return allNames.begin(); }
    const_iterator      begin() const { return allNames.begin(); }
    iterator            end()         { return allNames.end();   }
    const_iterator      end()   const { return allNames.end();   }
};



//======================================================================
//                              XNameDict
//======================================================================

// XNameDict -- dictionary for XName objects
// This is more or less a subset of RefNameDict, with OasisName replaced
// by XName.  We don't need a stringToName map here because the names in
// XNAME records are not required to be unique.  We don't need a
// nameToRefnum map because the refnums are stored in the XName objects.

class XNameDict : public OasisDict {
    typedef  HashMap<Ulong, XName*>   RefnumToNameMap;

    bool        autoNumber;
    Ulong       nextRefnum;
    RefnumToNameMap     refnumToName;
    vector<XName*>      allNames;

public:
                XNameDict();
                ~XNameDict();
    XName*      lookupRefnum (Ulong refnum);
    XName*      add (const string& name);
    XName*      add (const string& name, Ulong refnum);

    typedef vector<XName*>::iterator            iterator;
    typedef vector<XName*>::const_iterator      const_iterator;

    iterator            begin()       { return allNames.begin(); }
    const_iterator      begin() const { return allNames.begin(); }
    iterator            end()         { return allNames.end();   }
    const_iterator      end()   const { return allNames.end();   }

private:
    XName*      doAdd (const string& str, Ulong refnum);
};



//========================================================================
//                              CellDict
//========================================================================

// CellDict -- map from CellName to Cell
// We cannot use the cell's name as the key because cells may be
// temporarily nameless: the CELL record may use a refnum that
// references a CELLNAME further down the file.

class CellDict {
    typedef HashMap<const CellName*, Cell*, HashPointer>  CellMap;

    CellMap              nameToCell;
    PointerVector<Cell>  allCells;

public:
                CellDict();
                ~CellDict();
    Cell*       lookup (const CellName*, bool create);
    bool        empty() const;

private:
                CellDict (const CellDict&);             // forbidden
    void        operator= (const CellDict&);            // forbidden
};


inline bool
CellDict::empty() const {
    return nameToCell.empty();
}


}  // namespace Oasis

#endif  // OASIS_DICTS_H_INCLUDED
