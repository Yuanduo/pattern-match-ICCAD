// oasis/names.cc --  OASIS names and their property lists
//
// last modified:   28-Dec-2009  Mon  19:47
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <algorithm>            // for swap()
#include "names.h"


namespace Oasis {

using namespace std;
using namespace SoftJin;


//========================================================================
//                              PropValue
//========================================================================

PropValue::PropValue (const PropValue& propval)
{
    type = propval.type;
    switch (type) {
        case PV_Real_PosInteger:
        case PV_Real_NegInteger:
        case PV_Real_PosReciprocal:
        case PV_Real_NegReciprocal:
        case PV_Real_PosRatio:
        case PV_Real_NegRatio:
        case PV_Real_Float32:
        case PV_Real_Float64:
            rvalp = new Oreal(*propval.rvalp);
            break;
        case PV_UnsignedInteger:
            uival = propval.uival;
            break;
        case PV_SignedInteger:
            ival = propval.ival;
            break;
        case PV_AsciiString:
        case PV_BinaryString:
        case PV_NameString:
            svalp = new string(*propval.svalp);
            break;
        case PV_Ref_AsciiString:
        case PV_Ref_BinaryString:
        case PV_Ref_NameString:
            resolved = propval.resolved;
            if (resolved)
                psval = propval.psval;
            else
                uival = propval.uival;
            break;
        default:
            assert (false);
    }
}



// deleteOldValue -- delete objects allocated by the PropValue
// If the caller is not the destructor it should set the type and
// value fields immediately after, to ensure that the pointers to
// the deleted objects are not dereferenced.

inline void
PropValue::deleteOldValue()
{
    if (isReal())
        delete rvalp;
    else if (isString())
        delete svalp;

    // For reference types the PropString object pointed to is owned by
    // something else, typically a PropStringDict.
}



PropValue::~PropValue() {
    deleteOldValue();
}



// The overloaded assign() methods below replace the current contents of
// the PropValue.  All have the same pattern: delete the current value
// if it needs deleting, and then copy the new value.  If both the
// current and new values are Oreals, or both are strings, we can just
// copy the new value over the old instead of deleting and reallocating.


void
PropValue::assign (PropValueType type, const Oreal& val)
{
    assert (typeIsReal(type));

    if (isReal())
        *rvalp = val;
    else {
        deleteOldValue();
        this->type = type;
        rvalp = new Oreal(val);
    }
}



void
PropValue::assign (PropValueType type, Ullong val)
{
    assert (type == PV_UnsignedInteger);

    deleteOldValue();
    this->type = type;
    uival = val;
}



void
PropValue::assign (PropValueType type, llong val)
{
    assert (type == PV_SignedInteger);

    deleteOldValue();
    this->type = type;
    ival = val;
}



void
PropValue::assign (PropValueType type, const string& val)
{
    assert (typeIsString(type));

    if (isString())
        *svalp = val;
    else {
        deleteOldValue();
        this->type = type;
        svalp = new string(val);
    }
}



void
PropValue::assign (PropValueType type, PropString* val)
{
    assert (typeIsReference(type));

    deleteOldValue();
    this->type = type;
    resolved = true;    // true => reference type contains a PropString*
    psval = val;
}



bool
operator== (const PropValue& pva, const PropValue& pvb)
{
    // For real types, the type in the PropValue is irrelevant because
    // it may not match the Oreal's storage representation.  So don't
    // check that the PropValueTypes are the same; just compare the
    // Oreal values.

    if (pva.isReal()  &&  pvb.isReal())
        return (pva.getRealValue().isIdenticalTo(pvb.getRealValue()));

    // Otherwise the types have to match.  An AsciiString is not equal
    // to a NameString even if the strings are identical.

    if (pva.getType() != pvb.getType())
        return false;

    // Compare the values.  For reference types the two PropValues must
    // point to the same PropString object; it is not enough for the
    // strings in the PropStrings to be identical.  This is because one
    // PropString may later have properties added.

    switch (pva.getType()) {
        case PV_UnsignedInteger:
            return (pva.getUnsignedIntegerValue()
                        == pvb.getUnsignedIntegerValue());

        case PV_SignedInteger:
            return (pva.getSignedIntegerValue()
                        == pvb.getSignedIntegerValue());

        case PV_AsciiString:
        case PV_BinaryString:
        case PV_NameString:
            return (pva.getStringValue() == pvb.getStringValue());

        case PV_Ref_AsciiString:
        case PV_Ref_BinaryString:
        case PV_Ref_NameString:
            if (pva.isResolved() != pvb.isResolved())
                return false;
            if (pva.isResolved())
                return (pva.getPropStringValue() == pvb.getPropStringValue());
            else
                return (pva.getUnsignedIntegerValue()
                            == pvb.getUnsignedIntegerValue());

        default:                // make gcc happy
            assert (false);
            return false;
    }
}



//======================================================================
//                            PropValueVector
//======================================================================


bool
operator== (const PropValueVector& pvva, const PropValueVector& pvvb)
{
    size_t  nelems = pvva.size();
    if (nelems != pvvb.size())
        return false;

    for (size_t j = 0;  j < nelems;  ++j)
        if (*pvva[j] != *pvvb[j])
            return false;
    return true;
}



//======================================================================
//                              Property
//======================================================================

// Property constructor
//   propName   property name.  The caller retains ownership of the
//              PropName object, but must obviously not delete it while
//              this object is in use.
//   isStd      whether the property is a standard one.  The value of
//              the S bit in the PROPERTY record's info-byte.

Property::Property (PropName* propName, bool isStd)
{
    this->propName = propName;
    this->isStd = isStd;
}



Property::Property (const Property& prop)
  : values(prop.values)
{
    propName = prop.propName;
    isStd = prop.isStd;
}


Property::~Property() { }


// Convenience functions to create a PropValue of a given type and
// add it to the Property.


void
Property::addValue (PropValueType type, const Oreal& val)
{
    auto_ptr<PropValue>  apv(new PropValue(type, val));
    addValue(apv.get());
    apv.release();      // Property now owns value
}



void
Property::addValue (PropValueType type, Ullong val)
{
    auto_ptr<PropValue>  apv(new PropValue(type, val));
    addValue(apv.get());
    apv.release();
}



void
Property::addValue (PropValueType type, llong val)
{
    auto_ptr<PropValue>  apv(new PropValue(type, val));
    addValue(apv.get());
    apv.release();
}



void
Property::addValue (PropValueType type, const string& val)
{
    auto_ptr<PropValue>  apv(new PropValue(type, val));
    addValue(apv.get());
    apv.release();
}



void
Property::addValue (PropValueType type, PropString* val)
{
    auto_ptr<PropValue>  apv(new PropValue(type, val));
    addValue(apv.get());
    apv.release();
}



Property&
Property::operator= (const Property& prop)
{
    Property  temp(prop);
    temp.swap(*this);
    return *this;
}



// swap -- swap the contents of this Property object with another's
// About the only use this has is to implement operator=().

void
Property::swap (Property& prop)
{
    std::swap(propName, prop.propName);
    std::swap(isStd, prop.isStd);
    values.swap(prop.values);
}



//========================================================================
//                              PropertyList
//========================================================================


// getProperty -- locate a standard/nonstandard Property with given name.
// If several properties match, any one may be returned.
// Returns Null if there is no such property.
// XXX: should have an iterator for matching properties

Property*
PropertyList::getProperty (const string& name, bool isStd)
{
    iterator  iter = find_if(begin(), end(), MatchPropertyName(name, isStd));
    return (iter != end() ? *iter : Null);
}



// deleteProperties -- delete all properties with given name and standardness.
// Does nothing if no property matches.

void
PropertyList::deleteProperties (const string& name, bool isStd)
{
    deleteIf(MatchPropertyName(name, isStd));
}



// The same functions, but with the property identified by a PropName*
// rather than a name string.  These two functions are intended to be
// used only by ParserImpl.  For applications it is dangerous to locate
// a property by comparing PropName pointers because OasisParser may
// create more than one PropName object for a given name.  This can
// happen even if the propname table is strict if the parser option
// strictConformance is false.


Property*
OasisName::getProperty (const PropName* propName, bool isStd)
{
    PropertyList::iterator  iter = plist.begin();
    PropertyList::iterator  end  = plist.end();
    for ( ;  iter != end;  ++iter) {
        Property*  prop = *iter;
        if (isStd == prop->isStandard()  &&  prop->getName() == propName)
            return prop;
    }
    return Null;
}



void
OasisName::removeProperties (const PropName* propName, bool isStd)
{
    PropertyList::iterator  iter = plist.begin();
    PropertyList::iterator  end  = plist.end();
    while (iter != end) {
        Property*  prop = *iter;
        if (isStd == prop->isStandard()  &&  prop->getName() == propName)
            iter = plist.erase(iter);
        else
            ++iter;
    }
}



//======================================================================
//                              LayerName
//======================================================================

// LayerName constructor
//   name       name of this group of layers/types
//   ivalType   GeometryInterval if this object is being created for a
//              RID_LAYERNAME_GEOMETRY record; TextInterval if it is
//              being created for a RID_LAYERNAME_TEXT record.
//   layer      the range of layers or textlayers
//   types      the range of datatypes or texttypes

LayerName::LayerName (const string& name, IntervalType ivalType,
                      const Interval& layers, const Interval& types)
  : OasisName(name),
    layers(layers),
    types(types)
{
    this->ivalType = ivalType;
}


}  // namespace Oasis
