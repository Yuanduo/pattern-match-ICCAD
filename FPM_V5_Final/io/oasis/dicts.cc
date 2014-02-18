// oasis/dicts.cc -- dictionaries for names and cells
//
// last modified:   01-Jan-2010  Fri  22:02
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <memory>
#include "dicts.h"


namespace Oasis {

using namespace std;
using namespace SoftJin;


//======================================================================
//                              RefNameDict
//======================================================================

// RefnameDict constructor
//   uniqueNames        true => names must be unique
// Cellnames and propnames must be unique; text-strings and prop-strings
// need not be.

RefNameDict::RefNameDict (bool uniqueNames)
{
    autoNumber = false;
    explicitNumber = false;
    this->uniqueNames = uniqueNames;
    nextRefnum = 0;
}



RefNameDict::~RefNameDict()
{
    DeleteContainerElements(allNames);
}



// add -- create OasisName with given name and automatically-assigned refnum.
// The reference-number assigned is 1 + the last number assigned.
// This function and the other add() below are mutually exclusive.  For
// any dictionary only one of them must be called.

OasisName*
RefNameDict::add (const string& name)
{
    assert (! explicitNumber);
    autoNumber = true;

    // Use up the current refnum only when the name has been
    // successfully added.  Note that there is no danger of wraparound
    // in ++nextRefnum.  We will run out of memory first.

    OasisName*  oname = doAdd(name, nextRefnum);
    if (oname != Null)
        ++nextRefnum;
    return oname;
}



// add -- create OasisName with given name and refnum

OasisName*
RefNameDict::add (const string& name, Ulong refnum)
{
    assert (! autoNumber);
    explicitNumber = true;

    return (doAdd(name, refnum));
}



// lookupName -- locate OasisName with given name, optionally creating it
//   name       name to look up
//   create     if this is true and the dictionary has no OasisName with
//              the given name, a new one is created and returned
//
// Precondition:
//   uniqueNames is true

OasisName*
RefNameDict::lookupName (const string& name, bool create)
{
    assert (uniqueNames);

    StringToNameMap::iterator  iter = stringToName.find(&name);
    if (iter != stringToName.end())
        return iter->second;
    if (! create)
        return Null;

    // The vector allNames is the container that owns all the OasisName
    // objects allocated because that is what the destructor iterates
    // over.  The new OasisName should be owned by the auto_ptr until it
    // is safely in allNames.  It should be removed from the auto_ptr
    // before being inserted in stringToName because there shouldn't be
    // two owners if operator[] throws an exception.
    //
    // Note that the key (a const string*) in the map stringToName should
    // point to the string in the OasisName to ensure that it lives as
    // long as the OasisName.

    auto_ptr<OasisName>  aon(new OasisName(name));
    allNames.push_back(aon.get());
    OasisName*  oname = aon.release();
    stringToName[&oname->getName()] = oname;
    return oname;
}



// lookupRefnum -- locate OasisName with given name, optionally creating it
//   refnum     reference-number to look up
//   create     if this is true and the dictionary has no OasisName with
//              the given refnum, a new one is created and returned
// The OasisName created is nameless; the name should be filled in later.

OasisName*
RefNameDict::lookupRefnum (Ulong refnum, bool create)
{
    RefnumToNameMap::iterator  iter = refnumToName.find(refnum);
    if (iter != refnumToName.end())
        return iter->second;
    if (! create)
        return Null;

    auto_ptr<OasisName>  aon(new OasisName);
    allNames.push_back(aon.get());
    OasisName*  oname = aon.release();
    refnumToName[refnum] = oname;
    nameToRefnum[oname] = refnum;
    return oname;
}



// doAdd -- add an OasisName and refnum to the dictionary.
//
// If an OasisName with the given name and refnum already exists, that
// is returned.  If an OasisName with the given name exists but without
// any refnum, its refnum is set to the argument.  Similarly if an
// OasisName exists with the same refnum but without any name, its name
// is set to the argument.  Otherwise a new OasisName is created.
//
// Returns Null if an OasisName already exists with the given name but
// a different refnum, or (if uniqueNames is true) with the given refnum
// but a different name.

OasisName*
RefNameDict::doAdd (const string& name, Ulong refnum)
{
    OasisName*  oname;

    // See if an OasisName with the given refnum already exists in the
    // dictionary.  If it does and has the same name, simply return it
    // because in OASIS it is not an error for two name records to have
    // the same name and refnum.  If the name is different, it is an
    // error because refnums must be unique.
    //
    // If the OasisName we found does not have a name at all, it was
    // created because of a forward reference to the refnum.  So fill in
    // the name we now have.  If names are required to be unique, first
    // verify that the new name is not already in use.
    //
    // A forward reference occurs e.g. when a PROPERTY record referring
    // to a PROPSTRING reference number appears before the corresponding
    // PROPSTRING record.

    if ((oname = lookupRefnum(refnum, false)) != Null) {
        if (oname->hasName())
            return (oname->getName() == name ? oname : Null);
        if (uniqueNames  &&  lookupName(name, false) != Null)
            return Null;        // new name already in use
        oname->setName(name);
        if (uniqueNames)
            stringToName[&oname->getName()] = oname;
        return oname;
    }

    // If names are unique, see if an OasisName with the given name
    // already exists.

    if (uniqueNames  &&  (oname = lookupName(name, false)) != Null) {
        // If oname already has a refnum, it must be different from what
        // we have now because the lookup for the new refnum failed
        // above.  So return Null to indicate a collision.
        //
        if (nameToRefnum.find(oname) != nameToRefnum.end())
            return Null;

        // Set oname's refnum to the given value.
        nameToRefnum[oname] = refnum;
        refnumToName[refnum] = oname;
        return oname;
    }

    // The dict contains no OasisName with this refnum.  Create a new
    // OasisName and add it to all hash maps.

    auto_ptr<OasisName>  aon(new OasisName(name));
    allNames.push_back(aon.get());
    oname = aon.release();
    if (uniqueNames)
        stringToName[&oname->getName()] = oname;
    nameToRefnum[oname] = refnum;
    refnumToName[refnum] = oname;
    return oname;
}



// getRefnum -- get the reference-number for an OasisName, if any.
//   oname      the OasisName to look up
//   refnump    out: the refnum for oname is stored here
//
// If oname has been added to the dictionary and has a refnum,
// getRefnum() stores that refnum in *refnump and returns true.
// Otherwise it returns false.
//
// Every OasisName in the dictionary has a refnum, with one exception:
// A CellName created for a cellname-string in a CELL record.  If a
// CELLNAME record with the same name is later seen, the CellName will
// be assigned a refnum.  If there is no CELLNAME with the same name
// the CellName object will never have a refnum.
//
// All other OasisName objects have refnums because they get added only
// when a <name> record is seen.

bool
RefNameDict::getRefnum (OasisName* oname, /*out*/ Ulong* refnump) const
{
    NameToRefnumMap::const_iterator  iter = nameToRefnum.find(oname);
    if (iter == nameToRefnum.end())
        return false;

    *refnump = iter->second;
    return true;
}



//======================================================================
//                              LayerNameDict
//======================================================================

LayerNameDict::LayerNameDict() { }

LayerNameDict::~LayerNameDict()  {
    DeleteContainerElements(allNames);
}



// add -- create a new LayerName object and add it to the dictionary
//   name       name of the layer ranges
//   ivalType   specifies whether the next two args are layer and datatype
//              ranges or textlayer and texttype ranges
//   layers     range of layers or textlayers
//   types      range of datatypes or texttypes
// Returns a pointer to the new LayerName object.

LayerName*
LayerNameDict::add (const string& name, LayerName::IntervalType ivalType,
                    const Interval& layers, const Interval& types)
{
    auto_ptr<LayerName> aln(new LayerName(name, ivalType, layers, types));
    allNames.push_back(aln.get());
    return aln.release();
}




//======================================================================
//                              XNameDict
//======================================================================

// The XNameDict code is similar to the RefNameDict code.
// Repeating the code with the changes required for XName looks the
// simplest way of handling things -- simpler than using templates
// or adding virtual methods to OasisDict.


XNameDict::XNameDict() {
    autoNumber = false;
    nextRefnum = 0;
}



XNameDict::~XNameDict()  {
    DeleteContainerElements(allNames);
}



XName*
XNameDict::add (const string& name)
{
    assert (autoNumber  ||  allNames.size() == 0);
    autoNumber = true;

    XName*  xname = doAdd(name, nextRefnum);
    if (xname != Null)
        ++nextRefnum;
    return xname;
}



XName*
XNameDict::add (const string& name, Ulong refnum)
{
    assert (!autoNumber  ||  allNames.size() == 0);
    autoNumber = false;

    return (doAdd(name, refnum));
}



// lookupRefnum -- see if XName with given refnum exists in dictionary
// Unlike RefNameDict::lookupRefnum(), this has no `create' flag because
// XNames are not referenced by any of the normal records.  We don't
// have to worry about forward references.

XName*
XNameDict::lookupRefnum (Ulong refnum)
{
    RefnumToNameMap::iterator  iter = refnumToName.find(refnum);
    return (iter != refnumToName.end() ? iter->second : Null);
}



// doAdd -- create XName and add to dictionary.
// Normally creates a new XName object with the given name and refnum,
// adds it to the dictionary, and returns a pointer to it.
// If an XName object already exists in the dictionary with the same
// name and refnum, doAdd() simply returns a pointer to the old object.
// If an XName object already exists in the dictionary with the same
// refnum but a different name, doAdd() returns Null.

XName*
XNameDict::doAdd (const string& name, Ulong refnum)
{
    XName*      xname;

    // If the given refnum is already in use, return the old object if
    // the name also matches, otherwise return Null.

    if ((xname = lookupRefnum(refnum)) != Null)
        return (xname->getName() == name ? xname : Null);

    // The dict contains no XName with this refnum.  Create a new XName
    // and add it to the hash map.

    auto_ptr<XName>  axn(new XName(name, refnum));
    allNames.push_back(axn.get());
    xname = axn.release();
    refnumToName[refnum] = xname;
    return xname;
}



//========================================================================
//                              CellDict
//========================================================================

CellDict::CellDict()  { }
CellDict::~CellDict() { }


// lookup -- locate Cell given the corresponding CellName
//   cellName   key to look up in map
//   create     specifies what to do if no Cell is associated with cellName.
//              If true, a new Cell is created, added to the map and returned.
//              If false, Null is returned.

Cell*
CellDict::lookup (const CellName* cellName, bool create)
{
    CellMap::iterator  iter = nameToCell.find(cellName);
    if (iter != nameToCell.end())
        return iter->second;
    if (! create)
        return Null;

    allCells.push_back(Null);
    Cell*  cell = new Cell;
    allCells.back() = cell;

    nameToCell[cellName] = cell;
    return cell;
}


}  // namespace Oasis
