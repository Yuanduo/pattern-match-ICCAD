// oasis/names.h -- OASIS names and their property lists
//
// last modified:   31-Dec-2009  Thu  12:00
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// Classes defined here:
//
// PropValue            single property value -- a discriminated union
// PropValueVector      vector of all values in a single property
// Property             contents of a PROPERTY record
// PropertyList         list of properties for an OasisName
// OasisName            base class for contents of name records
// LayerName            OasisName subclass for LAYERNAME records
// XName                OasisName subclass for XNAME records
// Cell                 store cell offset and whether defined, for OasisParser


#ifndef OASIS_NAMES_H_INCLUDED
#define OASIS_NAMES_H_INCLUDED

#include <cassert>
#include <climits>
#include <functional>   // for unary_function
#include <string>
#include <vector>

#include "misc/ptrlist.h"
#include "misc/ptrvec.h"
#include "misc/utils.h"
#include "oasis.h"


namespace Oasis {

using std::string;
using std::vector;
using SoftJin::llong;
using SoftJin::Uint;
using SoftJin::Ullong;
using SoftJin::Ulong;
using SoftJin::PointerList;
using SoftJin::PointerVector;


// CELLNAME, TEXTSTRING, PROPNAME and PROPSTRING records all contain the
// same information: a name and property list.  So a single class,
// OasisName, is used for all four.  OasisName is defined later in this
// file.  We need these typedefs up here because properties contain
// references to PropNames and PropStrings.

class OasisName;                // defined below
typedef OasisName   CellName;
typedef OasisName   TextString;
typedef OasisName   PropName;
typedef OasisName   PropString;



//======================================================================
//                      PropValueType and PropValue
//======================================================================

// PropValueType -- type of value in a PropValue object
// The PV_Real_* values are specified in Table 3 of the spec and the
// remaining values in table 8.

enum PropValueType {
    PV_Real_PosInteger    = 0,
    PV_Real_NegInteger    = 1,
    PV_Real_PosReciprocal = 2,
    PV_Real_NegReciprocal = 3,
    PV_Real_PosRatio      = 4,
    PV_Real_NegRatio      = 5,
    PV_Real_Float32       = 6,
    PV_Real_Float64       = 7,
    PV_UnsignedInteger    = 8,
    PV_SignedInteger      = 9,
    PV_AsciiString        = 10,
    PV_BinaryString       = 11,
    PV_NameString         = 12,
    PV_Ref_AsciiString    = 13,
    PV_Ref_BinaryString   = 14,
    PV_Ref_NameString     = 15,

    PV_MaxType            = 15
};



// PropValue -- one value in a Property's vector of values.
// This class is essentially a discriminated union that stores a
// property value and its type.
//
// For real types (PV_Real_*), the type field may not match the actual
// representation in the Oreal.  For example, the scanner converts
// integer Oreals that are too large to handle into doubles.  Code that
// looks at the type field should treat all PV_Real_* values alike.
//
// For reals, strings, and PropStrings, PropValue holds pointers to
// objects allocated in the free store rather than the objects
// themselves.  This is because the corresponding classes have
// constructors and hence cannot be part of a union.  PropValue owns
// Oreal and string objects it points to , but _not_ any PropString
// object.  PropString objects created by OasisParser are owned by a
// PropStringDict.
//
// When the type is a reference (one of the PV_Ref_* enumerators), the
// value is normally a PropString*.  However, when OasisRecordReader
// creates a PropValue for a propstring-reference-number in a PROPERTY
// record, it cannot resolve the refnum into a PropString* because
// PropStrings belong to a higher layer.  So it stores the refnum itself
// in the PropValue.  OasisParser later replaces the refnum by a
// PropString*.
//
// The data member 'resolved' keeps track of whether a PV_Ref_* value
// is a refnum or a PropString*.  It is defined only when the type is a
// reference.  It is false if the reference's value is a refnum in the
// uival field and true when the value is a PropString*.


class PropValue {
    PropValueType  type;        // discriminant of union
    bool        resolved;       // true => refnum converted to PropString*
    union {
        Ullong  uival;
        llong   ival;
        Oreal*  rvalp;
        string* svalp;
        PropString*  psval;
    };

public:
                PropValue (const PropValue&);
                PropValue (PropValueType type, const Oreal& val);
                PropValue (PropValueType type, Ullong val);
                PropValue (PropValueType type, llong val);
                PropValue (PropValueType type, const string& val);
                PropValue (PropValueType type, PropString* val);
                ~PropValue();

    void        assign (PropValueType type, const Oreal& val);
    void        assign (PropValueType type, Ullong val);
    void        assign (PropValueType type, llong val);
    void        assign (PropValueType type, const string& val);
    void        assign (PropValueType type, PropString* val);

    PropValueType  getType() const;
    Ullong         getUnsignedIntegerValue() const;
    llong          getSignedIntegerValue() const;
    const Oreal&   getRealValue() const;
    const string&  getStringValue() const;
    PropString*    getPropStringValue() const;

    bool        isReal() const;
    bool        isString() const;
    bool        isReference() const;
    bool        isResolved() const;

    void        setPropString (PropString*);

    static bool typeIsReal (Ulong type);
    static bool typeIsString (Ulong type);
    static bool typeIsReference (Ulong type);
    static bool typeIsValid (Ulong type);

private:
    void        deleteOldValue();
    void        operator= (const PropValue&);   // forbidden for now
};



/*static*/ inline bool
PropValue::typeIsValid (Ulong type) {
    return (type <= PV_MaxType);
}


/*static*/ inline bool
PropValue::typeIsReal (Ulong type) {
    return (type <= PV_Real_Float64);
}


/*static*/ inline bool
PropValue::typeIsString (Ulong type) {
    return (type >= PV_AsciiString  &&  type <= PV_NameString);
}



/*static*/ inline bool
PropValue::typeIsReference (Ulong type) {
    return (type >= PV_Ref_AsciiString  &&  type <= PV_Ref_NameString);
}



inline PropValueType
PropValue::getType() const {
    return type;
}


inline bool
PropValue::isReal() const {
    return (typeIsReal(type));
}


inline bool
PropValue::isString() const {
    return (typeIsString(type));
}


inline bool
PropValue::isReference() const {
    return (typeIsReference(type));
}


inline bool
PropValue::isResolved() const {
    assert (isReference());
    return resolved;
}



inline
PropValue::PropValue (PropValueType type, const Oreal& val)
{
    assert (typeIsReal(type));
    this->type = type;
    rvalp = new Oreal(val);
}


inline
PropValue::PropValue (PropValueType type, Ullong val)
{
    // Only OasisRecordReader and AsciiRecordReader should call this
    // constructor with the `type' argument set to a reference type (one
    // of the PV_Ref_* enumerators).  In those classes, a reference
    // is still a reference-number, an unsigned-integer.  Hence this
    // constructor is the right one there.  OasisParser converts the
    // reference to a PropString*, so it and higher layers should use
    // the PropString* constructor for references.

    assert (type == PV_UnsignedInteger  ||  typeIsReference(type));
    this->type = type;
    resolved = false;   // false => reference type contains a Ulong
    uival = val;
}


inline
PropValue::PropValue (PropValueType type, llong val)
{
    assert (type == PV_SignedInteger);
    this->type = type;
    ival = val;
}


inline
PropValue::PropValue (PropValueType type, const string& val)
{
    assert (typeIsString(type));
    this->type = type;
    svalp = new string(val);
}


inline
PropValue::PropValue (PropValueType type, PropString* val)
{
    assert (typeIsReference(type));
    this->type = type;
    resolved = true;    // true => reference type contains a PropString*
    psval = val;
}



// Only OasisParser should invoke this for reference types.

inline Ullong
PropValue::getUnsignedIntegerValue() const {
    assert (type == PV_UnsignedInteger
            ||  (typeIsReference(type) && !resolved));
    return uival;
}


inline llong
PropValue::getSignedIntegerValue() const {
    assert (type == PV_SignedInteger);
    return ival;
}


inline const Oreal&
PropValue::getRealValue() const {
    assert (isReal());
    return *rvalp;
}


inline const string&
PropValue::getStringValue() const {
    assert (isString());
    return *svalp;
}


inline PropString*
PropValue::getPropStringValue() const {
    assert (isReference() && resolved);
    return psval;
}



// setPropString -- replace propstring-refnum by PropString*.
// This is intended only for OasisParser, which resolves the refnum
// that OasisRecordReader stored to get a PropString* and stores that.

inline void
PropValue::setPropString (PropString* val) {
    assert (isReference() && !resolved);
    psval = val;
    resolved = true;
}


bool  operator== (const PropValue& pva, const PropValue& pvb);

inline bool
operator!= (const PropValue& pva, const PropValue& pvb) {
    return (! (pva == pvb));
}


//======================================================================
//                            PropValueVector
//======================================================================

// PropValueVector is a vector of pointers to PropValue objects.  The
// vector owns the PropValue objects and will delete them when it is
// destroyed.  It is used in the Property class below, in PropertyRecord
// (rec-reader.h), and to store the last-value-list in ModalVars
// (modal-vars.h).  operator== is provided because OasisCreator needs to
// compare the current property values with the modal variable
// last-value-list.
//
// We use a vector instead of a list because random access is convenient,
// e.g., when accessing the values of S_BOUNDING_BOX.


typedef PointerVector<PropValue>  PropValueVector;

bool  operator== (const PropValueVector& pvva, const PropValueVector& pvvb);

inline bool
operator!= (const PropValueVector& pvva, const PropValueVector& pvvb) {
    return (! (pvva == pvvb));
}



//======================================================================
//                              Property
//======================================================================

// Property -- contents of an OASIS PROPERTY record
// For simplicity we always use a PropName object for the name, even
// when the PROPERTY record contains a propname-string.  The Property
// object is NOT the owner of the PropName it points to.  For Property
// objects that OasisParser creates, the PropName is owned by a
// PropNameDict or by OasisParser itself.
//
// Note that PropNames may temporarily be nameless while OasisParser
// is parsing a file.

class Property {
    PropName*   propName;       // property name
    bool        isStd;          // S bit from the PROPERTY record's info-byte
    PropValueVector  values;

public:
                Property (PropName* name, bool isStd);
                Property (const Property&);
                ~Property();
    Property&   operator= (const Property&);
    void        swap (Property&);

    void        addValue (PropValue* propval);
    void        addValue (PropValueType type, const Oreal& val);
    void        addValue (PropValueType type, Ullong val);
    void        addValue (PropValueType type, llong val);
    void        addValue (PropValueType type, const string& val);
    void        addValue (PropValueType type, PropString* val);

    bool        isStandard() const { return isStd;    }
    PropName*   getName() const    { return propName; }
    size_t      numValues() const  { return values.size(); }

    PropValue*              getValue (size_t n)       { return values[n]; }
    const PropValue*        getValue (size_t n) const { return values[n]; }
    PropValueVector&        getValueVector()          { return values;    }
    const PropValueVector&  getValueVector()    const { return values;    }

    // Provide abbreviations for getValueVector().begin() and
    // getValueVector().end().

    PropValueVector::iterator  beginValues() { return values.begin(); }
    PropValueVector::iterator  endValues()   { return values.end();   }

    PropValueVector::const_iterator
    beginValues() const {
        return values.begin();
    }
    PropValueVector::const_iterator
    endValues() const  {
        return values.end();
    }
};



// addValue -- append a value to the property's value vector
// The arg must be allocated from the free store.  The Property object
// becomes its owner.

inline void
Property::addValue (PropValue* propval) {
    values.push_back(propval);
}



//========================================================================
//                              PropertyList
//========================================================================

// PropertyList -- set of properties for an OasisName
// This class is a model of Forward Container.  The PropertyList owns
// the Property objects it points to and will delete them when it is
// destroyed.
//
// A PropertyList may contain several properties with the same name.
// For example, if a GDSII file that is converted to OASIS contains an
// element with several properties, the corresponding OASIS element will
// have several S_GDS_PROPERTY properties, each of which contains one
// GDSII propattr-propvalue pair.

class PropertyList : public PointerList<Property> {
public:
    void        addProperty (Property* prop);
    void        deleteProperties (const string& propname, bool isStd);
    Property*   getProperty (const string& propname, bool isStd);
};



// addProperty -- add a property to the property list and take ownership of it.

inline void
PropertyList::addProperty (Property* prop) {
    push_back(prop);
}


//======================================================================
//                              OasisName
//======================================================================

// OasisName and its two derived classes LayerName and XName store the
// contents of OASIS <name> records and their associated PROPERTY
// records.  OasisName is used for CELLNAME, TEXTSTRING, PROPNAME, and
// PROPSTRING records.
//
// The refnum associated with the name is stored elsewhere because
// OasisParser and OasisCreator may want to give different refnums for
// the same OasisName.  OasisParser uses OasisDict (dicts.h) to map
// OasisNames to refnums; OasisCreator uses RefNameTable (creator.cc).
//
// The refnums in CELLNAME, TEXTSTRING, PROPNAME, and PROPSTRING
// records are hidden from the application.  OasisParser provides no
// way of getting them and OasisCreator generates its own.
//
// OasisName becomes the owner of all properties added.


class OasisName {
    enum {
        NameBit = 0x01,         // object has a name
    };

    int           flags;
    string        name;
    PropertyList  plist;        // contents of PROPERTY records

public:
                OasisName();
    explicit    OasisName (const string& n);
                ~OasisName();

    bool        hasName() const;
    void        setName (const string& s);
    const string& getName() const;

    Property*   getProperty (const string& propname, bool isStd);
    void        addProperty (Property* prop);
    void        deleteProperties (const string& propname, bool isStd);
    bool        hasProperties() const;

    const PropertyList&  getPropertyList() const  { return plist; }

    // The following two functions are intended only for ParserImpl.  It
    // is dangerous for applications to use them because there may be
    // more than one PropName object for a given property name.

    void        removeProperties (const PropName* propName, bool isStd);
    Property*   getProperty (const PropName* propName, bool isStd);

protected:
    void        setBit (int bit)        { flags |= bit;         }
    bool        testBit (int bit) const { return (flags & bit); }

private:
                OasisName (const OasisName&);           // forbidden
    void        operator= (const OasisName&);           // forbidden
};



// OasisName constructor (overloaded)
// OasisParser uses this constructor, for a temporarily-nameless name,
// when it encounters a forward reference, e.g., a PROPERTY record where
// the property name is given by a reference-number that has not yet
// been seen in a PROPNAME record.  It fills in the name later.
//
// Do not use this constructor from application code.

inline
OasisName::OasisName() {
    flags = 0;
}


// OasisName constructor (overloaded)
// This is the normal constructor, used when the name is available.

inline
OasisName::OasisName (const string& s) : name(s) {
    flags = NameBit;
}


inline
OasisName::~OasisName() { }


// Methods to get and set the name.  The name can be set only for an
// anonymous OasisName.  This implies that once the name is set it
// cannot be changed.

inline bool
OasisName::hasName() const {
    return (testBit(NameBit));
}


inline void
OasisName::setName (const string& s) {
    assert (! hasName());
    name = s;
    setBit(NameBit);
}


inline const string&
OasisName::getName() const {
    assert (hasName());
    return name;
}


inline Property*
OasisName::getProperty (const string& pname, bool isStd) {
    return (plist.getProperty(pname, isStd));
}


inline void
OasisName::addProperty (Property* prop) {
    plist.addProperty(prop);
}


inline void
OasisName::deleteProperties (const string& pname, bool isStd) {
    plist.deleteProperties(pname, isStd);
}


inline bool
OasisName::hasProperties() const {
    return (! plist.empty());
}



//======================================================================
//                              LayerName
//======================================================================


// Interval -- intervals of integers, bounded or semi-infinite
// This is used for the intervals of LAYERNAME records.
// The interval includes both bounds.
// Invariant:
//      unbounded => (upper == ULONG_MAX)

class Interval {
    Ulong       lower, upper;   // lower and upper bounds
    bool        unbounded;      // true => upper bound is really infinite

public:
                Interval (Ulong lower, Ulong upper) {
                    this->lower = lower;
                    this->upper = upper;
                    unbounded = false;
                }
    explicit    Interval (Ulong lower) {
                    this->lower = lower;
                    this->upper = ULONG_MAX;
                    unbounded = true;
                }

    // The compiler-generated copy constructor and copy assignment operator
    // are fine.

    bool        empty()         const { return (lower > upper); }
    bool        isUnbounded()   const { return unbounded; }
    Ulong       getLowerBound() const { return lower;     }
    Ulong       getUpperBound() const { return upper;     }
};



// LayerName -- name describing groups of layers and/or datatypes.
// Each LayerName object holds the information in one LAYERNAME
// record.  Spec paragraph 19.5 says:
//
//      LAYERNAME records may be repeated for the same layer name.  The
//      complete mapping for a layer name is formed by the union of all
//      layer, datatype, textlayer, and texttype ranges associated with
//      the name.
//
// However, we do not merge the information in different LAYERNAME
// records with the same name because we do not know what meaning the
// application attaches to the properties of each LAYERNAME record.

class LayerName : public OasisName {
public:
    enum IntervalType {
        GeometryInterval,       // layer-interval, datatype-interval
        TextInterval            // textlayer-interval, texttype-interval
    };

private:
    IntervalType  ivalType;
    Interval      layers;       // layer-interval or textlayer-interval
    Interval      types;        // datatype-interval or texttype-interval

public:
                LayerName (const string& name, IntervalType ivalType,
                           const Interval& layers, const Interval& types);
                ~LayerName() { }

    const Interval&  getLayers() const       { return layers;   }
    const Interval&  getTypes()  const       { return types;    }
    IntervalType     getIntervalType() const { return ivalType; }
};



//======================================================================
//                              XName
//======================================================================

// XName -- contents of XNAME record
//
// For XNames we store the refnum associated with the name in the XName
// object itself, rather than in the dictionary as with the other kinds
// of names.  This is because XNames have no predefined semantics; they
// are meant for application-specific extensions.  The Oasis library
// therefore does not hide these refnums from the application.
// OasisParser passes the XNames to the application with the refnums,
// and OasisCreator expects the application to provide the refnums with
// the XNames.

class XName : public OasisName {
    Ulong       refnum;
    Ulong       attribute;

public:
    XName (const string& name, Ulong refnum) : OasisName(name) {
        this->refnum = refnum;
    }

    ~XName() { }

    Ulong       getRefnum()    const      { return refnum;    }
    Ulong       getAttribute() const      { return attribute; }
    void        setAttribute (Ulong attr) { attribute = attr; }
};



//========================================================================
//                           MatchPropertyName
//========================================================================

// MatchPropertyName -- unary predicate to look for a particular property.
// An instance of this can be used with std::find_if() to search a
// PropertyList.  The constructor arguments are the property's name and
// whether the property is a standard one (the S bit in the PROPERTY
// record's info-byte).  For example,
//
//     PropertyList::iterator  iter =
//             find_if(proplist.begin(), proplist.end(),
//                     MatchPropertyName("S_CELL_OFFSET", true));
//     if (iter != proplist.end())
//         ...

class MatchPropertyName : public std::unary_function<Property*, bool> {
    const string&  name;
    bool           isStd;
public:
    MatchPropertyName (const string& n, bool isStd) : name(n) {
        this->isStd = isStd;
    }

    bool
    operator() (const Property* prop) const {
        PropName*  propName = prop->getName();
        return (prop->isStandard() == isStd
                &&  propName->hasName()
                &&  propName->getName() == name);
    }
};



//========================================================================
//                              Cell
//========================================================================

// Cell -- store information about a cell
// For now, all we store is whether the cell is defined (i.e.,
// OasisParser has seen a CELL record for it) and the CELL record's
// offset, either from the S_CELL_OFFSET property of the corresponding
// CELLNAME record or from the CELL record itself.
//
// We don't keep this info in the CellName class itself because when an
// OASIS file is read, filtered, and written, a single CellName object
// may be used by both OasisParser and OasisCreator.  The cell
// referenced, however, will have different offsets in the input and
// output files.

class Cell {
    enum {
        CellDefinedBit = 0x1,   // CELL record seen
        HaveOffsetBit  = 0x2    // offset of CELL record known
    };
    int         flags;
    off_t       fileOffset;

public:
                Cell();
    void        setOffsetFromCell (off_t offset);
    void        setOffsetFromProperty (off_t offset);

    bool        isDefined() const;
    bool        haveOffset() const;
    off_t       getOffset() const;
};


inline
Cell::Cell() {
    flags = 0;
    fileOffset = 0;
}



// setOffsetFromCell -- note actual file offset of CELL record.
// The parser calls this when it parses the CELL record.  By saying that
// the offset comes from the CELL record we do two things: record the
// cell as being defined, and prevent the offset from being overwritten
// by the value of the S_CELL_OFFSET property of a CELLNAME record for
// the cell.  There is no point in using the property value when we have
// the actual offset.

inline void
Cell::setOffsetFromCell (off_t offset)
{
    assert (offset > 0);
    fileOffset = offset;
    flags |= (CellDefinedBit | HaveOffsetBit);
}



// setOffsetFromProperty -- set file offset of cell from S_CELL_OFFSET.

inline void
Cell::setOffsetFromProperty (off_t offset)
{
    // If we already have the CELL record's actual offset, ignore the
    // CELLNAME record's S_CELL_OFFSET property.  Note that this
    // offset can be 0, meaning that the cell is external.

    if (! (flags & CellDefinedBit)) {
        fileOffset = offset;
        flags |= HaveOffsetBit;
    }
}



inline bool
Cell::isDefined() const {
    return (flags & CellDefinedBit);
}


inline bool
Cell::haveOffset() const {
    return (flags & HaveOffsetBit);
}


inline off_t
Cell::getOffset() const {
    assert (haveOffset());
    return fileOffset;
}


}  // namespace Oasis

#endif  // OASIS_NAMES_H_INCLUDED
