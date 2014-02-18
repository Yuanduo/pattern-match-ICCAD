// conv/gds-oasis.cc -- translate GDSII file to OASIS
//
// last modified:   01-Jan-2010  Fri  21:57
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// ConvertGdsToOasis(), the external interface of this file, is at the
// bottom of the file.  It creates a GdsParser object and invokes its
// parseFile() with an instance of the class GdsToOasisConverter as the
// builder.  GdsToOasisConverter does most of the conversion.  It uses
// OasisCreator to construct the OASIS file.
//
// GdsToOasisConverter converts the GDSII elements in batches.  It reads
// in a batch, converts them, and writes them out to the OASIS file.  By
// buffering the elements instead of writing them out immediately it can
// use two techniques to make the OASIS file more compact.
//
//   - It can reorder the elements before writing to take advantage
//     of the OASIS modal variables.
//
//   - It can use the repetition field in OASIS elements to merge
//     identical GDSII elements at different positions.


#include <algorithm>            // for min(), max(), sort(), swap()
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>              // for abs()
#include <memory>
#include <list>
#include <vector>

#include "port/compiler.h"
#include "port/hash-table.h"
#include "misc/arith.h"
#include "misc/ptrvec.h"
#include "misc/utils.h"
#include "gdsii/builder.h"
#include "gdsii/glimits.h"
#include "gdsii/parser.h"
#include "oasis/creator.h"
#include "oasis/names.h"
#include "oasis/oasis.h"

#include "gds-oasis.h"
#include "ptgroup.h"


namespace GdsiiOasis {

using namespace std;
using namespace SoftJin;
using namespace Gdsii;
using namespace Oasis;


const char   OasisVersion[] = "1.0";
    // Which version of OASIS file we generate.


const Uint   MaxBufferedElements = 300*1000;
    // The number of GDSII elements to accumulate before writing them to
    // the OASIS file.  Increasing this value will probably make the
    // OASIS file more compact by increasing the chances of getting a
    // good ordering, but it will also increase the memory used and the
    // time taken to sort the elements.
    //
    // If immediateNames is true in the options passed to the
    // constructor, this limit is ignored and all the elements in a cell
    // are buffered before anything is written.  That is because name
    // records may have to be written for text strings and property
    // values, and those records are not allowed within a cell.
    //
    // XXX: Might be better to have a per-element-type limit and flush
    // only elements of the type that reaches the limit.
    //
    // XXX: Multiply limit by (optLevel+1) ?
    // That is, buffer more elements with increasing optimization level.


// GdsToOasisConverter stores all the Oasis CellName, TextString, and
// PropString objects it creates in vectors.  The element classes below
// use the index in the vector to reference those objects instead of
// using pointers to the objects.  This saves space on 64-bit systems
// because we can use a 32-bit index instead of a 64-bit pointer.

typedef Uint    CellNameIndex;
typedef Uint    TextStringIndex;
typedef Uint    PropStringIndex;



template <typename IntType>
inline bool
IsOdd (IntType n) {
    return (n & 0x1);
}



//----------------------------------------------------------------------
//                      PropInfo and PropInfoList
//----------------------------------------------------------------------

// Elements in GDSII can have properties.  Each property is identified
// by an integer propattr in the range 1..127 and has a string value.
// The properties of each element should have distinct propattrs.  We
// need to store these properties with the element data when we buffer
// the elements, and for that we have the classes PropInfo and
// PropInfoList.  Every element has a PropInfoList member.
//
// Elements are stored temporarily in hash tables so that identical
// elements can be merged.  For hashing we need to generate a hash value
// from the contents of an element and compare elements for equality.
// Therefore we provide these methods in PropInfo and PropInfoList:
//
//     getHash()     to compute a hash of all the properties
//     operator==()  to compare two PropInfoLists
//
// We also provide operator!=() for completeness.


// PropInfo -- GDSII property
// GdsToOasisConverter creates a PropString object for each unique GDSII
// property value.  The 'value' member here is the index of the
// PropString in its allPropStrings vector.

struct PropInfo {
    int              attr;      // PROPATTR
    PropStringIndex  value;     // PROPVALUE

    PropInfo() { }
    PropInfo (int attr, PropStringIndex value) {
        this->attr = attr;
        this->value = value;
    }

    size_t      getHash() const;

    bool        operator== (const PropInfo& rhs) const {
                    return (attr == rhs.attr  &&  value == rhs.value);
                }
    bool        operator!= (const PropInfo& rhs) const {
                    return (! (*this == rhs));
                }
};



size_t
PropInfo::getHash() const
{
    // Multiply the fields by primes to try to spread out the hash value.

    return (647 * static_cast<size_t>(attr)
            + 179 * static_cast<size_t>(value));
}



// PropInfoList -- list of GDSII properties
// The list is kept sorted in increasing order of propattr.  We can
// therefore use list<T>::operator==() to see if two property lists are
// the same.  This is a list and not a vector because lists use less
// space when empty, and most elements are expected to have no
// properties.

class PropInfoList {
    list<PropInfo>  plist;
public:
    void        addProperty (int attr, PropStringIndex val);

    typedef list<PropInfo>::iterator           iterator;
    typedef list<PropInfo>::const_iterator     const_iterator;

    const_iterator      begin() const  { return plist.begin(); }
    iterator            begin()        { return plist.begin(); }
    const_iterator      end()   const  { return plist.end();   }
    iterator            end()          { return plist.end();   }

    size_t      getHash() const;

    bool        operator== (const PropInfoList& rhs) const {
                    return (plist == rhs.plist);
                }
    bool        operator!= (const PropInfoList& rhs) const {
                    return (! (*this == rhs));
                }
};



// addProperty -- add property attr/value pair to GDSII property list

void
PropInfoList::addProperty (int attr, PropStringIndex value)
{
    // Keep the list sorted by attr when inserting the new property.

    iterator  iter    = plist.begin();
    iterator  endIter = plist.end();
    while (iter != endIter  &&  iter->attr < attr)
        ++iter;
    plist.insert(iter, PropInfo(attr, value));
}



// getHash -- compute hash of all the properties in the list

size_t
PropInfoList::getHash() const
{
    size_t  hash = 0;
    const_iterator  iter    = plist.begin();
    const_iterator  endIter = plist.end();
    for ( ;  iter != endIter;  ++iter)
        hash += iter->getHash();

    return hash;
}



//----------------------------------------------------------------------
//                           TransformInfo
//----------------------------------------------------------------------

// TransformInfo -- GDSII transform
// SREFs, AREFs, and TEXTs may be transformed (rotated/magnified/flipped)
// in GDSII.
//
// To avoid using 8 bytes to store 1 bit, we use a negative mag to
// indicate that the reflectX (flip) bit is set.

class TransformInfo {
    double      mag;            // negative => isFlipped
    double      angle;

public:
    void        init (double mag, double angle, bool flip) {
                    this->mag = (flip ? -mag : mag);
                    this->angle = angle;
                }
    double      getMag()    const { return fabs(mag);   }
    double      getAngle()  const { return angle;       }
    bool        isFlipped() const { return (mag < 0.0); }

    // As with PropInfo and PropInfoList, provide getHash() and
    // operators == and != so that element managers can compare elements
    // that contain transforms and insert them into HashSets.

    size_t      getHash() const {
                    return (179 * static_cast<size_t>(mag)
                            + 647 * static_cast<size_t>(angle) );
                }
    bool        operator== (const TransformInfo& rhs) const {
                    return (mag == rhs.mag  &&  angle == rhs.angle);
                }
    bool        operator!= (const TransformInfo& rhs) const {
                    return (! (*this == rhs));
                }
};


//======================================================================
//                              Elements
//======================================================================

// To store the buffered elements we define eleminfo structs for each
// type of OASIS element we write, e.g., RectangleInfo for RECTANGLE
// records and PathInfo for PATH records.  The members of this struct
// are obtained by converting the fields of the GDSII element.
//
// Two element types need special treatment.  How we handle PLACEMENT
// records depends on whether they were generated from SREF or AREF
// elements in GDSII; so we have both SrefInfo and ArefInfo.  For TEXT
// elements we have two structs, TextInfo and XTextInfo.  Which is used
// depends on whether the GDSII text has a geometry transform and on
// the setting of convOptions.transformTexts.
//
// Each eleminfo struct provides an operator<.  This is used for sorting
// the elements to maximise use of the modal variables in the OASIS file.
//
// Each eleminfo struct apart from ArefInfo provides two other
// operations.  getHash() returns a hash of the element's fields so that
// the eleminfo can be inserted in a hash table.  It is not necessary to
// include all fields in the hash.  operator== compares two eleminfo
// structs for equality.  Eleminfos that are equal are merged, i.e., a
// single OASIS record is written.
//
// Apart from the data members for the element fields, each eleminfo
// struct except ArefInfo has a member
//     vector<GdsPoint>  positions;
// This is the set of positions at which the element appears.  When a
// new element is merged into an older one, the new element's position
// is added to the old element's vector and the new element is deleted.
//
// XXX: In all the eleminfo structs we use primitive hash functions.
// Should measure performance on real data.  Similarly operator< also
// does a half-hearted job of comparison.


// ElementInfo -- base class for all eleminfo structs

struct ElementInfo {
    PropInfoList        propList;       // property list
};


//----------------------------------------------------------------------
//                              SrefInfo
//----------------------------------------------------------------------

// SrefInfo instances are created for SREFs in GDSII, and are used to
// generate PLACEMENT records in OASIS.  The cellIndex member is the
// index in GdsToOasisConverter::allCellNames of the CellName for the
// cell being placed.


struct SrefInfo : public ElementInfo {
    vector<GdsPoint> positions;
    CellNameIndex    cellIndex;
    TransformInfo    transform;

    size_t      getHash() const;
    bool        operator== (const SrefInfo& rhs) const;
    bool        operator<  (const SrefInfo& rhs) const {
                    return (cellIndex < rhs.cellIndex);
                }
};



// getHash() -- compute hash of contents of SrefInfo

size_t
SrefInfo::getHash() const
{
    return (433 * cellIndex + transform.getHash() + propList.getHash());
}



bool
SrefInfo::operator== (const SrefInfo& rhs) const
{
    // Note that the positions are not included in the comparison.

    return (cellIndex == rhs.cellIndex
            &&  transform == rhs.transform
            &&  propList  == rhs.propList);
}



//----------------------------------------------------------------------
//                              ArefInfo
//----------------------------------------------------------------------

// ArefInfo does not have the vector of GdsPoints that other ElementInfo
// subclasses have.  Because each ArefInfo already has a repetition spec
// describing the array, we don't try to merge different ArefInfo
// instances.  Hence each ArefInfo has just one position.  Because we
// don't merge ArefInfos we don't need getHash() or operator==().

struct ArefInfo : public ElementInfo {
    GdsPoint       pos;
    CellNameIndex  cellIndex;
    TransformInfo  transform;
    Repetition     rep;

    bool        operator<  (const ArefInfo& rhs) const {
                    return (cellIndex < rhs.cellIndex);
                }
};



//----------------------------------------------------------------------
//                              RectangleInfo
//----------------------------------------------------------------------

// A BOUNDARY element in GDSII is converted into a RECTANGLE, TRAPEZOID,
// or POLYGON in OASIS.  BOX elements are converted into RECTANGLEs.


struct RectangleInfo : public ElementInfo {
    vector<GdsPoint>    positions;
    short       layer;
    short       datatype;
    int         width, height;

    size_t      getHash() const;
    bool        operator== (const RectangleInfo& rhs) const;
    bool        operator<  (const RectangleInfo& rhs) const;
};



bool
RectangleInfo::operator< (const RectangleInfo& rhs) const
{
    return (layer < rhs.layer
            ||  (layer == rhs.layer  &&  width < rhs.width));
}



size_t
RectangleInfo::getHash() const
{
    return (997 * layer
            + 271 * datatype
            + 761 * static_cast<size_t>(width)
            + 463 * static_cast<size_t>(height)
            + propList.getHash() );
}



bool
RectangleInfo::operator== (const RectangleInfo& rhs) const
{
    return (layer == rhs.layer
            &&  datatype == rhs.datatype
            &&  width    == rhs.width
            &&  height   == rhs.height
            &&  propList == rhs.propList);
}



//----------------------------------------------------------------------
//                              TrapezoidInfo
//----------------------------------------------------------------------

struct TrapezoidInfo : public ElementInfo {
    vector<GdsPoint>   positions;
    short       layer;
    short       datatype;
    int         width, height;
    int         delta_a, delta_b;       // as defined by OASIS
    Oasis::Trapezoid::Orientation  orient;

    size_t      getHash() const;
    bool        operator== (const TrapezoidInfo& rhs) const;
    bool        operator<  (const TrapezoidInfo& rhs) const {
                    return (layer < rhs.layer);
                }
};



size_t
TrapezoidInfo::getHash() const
{
    return (157 * layer
            + 439 * datatype
            + 761 * static_cast<size_t>(width)
            + 911 * static_cast<size_t>(height)
            + 257 * static_cast<size_t>(delta_a)
            + 563 * static_cast<size_t>(delta_b)
            + 433 * static_cast<size_t>(orient)
            + propList.getHash() );
}


bool
TrapezoidInfo::operator== (const TrapezoidInfo& rhs) const
{
    return (layer == rhs.layer
            &&  datatype == rhs.datatype
            &&  width    == rhs.width
            &&  height   == rhs.height
            &&  delta_a  == rhs.delta_a
            &&  delta_b  == rhs.delta_b
            &&  orient   == rhs.orient
            &&  propList == rhs.propList);
}



//----------------------------------------------------------------------
//                              PolygonInfo
//----------------------------------------------------------------------

struct PolygonInfo : public ElementInfo {
    vector<GdsPoint>    positions;
    short       layer;
    short       datatype;
    PointList   ptlist;

    size_t      getHash() const;
    bool        operator== (const PolygonInfo& rhs) const;
    bool        operator<  (const PolygonInfo& rhs) const {
                    return (layer < rhs.layer);
                }
};



size_t
PolygonInfo::getHash() const
{
    size_t hash = 701 * layer
                  + 337 * datatype
                  + 977 * ptlist.getType()
                  + propList.getHash();

    PointList::const_iterator  iter = ptlist.begin();
    PointList::const_iterator  end  = ptlist.end();
    for ( ;  iter != end;  ++iter) {
        hash += 149 * static_cast<size_t>(iter->x);
        hash += 587 * static_cast<size_t>(iter->y);
    }

    return hash;
}


bool
PolygonInfo::operator== (const PolygonInfo& rhs) const
{
    return (layer == rhs.layer
            &&  datatype == rhs.datatype
            &&  ptlist    == rhs.ptlist
            &&  propList == rhs.propList);
}



//----------------------------------------------------------------------
//                              PathInfo
//----------------------------------------------------------------------

struct PathInfo : public ElementInfo {
    vector<GdsPoint>    positions;
    short       layer;
    short       datatype;
    int         halfwidth;
    int         startExtn;
    int         endExtn;
    PointList   ptlist;

    size_t      getHash() const;
    bool        operator== (const PathInfo& rhs) const;
    bool        operator<  (const PathInfo& rhs) const {
                    return (layer < rhs.layer);
                }
};


size_t
PathInfo::getHash() const
{
    size_t hash = 563 * layer
                  + 787 * datatype
                  + 173 * static_cast<size_t>(halfwidth)
                  + 491 * static_cast<size_t>(startExtn)
                  + 223 * static_cast<size_t>(endExtn)
                  + 947 * ptlist.getType()
                  + propList.getHash();

    PointList::const_iterator  iter = ptlist.begin();
    PointList::const_iterator  end  = ptlist.end();
    for ( ;  iter != end;  ++iter) {
        hash += 149 * static_cast<size_t>(iter->x);
        hash += 587 * static_cast<size_t>(iter->y);
    }

    return hash;
}



bool
PathInfo::operator== (const PathInfo& rhs) const
{
    return (layer == rhs.layer
            &&  datatype  == rhs.datatype
            &&  halfwidth == rhs.halfwidth
            &&  startExtn == rhs.startExtn
            &&  endExtn   == rhs.endExtn
            &&  ptlist    == rhs.ptlist
            &&  propList  == rhs.propList);
}



//----------------------------------------------------------------------
//                              TextInfo
//----------------------------------------------------------------------

// TEXT elements are special because GDSII allows texts to be
// transformed but OASIS does not.  Because we expect most GDSII TEXTs
// to be untransformed, we use two eleminfo structs to store them --
// TextInfo for untransformed TEXTs and XTextInfo for transformed ones.

struct TextInfo : public ElementInfo {
    vector<GdsPoint>    positions;
    TextStringIndex     textIndex;
    short       layer;
    short       texttype;
    int         width;
    Uchar       pathtype;
    Uchar       presentation;

    void        setPresentation (Uint font, Uint vjust, Uint hjust) {
                    assert (font < 4  &&  vjust < 4  &&  hjust < 4);
                    presentation = ((font << 4) | (vjust << 2) | hjust);
                }
    Uint        getFont()         const { return ((presentation >> 4) & 0x3); }
    Uint        getVertJustify()  const { return ((presentation >> 2) & 0x3); }
    Uint        getHorizJustify() const { return (presentation        & 0x3); }

    size_t      getHash() const;
    bool        operator== (const TextInfo& rhs) const;
    bool        operator<  (const TextInfo& rhs) const {
                    return (layer < rhs.layer);
                }
};


size_t
TextInfo::getHash() const {
    return (631*textIndex + 239*layer + 827*texttype + propList.getHash());
}


bool
TextInfo::operator== (const TextInfo& rhs) const
{
    return (textIndex == rhs.textIndex
            &&  layer        == rhs.layer
            &&  texttype     == rhs.texttype
            &&  width        == rhs.width
            &&  pathtype     == rhs.pathtype
            &&  presentation == rhs.presentation
            &&  propList     == rhs.propList);
}



//----------------------------------------------------------------------
//                              XTextInfo
//----------------------------------------------------------------------

// XTextInfo -- store a transformed text

// If a GDSII TEXT has a transform and we have to preserve the transform
// (convOptions.transformTexts is true), we cannot write the GDSII TEXT
// as an OASIS TEXT because OASIS does not allow transforms in TEXT
// records.
//
// Instead we create a special cell containing just the TEXT element and
// write that cell after the current one.  In the current cell we
// convert the TEXT to a PLACEMENT (with a transform) of the special
// cell.  The member 'cellIndex' identifies the special cell to
// reference in the PLACEMENT.
//
// This class is publicly derived from TextInfo for implementation
// convenience.

struct XTextInfo : public TextInfo {
    vector<GdsPoint> positions;
    TransformInfo    transform;
    CellNameIndex    cellIndex;

    size_t      getHash() const;
    bool        operator== (const XTextInfo& rhs) const;
    bool        operator<  (const XTextInfo& rhs) const {
                    return (layer < rhs.layer);
                }
};


size_t
XTextInfo::getHash() const {
    return (TextInfo::getHash() + transform.getHash());
}


bool
XTextInfo::operator== (const XTextInfo& rhs) const
{
    // Don't compare the cellIndex members.  cellIndex is set only in
    // GdsToOasisConverter::writeTransformedTexts() when a PLACEMENT
    // record is written for the text.

    return (textIndex == rhs.textIndex
            &&  transform    == rhs.transform
            &&  layer        == rhs.layer
            &&  texttype     == rhs.texttype
            &&  width        == rhs.width
            &&  pathtype     == rhs.pathtype
            &&  presentation == rhs.presentation
            &&  propList     == rhs.propList);
}


//======================================================================
//                           ElementManager
//======================================================================

// We have described all the eleminfo structs which are used to store
// the buffered elements.  Let us turn now to element managers, the
// containers in which the eleminfo structs are stored.
//
// GdsToOasisConverter has an ElementManager for each type of eleminfo.
// When it gets an element from GdsParser it constructs an eleminfo on
// the heap and passes it to the appropriate ElementManager.  When it
// has accumulated enough elements it iterates through the elements in
// each manager, writes them to the OASIS file, and deletes them.
//
// Each ElementManager has several responsibilites.
//
//   - Checking each eleminfo that GdsToOasisConverter gives it to see
//     if that eleminfo is identical to one that it already has.
//     If yes, it should merge the new eleminfo with the old one.  That
//     is, it should only add the new eleminfo's position to the old
//     eleminfo's vector of positions.
//
//   - Sorting the elements before they are written to take advantage of
//     OASIS's modal variables.  Getting the optimal sort order is of
//     course impractical.  sort() should try for a good trade-off
//     between sorting time and compactness of the OASIS file.
//
//   - Deleting the eleminfo structs contained if an exception occurs
//     and the ElementManager is destroyed during stack unwinding.
//     Normally GdsToOasisConverter deletes all the eleminfos after
//     writing them.
//
// The manager for ArefInfo is an exception.  The first requirement, for
// merging identical eleminfos, does not apply to it.  For all the others,
// we define a class template.


template <typename ElemType>
class ElementManager {
    // The hasher and equality predicate for the HashSet delegate the
    // task to the corresponding eleminfo methods.
    //
    struct ElemHash {
        size_t
        operator() (const ElemType* elem) const {
            return elem->getHash();
        }
    };
    struct ElemEqual {
        bool
        operator() (const ElemType* lhs, const ElemType* rhs) const {
            return (*lhs == *rhs);
        }
    };

    // Every element in the manager is in both a vector and a HashSet.
    // The vector owns the elements.  We use the vector to sort the
    // elements before iterating through them, and for the iteration
    // itself.  We use the set to see if a newly-added element can be
    // merged with one we already have.

    typedef PointerVector<ElemType>                   ElemVector;
    typedef HashSet<ElemType*, ElemHash, ElemEqual>   ElemHashSet;

    ElemVector  elemVec;
    ElemHashSet elemSet;

public:
                ElementManager();
                ~ElementManager();
    void        add (ElemType* elem, const GdsPoint& pos);
    void        sort();
    void        deleteAll();
    void        clear();
    size_t      numUniqueElements() const  { return elemVec.size(); }

    // Methods to iterate over the elements after sorting them.

    typedef typename ElemVector::iterator        iterator;
    typedef typename ElemVector::const_iterator  const_iterator;

    iterator            begin()       { return elemVec.begin(); }
    const_iterator      begin() const { return elemVec.begin(); }
    iterator            end()         { return elemVec.end();   }
    const_iterator      end()   const { return elemVec.end();   }
};



// ElementManager constructor
// Make both the HashSet and the vector reasonably large.

template <typename ElemType>
ElementManager<ElemType>::ElementManager()
  : elemSet(MaxBufferedElements/10)
{
    elemVec.reserve(MaxBufferedElements/10);
}


template <typename ElemType>
ElementManager<ElemType>::~ElementManager() { }



// deleteAll -- delete all the elements being managed.
// This is invoked after all the elements have been written to the OASIS
// file.

template <typename ElemType>
void
ElementManager<ElemType>::deleteAll()
{
    DeleteContainerElements(elemVec);
    elemSet.clear();
}



// clear -- remove elements from manager without deleting them.
// We need this only for XTextInfo.  To prevent accidental use in the
// other managers instead of deleteAll(), define only for XTextInfo.

template <>
void
ElementManager<XTextInfo>::clear()
{
    elemVec.clear();
    elemSet.clear();
}



// add -- add an instance of an element at a given position
//   elem       element to add.  This must be allocated from the free
//              store.  add() takes ownership of the element and deletes
//              it if it is identical to an element already present.
//   pos        position of element

template <typename ElemType>
void
ElementManager<ElemType>::add (ElemType* elem, const GdsPoint& pos)
{
    // If the new elem is different from all the elems we already have,
    // add it to both the set and the vector and initialize its vector
    // of positions with its own position.  If it is identical to one of
    // the existing elems, simply add the new position to the existing
    // elem's vector of positions and delete the new elem.
    //
    // The order of operations is tricky because we must make sure that
    // the object being added has exactly one owner at any time that an
    // exception can be thrown.  Initially the caller owns the object.
    // After it has been inserted into the vector, the manager owns the
    // object.  So we must not add it to the vector until we are sure we
    // can return successfully.  Similarly if the element already exists
    // we must delete the new element only after adding the new position
    // to the old element.

    std::pair<typename ElemHashSet::iterator, bool>  insVal =
                                                        elemSet.insert(elem);

    if (insVal.second) {        // new element is different from all previous
        elem->positions.push_back(pos);
        elemVec.push_back(elem);
    } else {                    // identical element already present
        (*insVal.first)->positions.push_back(pos);
        delete elem;
    }
}



// ElemLess -- compare elements for sorting them
// This just invokes the eleminfo's operator<.

template <typename ElemType>
struct ElemLess {
    bool operator() (const ElemType* lhs, const ElemType* rhs) const {
        return (*lhs < *rhs);
    }
};



// sort -- sort elements to maximise use of modal variables
// GdsToOasisConverter calls this before it starts writing the elements.

template <typename ElemType>
void
ElementManager<ElemType>::sort()
{
    std::sort(elemVec.begin(), elemVec.end(), ElemLess<ElemType>());
}



typedef ElementManager<SrefInfo>        SrefManager;
typedef ElementManager<RectangleInfo>   RectManager;
typedef ElementManager<TrapezoidInfo>   TrapezoidManager;
typedef ElementManager<PolygonInfo>     PolygonManager;
typedef ElementManager<PathInfo>        PathManager;
typedef ElementManager<TextInfo>        TextManager;
typedef ElementManager<XTextInfo>       XTextManager;



//======================================================================
//                              ArefManager
//======================================================================

// As mentioned above, ArefInfos are treated differently from other
// eleminfos.  Identical eleminfos are normally merged so that we can
// make the OASIS file more compact by using a repetition specification
// instead of repeating the element.  But identical ArefInfos cannot be
// merged because each already has a repetition spec describing the
// array.  So for ArefInfo we cannot use ElementManager; we need a
// special manager that does not merge and hence needs no HashSet.


class ArefManager {
    PointerVector<ArefInfo>  arefVec;

public:
                ArefManager();
    void        add (ArefInfo* aref, const GdsPoint& pos);
    void        sort();
    void        deleteAll();
    size_t      numUniqueElements() const  { return arefVec.size(); }

    typedef vector<ArefInfo*>::iterator iterator;
    typedef vector<ArefInfo*>::const_iterator  const_iterator;

    iterator            begin()       { return arefVec.begin(); }
    const_iterator      begin() const { return arefVec.begin(); }
    iterator            end()         { return arefVec.end();   }
    const_iterator      end()   const { return arefVec.end();   }
};


inline
ArefManager::ArefManager()
{
    arefVec.reserve(MaxBufferedElements/10);
}


inline void
ArefManager::deleteAll() {
    DeleteContainerElements(arefVec);
}



// add -- add an ArefInfo at a given position
//   aref       ArefInfo to add.  This must be allocated from the free
//              store.  add() takes ownership of the element.
//   pos        position of ArefInfo

inline void
ArefManager::add (ArefInfo* aref, const GdsPoint& pos) {
    aref->pos = pos;
    arefVec.push_back(aref);
}


struct ArefLess {
    bool operator() (const ArefInfo* lhs, const ArefInfo* rhs) const {
        return (*lhs < *rhs);
    }
};


void
ArefManager::sort() {
    std::sort(arefVec.begin(), arefVec.end(), ArefLess());
}



//======================================================================
//                           GdsToOasisConverter
//======================================================================

// Now comes the class that drives everything.  GdsToOasisConverter is
// derived from GdsBuilder.  ConvertGdsToOasis() creates a GdsParser and
// invokes its parseFile(), passing an instance of GdsToOasisConverter
// as the builder argument.  As GdsParser hands it the elements,
// GdsToOasisConverter saves the elements' data in eleminfo structs and
// hands the structs to the appropriate ElementManager.  When it has
// enough elements, and at the end of each structure (cell), it invokes
// OasisCreator methods to write out the buffered elements to the OASIS
// file.


/*----------------------------------------------------------------------
GdsToOasisConverter -- a GdsBuilder that translates the GDSII file to OASIS

Data members

warnHandler             WarningHandler

    Callback to display warning and verbose messages.  May be Null.

locator                 const GdsLocator*

    The locator object passed by the parser.  This tells where the parser
    is in the file.  Used by the method warn() to print the context.

currElement             ElementInfo*

    The element currently being assembled, or Null. GdsToOasisConverter
    creates a new eleminfo struct and sets currElement to it in each of
    the beginFoo() methods that begin elements.  addProperty() adds the
    property to currElement's property list.  endElement() then passes
    the completed element to the manager for its type and sets
    currElement to Null.

    GdsToOasisConverter must hang on to each eleminfo until it has
    finished adding all its properties.  That is because most element
    managers compare new elements to existing elements, and the
    comparison includes the property lists.

    beginNode() sets this to Null because GdsToOasisConverter ignores
    GDSII NODE elements.

currElemType            enum ElementType

    Indicates the type of currElement.  Valid only when currElement !=
    Null.  Set by the methods that begin elements, via
    setCurrentElement(), when they set currElement.  We keep track of
    the type mainly to save the overhead of giving ElementInfo a virtual
    destructor.  It also comes useful in endElement(), which must decide
    to which manager it must give currElement.

currElemPos             GdsPoint

    The position of currElement.  Valid only when currElement != Null.
    Set by the methods that begin elements.

currentCell             Uint

    The index in allCellNames of the cell currently being processed.
    Set in beginStructure() and used by beginCell(), which writes the
    CELL record.  When immediateNames is true, beginCell() is called
    only at the end of the structure.

nextTextCellNum         Uint

    Used to generate unique (we hope) cell names for the special cells
    generated to hold transformed texts.  generateTextCellName()
    increments this each time it is invoked.

numBufferedElements     Uint

    The number of elements added to all the element managers since the
    last time the managers were emptied, i.e., since the last call to
    writeElements() or endCell().  endElement() calls writeElements()
    to write out all buffered elements when this number exceeds
    MaxBufferedElements, but only if immediateNames is false.

    This member is unused when immediateNames is true.

placementPos            Delta
textPos                 Delta
geometryPos             Delta

    Position of last placement/text/geometry element written to OASIS
    file.  These are used when convOptions.relativeMode is true.  When
    xy-mode is relative, the position fields in the element records of
    OASIS files contain not the element's actual position but the delta
    between its position and that of the last-written element of the
    same class.

    If the last-written element and the current one are far enough
    apart, this delta might overflow.  We must ensure that this does not
    happen.  We do it by keeping track of the last position and
    switching temporarily to absolute mode if the delta would overflow.

    These variables are reset to (0,0) at the beginning of each cell
    and are updated each time an element is written to the OASIS file.

propProperty            Property*
propPresentation        Property*
propPathtype            Property*
propWidth               Property*

    There are only four OASIS properties that we write: S_GDS_PROPERTY,
    GDS_PRESENTATION, GDS_PATHTYPE, and GDS_WIDTH.  We use these members
    as temporaries whenever we must write one, instead of repeatedly
    constructing and destroying Property objects.

properties              PointerVector<Property>
propNames               PointerVector<PropName>

    Owning vectors for the four Property objects described above, and
    for the PropName objects for their names.  Note that Property does
    not own the PropName it references.

xtexts                  PointerVector<XTextInfo>

    Holds all XTextInfo objects after writeTransformedTexts() writes
    PLACEMENT elements for them, until writeTextCells() writes the
    special cells for them.  We must remove all XTextInfo objects from
    the manager after writeTransformedTexts() processes them, or the
    next call will write them out again.  But we cannot delete them
    until we write the special cells that contain the TEXT elements, and
    we can do that only after the current cell ends.

    So writeTransformedTexts() moves all the XTextInfo objects here when
    it is done with them.  Later writeTextCells() deletes the objects
    and clears the vector when it in turn is done with the XTextInfos.

cellNameMap             StringToCellNameMap
allCellNames            PointerVector<CellName>

    For each GDSII structure name we create the equivalent in the Oasis
    module, a CellName.  We put pointers to all the CellName objects we
    create into allCellNames; it owns the objects.  Throughout the code
    each CellName is identified by its index in allCellNames.  We use
    cellNameMap to locate the CellName for a given structure name.

textStringMap           StringToTextStringMap
allTextStrings          PointerVector<TextString>

    To write an OASIS TEXT record we must create a TextString object for
    the text string and pass that to OasisCreator::beginText().  Because
    we want all the name tables in the OASIS file to be strict, we must
    register the TextString with OasisCreator before using it, and
    retain it until the end of the file.

    allTextStrings contains pointers to all the TextStrings we create;
    it owns the objects.  As with CellNames, each PropString is
    identified by its index in allTextStrings.  We use textStringMap to
    locate a TextString for a given string.  This is to avoid creating
    two TextStrings with the same string, which paragraph 16.4 of the
    OASIS spec forbids.

propStringMap           StringToPropStringMap
allPropStrings          PointerVector<PropString>

    We create a PropString object for each unique GDSII property value
    we write.  As with TextStrings, we register all these values with
    OasisCreator so that the propstring name table will be strict.

------------------------------------------------------------------------*/


class GdsToOasisConverter : public GdsBuilder {
    enum ElementType {
        ElemSref,
        ElemAref,
        ElemNormalText,
        ElemTransformedText,
        ElemRectangle,
        ElemTrapezoid,
        ElemPolygon,
        ElemPath
    };

    OasisCreator        creator;        // create OASIS file
    WarningHandler      warnHandler;    // display warning messages
    GdsToOasisOptions   convOptions;    // conversion options
    const GdsLocator*   locator;        // location in input file

    ElementInfo*        currElement;
    ElementType         currElemType;   // valid only when currElement != Null
    GdsPoint            currElemPos;    // valid only when currElement != Null

    Uint                currentCell;    // index of cell now being processed
    Uint                nextTextCellNum;
    Uint                numBufferedElements;

    // Managers for the buffered elements
    SrefManager         srefMgr;
    ArefManager         arefMgr;
    TextManager         textMgr;
    XTextManager        xtextMgr;
    RectManager         rectMgr;
    TrapezoidManager    trapMgr;
    PolygonManager      polygonMgr;
    PathManager         pathMgr;

    // Position of last placement/text/geomtry element written to OASIS file,
    // for use with relative xy-mode.

    Delta               placementPos;
    Delta               textPos;
    Delta               geometryPos;

    // Temporaries to reuse when writing OASIS properties
    Property*           propProperty;           // S_GDS_PROPERTY
    Property*           propPresentation;       // GDS_PRESENTATION
    Property*           propPathtype;           // GDS_PATHTYPE
    Property*           propWidth;              // GDS_WIDTH

    PointerVector<Property>     properties;
    PointerVector<PropName>     propNames;
    PointerVector<XTextInfo>    xtexts;

    typedef HashMap<const char*, CellNameIndex,
                    hash<const char*>, StringEqual>  StringToCellNameMap;
    StringToCellNameMap         cellNameMap;
    PointerVector<CellName>     allCellNames;


    typedef HashMap<const char*, TextStringIndex,
                    hash<const char*>, StringEqual>  StringToTextStringMap;
    StringToTextStringMap       textStringMap;
    PointerVector<TextString>   allTextStrings;


    typedef HashMap<const char*, PropStringIndex,
                    hash<const char*>, StringEqual>  StringToPropStringMap;
    StringToPropStringMap       propStringMap;
    PointerVector<PropString>   allPropStrings;

public:
                  GdsToOasisConverter (const char* outfilename,
                                       WarningHandler warner,
                                       const GdsToOasisOptions& options);
    virtual       ~GdsToOasisConverter();

    // Most methods of GdsToOasisConverter fall into three groups.
    //   - GdsBuilder methods to collect data from GdsParser;
    //   - methods to write the buffered elements to the OASIS file;
    //   - methods to convert elements from GDSII to OASIS.

    virtual void  setLocator (const GdsLocator* locator);

    // virtual void  gdsVersion (int version);
    //     Inherit empty method from GdsBuilder

    virtual void  beginLibrary (const char* libName,
                                const GdsDate& modTime,
                                const GdsDate& accTime,
                                const GdsUnits& units,
                                const GdsLibraryOptions& options);
    virtual void  endLibrary();

    virtual void  beginStructure (const char* structureName,
                                  const GdsDate& createTime,
                                  const GdsDate& modTime,
                                  const GdsStructureOptions& options);
    virtual void  endStructure();


    virtual void  beginBoundary (int layer, int datatype,
                                 const GdsPointList& points,
                                 const GdsElementOptions& options);

    virtual void  beginPath (int layer, int datatype,
                             const GdsPointList& points,
                             const GdsPathOptions& options);

    virtual void  beginSref (const char* sname,
                             int x, int y,
                             const GdsTransform& strans,
                             const GdsElementOptions& options);

    virtual void  beginAref (const char* sname,
                             Uint numCols, Uint numRows,
                             const GdsPointList& points,
                             const GdsTransform& strans,
                             const GdsElementOptions& options);

    virtual void  beginNode (int layer, int nodetype,
                             const GdsPointList& points,
                             const GdsElementOptions& options);

    virtual void  beginBox (int layer, int boxtype,
                            const GdsPointList& points,
                            const GdsElementOptions& options);

    virtual void  beginText (int layer, int texttype,
                             int x, int y,
                             const char* str,
                             const GdsTransform& strans,
                             const GdsTextOptions& options);

    virtual void  addProperty (int attr, const char* value);

    virtual void  endElement();

private:
    Property*   makeProperty (const char* pname, bool isStd);
    CellName*   getCellName (CellNameIndex index);
    TextString* getTextString (TextStringIndex index);
    PropString* getPropString (PropStringIndex index);
    void        setCurrentElement (ElementInfo* elem, ElementType type);

    CellNameIndex       generateTextCellName();
    CellNameIndex       makeCellName (const char* structureName);
    TextStringIndex     makeTextString (const char* str);
    PropStringIndex     makePropString (const char* str);

    // Methods to write the buffered elements to the OASIS file.

    void        writeElements();
    void        beginCell();
    void        writeSrefs();
    void        writeArefs();
    void        writeNormalTexts();
    void        writeTextPresentationProperties (const TextInfo* text);
    void        writeTransformedTexts();
    void        writeRectangles();
    void        writeTrapezoids();
    void        writePolygons();
    void        writePaths();
    void        writePropertyList (const PropInfoList& propList);
    void        writeTextCells();

    // Methods to convert elements from GDSII to OASIS.

    void        convertTransform (const char* elemType, const char* elemName,
                                  const GdsTransform& strans,
                                  /*out*/ TransformInfo* transform);

    void        convertPointList (const GdsPointList& points,
                                  bool isPolygon,
                                  /*out*/ PointList* ptlist);
    PointList::ListType
                getPointListType (const GdsPointList& points, bool isPolygon);

    bool        tryTrapezoid (const GdsPointList& points,
                              /*out*/ TrapezoidInfo* trap,
                              /*out*/ GdsPoint*  pos);
    bool        tryVerticalTrapezoid (const GdsPoint pt[],
                                      /*out*/ TrapezoidInfo* trap,
                                      /*out*/ GdsPoint* pos);
    bool        tryHorizontalTrapezoid (const GdsPoint pt[],
                                        /*out*/ TrapezoidInfo* trap,
                                        /*out*/ GdsPoint* pos);
    void        pathToRectangle (const GdsPointList& points,
                                 int halfwidth, int startExtn, int endExtn,
                                 /*out*/ int* pWidth,
                                 /*out*/ int* pHeight,
                                 /*out*/ GdsPoint* pos);

    void        warn (const char* fmt, ...)           SJ_PRINTF_ARGS(2,3);
    void        displayMessage (const char* fmt, ...) SJ_PRINTF_ARGS(2,3);

private:
                GdsToOasisConverter (const GdsToOasisConverter&);  // forbidden
    void        operator= (const GdsToOasisConverter&);            // forbidden
};



// GdsToOasisConverter constructor
//   outfilename  pathname of OASIS file
//   warner       callback to display warning messages.  May be Null.
//   options      conversion options

GdsToOasisConverter::GdsToOasisConverter (const char* outfilename,
                                          WarningHandler warner,
                                          const GdsToOasisOptions& options)
  : GdsBuilder(),
    creator(outfilename, OasisCreatorOptions(options.immediateNames)),
    warnHandler(warner),
    placementPos(0, 0),
    textPos(0, 0),
    geometryPos(0, 0)
{
    this->convOptions = options;
    currElement = Null;
    nextTextCellNum = 0;
    numBufferedElements = 0;
    locator = Null;
}



/*virtual*/
GdsToOasisConverter::~GdsToOasisConverter()
{
    // Normal case.
    if (currElement == Null)
        return;

    // This is executed only if something has gone wrong, e.g., the
    // converter is being destroyed because of an exception.  Normally
    // endElement() hands over currElement to the appropriate manager
    // and sets currElement to Null.
    //
    // currElement is not an auto_ptr because we must cast to the correct
    // subclass before deleting.  ElementInfo does not have a virtual
    // destructor because we might create and destroy millions of instances.
    // XXX: Maybe we should reuse those instances instead of destroying them.
    //
    // Note that currElemType is defined only if currElement != Null.

    switch (currElemType) {
        case ElemSref:
            delete static_cast<SrefInfo*>(currElement);
            break;
        case ElemAref:
            delete static_cast<ArefInfo*>(currElement);
            break;
        case ElemNormalText:
            delete static_cast<TextInfo*>(currElement);
            break;
        case ElemTransformedText:
            delete static_cast<XTextInfo*>(currElement);
            break;
        case ElemRectangle:
            delete static_cast<RectangleInfo*>(currElement);
            break;
        case ElemTrapezoid:
            delete static_cast<TrapezoidInfo*>(currElement);
            break;
        case ElemPolygon:
            delete static_cast<PolygonInfo*>(currElement);
            break;
        case ElemPath:
            delete static_cast<PathInfo*>(currElement);
            break;
        default:
            assert (false);
    }
}



// We keep pointers to all the CellNames, TextStrings, and PropStrings
// we create in three vectors, and use the index in these vectors
// instead of the pointers.  The next three functions each map an index
// to a pointer to the object.


inline CellName*
GdsToOasisConverter::getCellName (CellNameIndex index) {
    assert (index < allCellNames.size());
    return allCellNames[index];
}


inline TextString*
GdsToOasisConverter::getTextString (TextStringIndex index) {
    assert (index < allTextStrings.size());
    return allTextStrings[index];
}


inline PropString*
GdsToOasisConverter::getPropString (PropStringIndex index) {
    assert (index < allPropStrings.size());
    return allPropStrings[index];
}



// The beginFoo() element-starting methods invoke setCurrentElement()
// immediately after allocating the eleminfo object and before doing
// anything else with it.  This ensures that the object is owned.

inline void
GdsToOasisConverter::setCurrentElement (ElementInfo* elem, ElementType type)
{
    assert (currElement == Null);
        // endElement() sets currElement to Null, so something has gone
        // wrong if this is non-Null.

    currElement = elem;
    currElemType = type;
}



//----------------------------------------------------------------------
//                          GdsBuilder methods
//----------------------------------------------------------------------

// Now follow the GdsBuilder methods that GdsParser invokes as it
// recognizes chunks of the GDSII file.


/*virtual*/ void
GdsToOasisConverter::setLocator (const GdsLocator* locator)
{
    this->locator = locator;
}



// beginLibrary -- invoked after reading the library header records
// The only part of the header we look at is the UNITS record.

/*virtual*/ void
GdsToOasisConverter::beginLibrary (const char*  /*libName*/,
                                   const GdsDate&  /*modTime*/,
                                   const GdsDate&  /*accTime*/,
                                   const GdsUnits&  units,
                                   const GdsLibraryOptions&  /*options*/)
{
    // XXX:
    // Paragraph 13.3 of the OASIS spec says:
    //     The unit declaration is a positive real number which
    //     specifies the global precision of the OASIS file's
    //     coordinate system in grid steps per micron.  The OASIS unit
    //     value is essentially the reciprocal of the first value in the
    //     GDSII Stream UNITS record.
    //
    // Assuming that what OASIS calls the grid step is what GDSII calls
    // the database unit, this is true only if the GDSII user unit is
    // 1 micron.  The computation below, which uses the second value in
    // the UNITS record, seems to make more sense.

    Oreal  oasisUnit(1e-6/units.dbToMeter);
    creator.beginFile(OasisVersion, oasisUnit, convOptions.valScheme);

    creator.setCompression(convOptions.compress);

    // Create Property objects for the OASIS properties we write.  We
    // cache these objects in the class and use them whenever we want to
    // write a property, instead of creating and destroying Property
    // objects each time.  This is feasible because we write only four
    // different properties.

    // S_GDS_PROPERTY is for translated GDSII properties.
    // (See A2-3.1 in the OASIS spec.)  The first value is the propattr
    // and the second is the propvalue.  Pre-allocate space in the string
    // to avoid reallocations later.

    const size_t  InitLen = Gdsii::GLimits::MaxPropValueLength;
    propProperty = makeProperty("S_GDS_PROPERTY", true);
    propProperty->addValue(PV_UnsignedInteger, Ullong(0));
    propProperty->addValue(PV_BinaryString, string(InitLen, Cnul));

    // GDS_PRESENTATION is for the contents of the PRESENTATION record
    // in TEXT elements.  The three unsigned-integers are font,
    // vertical justification, and horizontal justification.

    propPresentation = makeProperty("GDS_PRESENTATION", false);
    propPresentation->addValue(PV_UnsignedInteger, Ullong(0));
    propPresentation->addValue(PV_UnsignedInteger, Ullong(0));
    propPresentation->addValue(PV_UnsignedInteger, Ullong(0));

    // GDS_PATHTYPE is for the contents of the PATHTYPE record in TEXT
    // elements.

    propPathtype = makeProperty("GDS_PATHTYPE", false);
    propPathtype->addValue(PV_UnsignedInteger, Ullong(0));

    // GDS_WIDTH is for the contents of the WIDTH record in TEXT
    // elements.  Note that the value's type is signed-integer.  In
    // GDSII a negative width is absolute, i.e., unaffected by the
    // magnification of the parent SREF/AREF.

    propWidth = makeProperty("GDS_WIDTH", false);
    propWidth->addValue(PV_SignedInteger, llong(0));
}



/*virtual*/ void
GdsToOasisConverter::beginStructure (const char*  structureName,
                                     const GdsDate&  /*createTime*/,
                                     const GdsDate&  /*modTime*/,
                                     const GdsStructureOptions&)
{
    if (convOptions.verbose)
        displayMessage("converting structure '%s'", structureName);

    // Make a CellName from the structure name and save its index
    // for beginCell() to use.

    currentCell = makeCellName(structureName);

    // Position modal variables get reset to 0 at the beginning of
    // each cell.  OASIS spec 10.1

    placementPos.assign(0, 0);
    textPos.assign(0, 0);
    geometryPos.assign(0, 0);

    // Whether we write the CELL record here depends on immediateNames.
    // beginText() may create TextString objects and addProperty() may
    // create PropString objects.  If immediateNames is true,
    // registering these names would result in name records being
    // written to the OASIS file.  Name records are not legal inside
    // cells, so we should buffer all elements in the structure, and
    // write the CELL record only when we reach the end of the
    // structure, just before writing the OASIS element records.
    //
    // But if immediateNames is false we buffer only a limited number of
    // elements, and may need to write out elements in the middle of the
    // structure.  So we should write the CELL record right now.

    if (! convOptions.immediateNames)
        beginCell();
}



// The following beginFoo() methods for elements have roughly the same
// structure.
//   1. Allocate from the free store an instance of the appropriate
//      subclass of ElementInfo, e.g., PathInfo for PATH elements.
//   2. Set currElement to this immediately so that the instance has
//      an owner.
//   3. Fill in the instance fields.
//   4. Set currElemPos to the element's position.
//
// After calling beginFoo(), GdsParser calls addProperty() for each
// element property (if any) and then endElement().  addProperty() adds
// the property to currElement's property list.  endElement() then hands
// currElement to the appropriate manager, which takes ownership of it.
//
// All the element functions ignore the ELFLAGS and PLEX records in
// the GDSII file.



/*virtual*/ void
GdsToOasisConverter::beginBoundary (int layer, int datatype,
                                    const GdsPointList&  points,
                                    const GdsElementOptions&)
{
    TrapezoidInfo       trap;

    // A boundary in GDSII is a polygon in OASIS.  But OASIS also
    // defines separate records for two special cases: rectangles and
    // trapezoids.  Because these records are more compact, we check
    // each boundary to see if it can be represented by a rectangle
    // or trapezoid.
    //
    // tryTrapezoid() returns true if the polygon is a trapezoid.  The
    // call stores the trapezoid parameters in trap and its position in
    // currElemPos.  A rectangle is in turn a special case of a
    // trapezoid for which delta_a and delta_b are 0.  See
    // oasis/trapezoid.cc for the meaning of those two deltas.

    if (tryTrapezoid(points, &trap, &currElemPos)) {
        if (trap.delta_a == 0  &&  trap.delta_b == 0) {
            RectangleInfo*  rect = new RectangleInfo;
            setCurrentElement(rect, ElemRectangle);

            rect->width = trap.width;
            rect->height = trap.height;
            rect->layer = layer;
            rect->datatype = datatype;
        }
        else {
            TrapezoidInfo*  newTrap = new TrapezoidInfo(trap);
            setCurrentElement(newTrap, ElemTrapezoid);
            newTrap->layer = layer;
            newTrap->datatype = datatype;
        }
        return;
    }

    PolygonInfo*  polygon = new PolygonInfo;
    setCurrentElement(polygon, ElemPolygon);
    currElemPos.x = points[0].x;
    currElemPos.y = points[0].y;
    polygon->layer = layer;
    polygon->datatype = datatype;
    convertPointList(points, true, &polygon->ptlist);
}



/*virtual*/ void
GdsToOasisConverter::beginBox (int layer, int boxtype,
                               const GdsPointList&  points,
                               const GdsElementOptions&)
{
    // Translate a GDSII box into an OASIS rectangle.
    // XXX: There is some doubt about whether it is correct to do that.

    RectangleInfo*  rect = new RectangleInfo;
    setCurrentElement(rect, ElemRectangle);
    rect->layer = layer;
    rect->datatype = boxtype;           // XXX: is this okay?

    // Exploit the fact that points[0] == points[4] to simplify the
    // code.  Set pt to the address of either points[0] or points[1],
    // whichever makes the edge pt[0]-pt[1] horizontal.

    const GdsPoint*  pt = &*points.begin();
    if (pt[0].y != pt[1].y)             // if the first edge is not horizontal,
        ++pt;                           // the next edge must be
    assert (pt[0].y == pt[1].y);

    // To make sense of the calculations below, note that because
    // 0-1 is horizontal the vertex numbering with respect to pt must
    // match one of these:
    //    0 1    1 0    3 2    2 3
    //    3 2    2 3    0 1    1 0

    rect->width  = abs(CheckedMinus(pt[1].x, pt[0].x));
    rect->height = abs(CheckedMinus(pt[2].y, pt[1].y));
    currElemPos.x = min(pt[0].x, pt[1].x);
    currElemPos.y = min(pt[1].y, pt[2].y);
}



/*virtual*/ void
GdsToOasisConverter::beginPath (int layer, int datatype,
                                const GdsPointList&  points,
                                const GdsPathOptions&  options)
{
    // In GDSII, a negative width is absolute, i.e., unaffected by
    // magnification of the parent sref/aref.  Because OASIS cannot
    // represent absolute widths, convert them to relative.

    int  width = options.getWidth();
    if (width < 0) {
        warn("absolute path width %d converted to relative", width);
        if (width == INT_MIN)
            ++width;            // avoid overflow when negating
        width = -width;
    }

    // OASIS specifies the path halfwidth instead of the width.  For odd
    // widths we set halfwidth = ceil(width/2) rather than
    // floor(width/2).  The latter would convert width 1 to halfwidth 0,
    // which might be treated specially.

    int  halfwidth = width/2;
    if (IsOdd(width)) {
        // warn("odd path width %d incremented to make it even", width);
        ++halfwidth;
    }

    // Set OASIS start/end extensions from GDSII pathtype and begin/end
    // extensions.  Because OASIS does not support rounded ends, convert
    // them to square ends extending by the halfwidth amount.

    int  startExtn = 0, endExtn = 0;
    switch (options.getPathtype()) {
        case PathtypeFlush:
            // nothing to do
            break;

        case PathtypeRound:
            warn("OASIS does not support PATHTYPE 1 (round-ended); "
                 "changed to 2 (square-ended)");
            // fall through
        case PathtypeExtend:
            startExtn = endExtn = halfwidth;
            break;

        case PathtypeCustom:
            // GDSII neither requires BGNEXTN and ENDEXTN to appear nor
            // specifies defaults for them.  Let both default to 0 here.
            //
            if (options.contains(GdsPathOptions::REC_BGNEXTN))
                startExtn = options.getBeginExtn();
            if (options.contains(GdsPathOptions::REC_ENDEXTN))
                endExtn = options.getEndExtn();
            break;

        default:
            assert (false);
    }

    // If the path has just one segment and the segment is horizontal or
    /// vertical, its outline is a rectangle.  So some GDSII tools
    // generate a PATH for a rectangle because that is more compact than
    // the equivalent BOUNDARY.  For other tools PATH and BOUNDARY have
    // different semantics.
    //
    // In OASIS a RECTANGLE is more compact than a PATH.  So if the
    // application allows it, try to convert the path into a RECTANGLE.
    // A zero-width path may be treated specially, so don't convert it
    // into a rectangle.

    if (convOptions.pathToRectangle && points.size() == 2
            &&  (points[0].x == points[1].x  ||  points[0].y == points[1].y)
            &&  halfwidth > 0) {
        RectangleInfo*  rect = new RectangleInfo;
        setCurrentElement(rect, ElemRectangle);
        rect->layer = layer;
        rect->datatype = datatype;
        pathToRectangle(points, halfwidth, startExtn, endExtn,
                        &rect->width, &rect->height, &currElemPos);
        return;
    }
    // Otherwise convert it into a PATH.

    PathInfo*  path = new PathInfo;
    setCurrentElement(path, ElemPath);
    currElemPos = points[0];

    path->layer     = layer;
    path->datatype  = datatype;
    path->halfwidth = halfwidth;
    path->startExtn = startExtn;
    path->endExtn   = endExtn;
    convertPointList(points, false, &path->ptlist);
}



/*virtual*/ void
GdsToOasisConverter::beginSref (const char* sname,
                                int x, int y,
                                const GdsTransform&  strans,
                                const GdsElementOptions&)
{
    // An SREF in GDSII is a PLACEMENT in OASIS.

    SrefInfo*  sref = new SrefInfo;
    setCurrentElement(sref, ElemSref);
    currElemPos.x = x;
    currElemPos.y = y;

    sref->cellIndex = makeCellName(sname);
    convertTransform("SREF", sname, strans, &sref->transform);
}



/*virtual*/ void
GdsToOasisConverter::beginAref (const char*  sname,
                                Uint numCols, Uint numRows,
                                const GdsPointList&  points,
                                const GdsTransform&  strans,
                                const GdsElementOptions&  options)
{
    // A 1x1 AREF is just an SREF, so delegate this to beginSref().  If
    // convertTransform() there prints a warning message it will say
    // SREF instead of AREF, but that is no big deal.

    if (numCols == 1  &&  numRows == 1) {
        beginSref(sname, points[0].x, points[0].y, strans, options);
        return;
    }

    // An AREF in GDSII is also a PLACEMENT in OASIS, but with a
    // repetition.

    ArefInfo*  aref = new ArefInfo;
    setCurrentElement(aref, ElemAref);
    currElemPos = points[0];

    aref->cellIndex = makeCellName(sname);
    convertTransform("AREF", sname, strans, &aref->transform);

    // points[1] is displaced from point[0] by the inter-column spacing
    // times the number of columns.  For OASIS, ndisp should be just the
    // inter-column spacing, so divide by the number of columns.
    // Similarly for mdisp and the inter-row spacing.

    Delta  ndisp(CheckedMinus(points[1].x, points[0].x),
                 CheckedMinus(points[1].y, points[0].y));
    ndisp /= numCols;

    Delta  mdisp(CheckedMinus(points[2].x, points[0].x),
                 CheckedMinus(points[2].y, points[0].y));
    mdisp /= numRows;

    // If there is only one row or column, pick whichever of UnformX,
    // UniformY, or Diagonal is appropriate.  Otherwise pick Matrix if
    // the array is parallel to both axes.  Fall back on TiltedMatrix,
    // the most general kind of array, if nothing else suits.
    // OASIS does not forbid zero spacing.

    if (numRows == 1) {
        if (ndisp.x >= 0  &&  ndisp.y == 0)     // mdisp is irrelevant
            aref->rep.makeUniformX(numCols, ndisp.x);
        else
            aref->rep.makeDiagonal(numCols, ndisp);
    }
    else if (numCols == 1) {
        if (mdisp.x == 0  &&  mdisp.y >= 0)     // ndisp is irrelevant
            aref->rep.makeUniformY(numRows, mdisp.y);
        else
            aref->rep.makeDiagonal(numRows, mdisp);
    }
    else if (ndisp.x >= 0  &&  ndisp.y == 0
             &&  mdisp.x == 0  &&  mdisp.y >= 0)
        aref->rep.makeMatrix(numCols, numRows, ndisp.x, mdisp.y);
    else
        aref->rep.makeTiltedMatrix(numCols, numRows, ndisp, mdisp);
}



/*virtual*/ void
GdsToOasisConverter::beginNode (int, int,
                                const GdsPointList&,
                                const GdsElementOptions&)
{
    warn("NODE element ignored because there is no OASIS equivalent");
}



/*virtual*/ void
GdsToOasisConverter::beginText (int layer, int texttype,
                                int x, int y,
                                const char*  str,
                                const GdsTransform&  strans,
                                const GdsTextOptions&  options)
{
    // GDSII allows texts to be transformed (rotated/magnified/flipped)
    // but OASIS does not.  If the GDSII file contains a text element
    // with a transform, we have two choices.  One is to simply ignore
    // the transform.  We do this if convOptions.transformTexts is false.
    // If it is true, we put each transformed text in its own cell (the
    // text-cell) and write a placement of that text-cell with a
    // transform wherever the text is placed.
    //
    // For transformed texts, XTextInfo::cellIndex, which identifies the
    // text-cell that will contain the TEXT element, is assigned only in
    // writeTransformedTexts().  It should not be assigned here because
    // that would prevent XTextManager from merging otherwise identical
    // transformed texts.
    //
    // XXX:
    // If two transformed texts differ only in the transform, only one
    // text-cell needs to be created because the part that differs will
    // be in the PLACEMENT record, not the TEXT record in the text-cell.
    // Both PLACEMENTs can refer to the single text-cell.  The current
    // design creates a separate text-cell for each text.  Should see if
    // transformed texts occur often enough to justify trying to reuse
    // text-cells.

    TextInfo*   text;

    TextStringIndex  index = makeTextString(str);
    if (! convOptions.transformTexts  ||  strans.isIdentity()) {
        text = new TextInfo;
        setCurrentElement(text, ElemNormalText);
    } else {                            // transformed text
        XTextInfo*  xtext = new XTextInfo;
        setCurrentElement(xtext, ElemTransformedText);
        convertTransform("TEXT", str, strans, &xtext->transform);
        text = xtext;
    }
    currElemPos.x = x;
    currElemPos.y = y;

    text->textIndex = index;
    text->layer     = layer;
    text->texttype  = texttype;
    text->setPresentation(options.getFont(),
                          options.getHorizJustify(),
                          options.getVertJustify() );

    // For TEXT elements we translate the pathtype and width into OASIS
    // properties because TEXT records in OASIS do not have any fields
    // for them.  So there is no need to check for PathtypeRound or to
    // convert width to halfwidth, as we do for paths.

    text->pathtype = options.getPathtype();
    text->width    = options.getWidth();
}



/*virtual*/ void
GdsToOasisConverter::addProperty (int attr, const char* value)
{
    // For elements like NODE that are being ignored, ignore the
    // properties too.

    if (currElement == Null)
        return;

    // Otherwise add the property attr and value to the element.  We
    // create a PropString for the value because we want the PropString
    // table in the OASIS file to be strict, and so all string values of
    // properties must be references to PropStrings.  We want the table
    // to be strict because OasisParser jumps directly to the name
    // tables (avoiding a preliminary pass of the whole file just to
    // collect names) only if _all_ name tables are strict.
    //
    // XXX: These PropStrings must exist until the OASIS file is closed.
    // If a GDSII file has lots of unique property values, the
    // PropStrings may use too much space.  Should measure this overhead
    // for real input files.

    PropStringIndex  index = makePropString(value);
    currElement->propList.addProperty(attr, index);
}



/*virtual*/ void
GdsToOasisConverter::endElement()
{
    // If the element was a NODE, we have nothing to do.
    if (currElement == Null)
        return;

    // Otherwise give the element to the manager for that type of
    // element.

    switch (currElemType) {
        case ElemSref:
            srefMgr.add(static_cast<SrefInfo*>(currElement), currElemPos);
            break;
        case ElemAref:
            arefMgr.add(static_cast<ArefInfo*>(currElement), currElemPos);
            break;
        case ElemNormalText:
            textMgr.add(static_cast<TextInfo*>(currElement), currElemPos);
            break;
        case ElemTransformedText:
            xtextMgr.add(static_cast<XTextInfo*>(currElement), currElemPos);
            break;
        case ElemRectangle:
            rectMgr.add(static_cast<RectangleInfo*>(currElement), currElemPos);
            break;
        case ElemTrapezoid:
            trapMgr.add(static_cast<TrapezoidInfo*>(currElement), currElemPos);
            break;
        case ElemPolygon:
            polygonMgr.add(static_cast<PolygonInfo*>(currElement),currElemPos);
            break;
        case ElemPath:
            pathMgr.add(static_cast<PathInfo*>(currElement), currElemPos);
            break;
        default:
            assert (false);
    }

    // Either the manager now owns the element or it has deleted the
    // element.  In any case we no longer own it.

    currElement = Null;

    // If we have accumulated enough elements, write out the OASIS
    // equivalents.  But do this only if immediateNames is false.  If
    // it is true we must buffer all elements in the structure.

    if (! convOptions.immediateNames) {
        if (++numBufferedElements > MaxBufferedElements) {
            writeElements();
            if (convOptions.verbose)
                displayMessage("... processing new batch of %u elements",
                               MaxBufferedElements);
        }
    }
}



/*virtual*/ void
GdsToOasisConverter::endStructure()
{
    writeElements();    // flush all buffered elements
    creator.endCell();
    writeTextCells();   // write text-cell for each transformed text
}



// endLibrary -- invoked at the end of the GDSII file

/*virtual*/ void
GdsToOasisConverter::endLibrary()
{
    creator.endFile();
}



//----------------------------------------------------------------------
//                    Writing elements to the OASIS file
//----------------------------------------------------------------------

// beginCell -- start new cell in OASIS file
// This is called from beginStructure() when immediateNames is false, and
// from endStructure() via writeElements() when immediateNames is true.

void
GdsToOasisConverter::beginCell()
{
    creator.beginCell(getCellName(currentCell));
    creator.setXYrelative(convOptions.relativeMode);
}



// writeElements -- write out the elements buffered in the managers.
// This is invoked when we have accumulated enough elements, and at the
// end of each GDSII structure.

void
GdsToOasisConverter::writeElements()
{
    // Each transformed text is put into its own cell.  Create the
    // CellNames for those cells.  We must do that before writing the
    // CELL record when immediateNames is true.  Otherwise registering
    // those CellNames would result in CELLNAME record appearing inside
    // a cell, which is illegal.

    XTextManager::iterator  iter = xtextMgr.begin();
    XTextManager::iterator  end  = xtextMgr.end();
    for ( ;  iter != end;  ++iter) {
        XTextInfo*  xtext = *iter;
        xtext->cellIndex = generateTextCellName();
    }

    if (convOptions.immediateNames)
        beginCell();

    writeSrefs();
    writeArefs();
    writeNormalTexts();
    writeTransformedTexts();
    writeRectangles();
    writeTrapezoids();
    writePolygons();
    writePaths();

    // The number of elements now buffered in the managers is 0.
    // Although the XTextInfo objects still exist, they have been
    // moved from xtextMgr to the xtexts vector.

    numBufferedElements = 0;
}



// Each of the writeFoo() methods below takes the eleminfo structs from
// an element manager and invokes OasisCreator methods to write out the
// element and its properties.  Because each element manager merges
// identical eleminfos, for each eleminfo we have a set of positions
// at which it occurs.
//
// We must use OASIS features to make the file compact.  We do this in
// two steps.  First we sort the eleminfos in the manager so that we
// write them in some order that will exploit the OASIS modal variables.
// Then for each eleminfo we use the class PointGrouper to group its set
// of positions into one or more OASIS repetitions.  That is, we write
// each eleminfo as one or more OASIS elements with repetition
// specifications so that the total file space used is minimized.
//
// If xy-mode is relative, we must also keep track of the position of
// the last element written for each class of element, and verify that
// the new position is within range of the previous one.  If it is not,
// we switch temporarily to absolute xy-mode, write the new element, and
// switch back to relative mode.
//
// Note that with this design most of the element managers' sort
// routines cannot try to order the eleminfos to reuse the repetition
// field, because the Repetition objects are constructed after the
// eleminfos are sorted.  Only ArefManager knows what the repetition
// fields will contain.


void
GdsToOasisConverter::writeSrefs()
{
    if (convOptions.optLevel > 0)
        srefMgr.sort();

    SrefManager::iterator  iter = srefMgr.begin(),
                           end  = srefMgr.end();
    for ( ;  iter != end;  ++iter) {
        SrefInfo*  sref = *iter;
        CellName*  cellName = getCellName(sref->cellIndex);
        Delta  pos;

        PointGrouper  pg(sref->positions,
                         convOptions.optLevel, convOptions.deleteDuplicates);
        while (! pg.empty()) {
            const Repetition*  rep = pg.makeRepetition(&pos);
            if (convOptions.relativeMode
                    &&  ! PointInReach(placementPos, pos))
                creator.setXYrelative(false);
            creator.beginPlacement(cellName, pos.x, pos.y,
                                   Oreal(sref->transform.getMag()),
                                   Oreal(sref->transform.getAngle()),
                                   sref->transform.isFlipped(),
                                   rep);
            writePropertyList(sref->propList);
            placementPos.assign(pos.x, pos.y);
            creator.setXYrelative(convOptions.relativeMode);
        }
    }

    srefMgr.deleteAll();
}



void
GdsToOasisConverter::writeArefs()
{
    if (convOptions.optLevel > 0)
        arefMgr.sort();

    // Arefs are not grouped because each of them already has a repetition.

    ArefManager::iterator  iter = arefMgr.begin(),
                           end  = arefMgr.end();
    for ( ;  iter != end;  ++iter) {
        ArefInfo*  aref = *iter;
        CellName*  cellName = getCellName(aref->cellIndex);
        Delta  pos(aref->pos.x, aref->pos.y);
        if (convOptions.relativeMode  &&  ! PointInReach(placementPos, pos))
            creator.setXYrelative(false);
        creator.beginPlacement(cellName, pos.x, pos.y,
                               Oreal(aref->transform.getMag()),
                               Oreal(aref->transform.getAngle()),
                               aref->transform.isFlipped(),
                               &aref->rep);
        writePropertyList(aref->propList);
        placementPos.assign(pos.x, pos.y);
        creator.setXYrelative(convOptions.relativeMode);
    }

    arefMgr.deleteAll();
}



void
GdsToOasisConverter::writeNormalTexts()
{
    if (convOptions.optLevel > 0)
        textMgr.sort();

    TextManager::iterator  iter = textMgr.begin(),
                           end  = textMgr.end();
    for ( ;  iter != end;  ++iter) {
        TextInfo*  text = *iter;
        TextString*  textString = getTextString(text->textIndex);
        Delta  pos;

        PointGrouper  pg(text->positions,
                         convOptions.optLevel, convOptions.deleteDuplicates);
        while (! pg.empty()) {
            const Repetition*  rep = pg.makeRepetition(&pos);
            if (convOptions.relativeMode  &&  ! PointInReach(textPos, pos))
                creator.setXYrelative(false);

            creator.beginText(text->layer, text->texttype, pos.x, pos.y,
                              textString, rep);
            writePropertyList(text->propList);
            writeTextPresentationProperties(text);

            textPos.assign(pos.x, pos.y);
            creator.setXYrelative(convOptions.relativeMode);
        }
    }

    textMgr.deleteAll();
}



void
GdsToOasisConverter::writeTransformedTexts()
{
    if (convOptions.optLevel > 0)
        xtextMgr.sort();

    XTextManager::iterator  iter = xtextMgr.begin(),
                            end  = xtextMgr.end();
    for ( ;  iter != end;  ++iter) {
        XTextInfo*  xtext = *iter;
        CellName*  cellName = getCellName(xtext->cellIndex);
        Delta  pos;

        PointGrouper  pg(xtext->positions,
                         convOptions.optLevel, convOptions.deleteDuplicates);
        while (! pg.empty()) {
            const Repetition*  rep = pg.makeRepetition(&pos);
            if (convOptions.relativeMode
                    &&  ! PointInReach(placementPos, pos))
                creator.setXYrelative(false);
            creator.beginPlacement(cellName, pos.x, pos.y,
                                   Oreal(xtext->transform.getMag()),
                                   Oreal(xtext->transform.getAngle()),
                                   xtext->transform.isFlipped(),
                                   rep);

            // Do not write the properties here.  writeTextCells() writes
            // them when it writes the TEXT elements.

            placementPos.assign(pos.x, pos.y);
            creator.setXYrelative(convOptions.relativeMode);
        }
    }

    // We process transformed texts differently from the other elements.
    // Instead of deleting the XTextInfo elements, we move them to the
    // vector 'xtexts' in this class.  We need to retain the elements
    // until the end of the structure so that we can write the cell for
    // each.  Because xtexts is an owning vector, we must make sure no
    // exception can occur during the move; otherwise both xtexts and
    // xtextMgr might try to delete the same XTextInfo objects.  That is
    // why we reserve space in xtexts before copying the elements.

    xtexts.reserve(xtextMgr.numUniqueElements());
    iter = xtextMgr.begin();
    end  = xtextMgr.end();
    for ( ;  iter != end;  ++iter)
        xtexts.push_back(*iter);

    // Just remove the elements from xtextMgr instead of deleting them.
    xtextMgr.clear();
}



void
GdsToOasisConverter::writeRectangles()
{
    if (convOptions.optLevel > 0)
        rectMgr.sort();

    RectManager::iterator  iter = rectMgr.begin(),
                           end  = rectMgr.end();
    for ( ;  iter != end;  ++iter) {
        RectangleInfo*  rect = *iter;
        Delta  pos;

        PointGrouper  pg(rect->positions,
                         convOptions.optLevel, convOptions.deleteDuplicates);
        while (! pg.empty()) {
            const Repetition*  rep = pg.makeRepetition(&pos);
            if (convOptions.relativeMode  &&  ! PointInReach(geometryPos, pos))
                creator.setXYrelative(false);

            creator.beginRectangle(rect->layer, rect->datatype,
                                   pos.x, pos.y, rect->width, rect->height,
                                   rep);
            writePropertyList(rect->propList);

            geometryPos.assign(pos.x, pos.y);
            creator.setXYrelative(convOptions.relativeMode);
        }
    }

    rectMgr.deleteAll();
}



void
GdsToOasisConverter::writeTrapezoids()
{
    if (convOptions.optLevel > 0)
        trapMgr.sort();

    TrapezoidManager::iterator  iter = trapMgr.begin(),
                                end  = trapMgr.end();
    for ( ;  iter != end;  ++iter) {
        TrapezoidInfo*  trap = *iter;
        Delta  pos;
        Oasis::Trapezoid  otrapezoid(trap->orient, trap->width, trap->height,
                                     trap->delta_a, trap->delta_b);
        PointGrouper  pg(trap->positions,
                         convOptions.optLevel, convOptions.deleteDuplicates);
        while (! pg.empty()) {
            const Repetition*  rep = pg.makeRepetition(&pos);
            if (convOptions.relativeMode  &&  ! PointInReach(geometryPos, pos))
                creator.setXYrelative(false);

            creator.beginTrapezoid(trap->layer, trap->datatype,
                                   pos.x, pos.y, otrapezoid, rep);
            writePropertyList(trap->propList);

            geometryPos.assign(pos.x, pos.y);
            creator.setXYrelative(convOptions.relativeMode);
        }
    }

    trapMgr.deleteAll();
}



void
GdsToOasisConverter::writePolygons()
{
    if (convOptions.optLevel > 0)
        polygonMgr.sort();

    PolygonManager::iterator  iter = polygonMgr.begin(),
                              end  = polygonMgr.end();
    for ( ;  iter != end;  ++iter) {
        PolygonInfo*  polygon = *iter;
        Delta  pos;

        PointGrouper  pg(polygon->positions,
                         convOptions.optLevel, convOptions.deleteDuplicates);
        while (! pg.empty()) {
            const Repetition*  rep = pg.makeRepetition(&pos);
            if (convOptions.relativeMode  &&  ! PointInReach(geometryPos, pos))
                creator.setXYrelative(false);

            creator.beginPolygon(polygon->layer, polygon->datatype,
                                 pos.x, pos.y, polygon->ptlist, rep);
            writePropertyList(polygon->propList);

            geometryPos.assign(pos.x, pos.y);
            creator.setXYrelative(convOptions.relativeMode);
        }
    }

    polygonMgr.deleteAll();
}



void
GdsToOasisConverter::writePaths()
{
    if (convOptions.optLevel > 0)
        pathMgr.sort();

    PathManager::iterator  iter = pathMgr.begin(),
                           end  = pathMgr.end();
    for ( ;  iter != end;  ++iter) {
        PathInfo*  path = *iter;
        Delta  pos;

        PointGrouper  pg(path->positions,
                         convOptions.optLevel, convOptions.deleteDuplicates);
        while (! pg.empty()) {
            const Repetition*  rep = pg.makeRepetition(&pos);
            if (convOptions.relativeMode  &&  ! PointInReach(geometryPos, pos))
                creator.setXYrelative(false);

            creator.beginPath(path->layer, path->datatype, pos.x, pos.y,
                              path->halfwidth, path->startExtn, path->endExtn,
                              path->ptlist, rep);
            writePropertyList(path->propList);

            geometryPos.assign(pos.x, pos.y);
            creator.setXYrelative(convOptions.relativeMode);
        }
    }

    pathMgr.deleteAll();
}



// writeTextPresentationProperties -- write TEXT presentation as OASIS props.
// The presentation attributes of TEXT elements in GDSII have no
// equivalents in the TEXT records of OASIS.  So we store those
// attributes as properties of the OASIS TEXT elements.

void
GdsToOasisConverter::writeTextPresentationProperties (const TextInfo* text)
{
    // Write the GDS_PRESENTATION property if any component of the
    // presentation differs from the default.  Instead of creating an
    // Oasis::Property object from scratch, reuse the cached property
    // created in the constructor, overwriting its values.
    //
    // The casts are needed to resolve the overloaded PropValue::assign().

    if (text->getFont() != 0
            ||  text->getVertJustify()  != GdsTextOptions::JustifyTop
            ||  text->getHorizJustify() != GdsTextOptions::JustifyLeft)
    {
        PropValue*  propval;

        propval = propPresentation->getValue(0);
        propval->assign(PV_UnsignedInteger,
                        static_cast<Ullong>(text->getFont()) );

        propval = propPresentation->getValue(1);
        propval->assign(PV_UnsignedInteger,
                        static_cast<Ullong>(text->getVertJustify()) );

        propval = propPresentation->getValue(2);
        propval->assign(PV_UnsignedInteger,
                        static_cast<Ullong>(text->getHorizJustify()) );

        creator.addElementProperty(propPresentation);
    }

    // Similarly write GDS_PATHTYPE and GDS_WIDTH if they differ from
    // the default value.

    if (text->pathtype != PathtypeFlush) {
        propPathtype->getValue(0)->assign(PV_UnsignedInteger,
                                          static_cast<Ullong>(text->pathtype) );
        creator.addElementProperty(propPathtype);
    }
    if (text->width != 0) {
        propWidth->getValue(0)->assign(PV_SignedInteger,
                                       static_cast<llong>(text->width) );
        creator.addElementProperty(propWidth);
    }
}



// writePropertyList -- write GDSII properties as OASIS properties

void
GdsToOasisConverter::writePropertyList (const PropInfoList& propList)
{
    PropInfoList::const_iterator  iter = propList.begin(),
                                  end  = propList.end();
    for ( ;  iter != end;  ++iter) {
        const PropInfo&  prop = *iter;
        propProperty->getValue(0)->assign(PV_UnsignedInteger,
                                          static_cast<Ullong>(prop.attr));
        propProperty->getValue(1)->assign(PV_Ref_BinaryString,
                                          getPropString(prop.value));
        creator.addElementProperty(propProperty);
    }
}



// writeTextCells -- write the special cells created to hold transformed texts

void
GdsToOasisConverter::writeTextCells()
{
    // Typical case
    if (xtexts.empty())
        return;

    // Each cell we create will contain a single TEXT element and its
    // properties.  We gain nothing by compressing such tiny cells.
    // We also need not bother about convOptions.relativeMode.

    creator.setCompression(false);

    // For each transformed TEXT, create a cell to hold it, put the TEXT
    // element in it at (0, 0), and add its properties.  The positions
    // and repetition in the XTextInfo are irrelevant here.  We took
    // care of them when writing the PLACEMENTs for the cells to be
    // created here.

    PointerVector<XTextInfo>::iterator  iter = xtexts.begin();
    PointerVector<XTextInfo>::iterator  end  = xtexts.end();
    for ( ;  iter != end;  ++iter) {
        XTextInfo*  xtext = *iter;
        CellName*  cellName = getCellName(xtext->cellIndex);
        creator.beginCell(cellName);
        creator.beginText(xtext->layer, xtext->texttype,
                          0, 0,
                          getTextString(xtext->textIndex),
                          Null);
        writePropertyList(xtext->propList);
        writeTextPresentationProperties(xtext);
        creator.endCell();
    }
    DeleteContainerElements(xtexts);

    creator.setCompression(convOptions.compress);
}


//----------------------------------------------------------------------


// makeProperty -- create an Oasis::Property object with a given name
//   pname      property name
//   isStd      whether it is a standard OASIS property
//
// Returns a pointer to the Property object, which is allocated in the
// free store.  This is a helper function for the constructor.

Property*
GdsToOasisConverter::makeProperty (const char* pname, bool isStd)
{
    // The Property constructor wants the property name in a PropName*,
    // not a plain string.  We remain responsible for the PropName
    // object (Property does not take ownership).  So we add it to the
    // owning vector propNames, a class member.  We need the usual dance
    // to ensure that the PropName and Property have exactly one owner
    // anytime an exception might occur.

    auto_ptr<PropName>  autoName(new PropName(string(pname)));
    propNames.push_back(autoName.get());
    PropName*  propName = autoName.release();
    creator.registerPropName(propName);

    auto_ptr<Property> autoProp(new Property(propName, isStd));
    properties.push_back(autoProp.get());
    return autoProp.release();
}



// makeCellName -- create CellName object and register it with the creator
//   structureName      name of cell
//
// If a CellName with the given name already exists, its index returned.


CellNameIndex
GdsToOasisConverter::makeCellName (const char* structureName)
{
    // If a CellName with the given name already exists, return its index.

    StringToCellNameMap::iterator  iter = cellNameMap.find(structureName);
    if (iter != cellNameMap.end())
        return iter->second;

    // No existing CellName matches, so create a new CellName and add it
    // to both the vector and the map.  Because the vector owns the
    // CellNames in it, release the CellName from the auto_ptr
    // as soon as it is in the vector.
    //
    // Note that the map's key (a const char*) must point to the
    // characters in the CellName itself.  Using structureName as the
    // key will not work because GdsParser owns the space structureName
    // points to, and will overwrite it later.

    auto_ptr<CellName>  acn(new CellName(string(structureName)));
    allCellNames.push_back(acn.get());
    CellName*  cellName = acn.release();
    CellNameIndex  index = allCellNames.size() - 1;
    cellNameMap[cellName->getName().c_str()] = index;

    // Register the name with the creator, which then assigns the name a
    // reference-number and writes that instead of the name.  We must
    // register all names to let the creator use strict name tables.

    creator.registerCellName(cellName);

    return index;
}



TextStringIndex
GdsToOasisConverter::makeTextString (const char* str)
{
    StringToTextStringMap::iterator  iter = textStringMap.find(str);
    if (iter != textStringMap.end())
        return iter->second;

    // XXX: verify string is ASCII?
    auto_ptr<TextString>  ats(new TextString(string(str)));
    allTextStrings.push_back(ats.get());

    TextString*  textString = ats.release();
    TextStringIndex  index = allTextStrings.size() - 1;
    textStringMap[textString->getName().c_str()] = index;
    creator.registerTextString(textString);
    return index;
}



PropStringIndex
GdsToOasisConverter::makePropString (const char* str)
{
    StringToPropStringMap::iterator  iter = propStringMap.find(str);
    if (iter != propStringMap.end())
        return iter->second;

    auto_ptr<PropString>  aps(new PropString(string(str)));
    allPropStrings.push_back(aps.get());

    // We create PropStrings only for the (string) values of GDSII
    // properties.  Because GDSII strings cannot contain NULs, it
    // is okay to use the C-string as the key.

    PropString*  propString = aps.release();
    PropStringIndex  index = allPropStrings.size() - 1;
    propStringMap[propString->getName().c_str()] = index;
    creator.registerPropString(propString);
    return index;
}



// generateTextCellName -- create CellName for cell holding transformed text.
// Creates a CellName with a name that we hope is unique, and returns
// its index.

CellNameIndex
GdsToOasisConverter::generateTextCellName()
{
    // There is no point in trying to verify that the cell name we
    // generate is unused because we have to write the cell before
    // processing the entire input file.  The best we can do without two
    // passes of the GDSII file is to choose a bizarre name and hope
    // that it does not appear in the input.

    char  buf[80];
    SNprintf(buf, sizeof buf, "!!TeXt-CeLl-%u!!", nextTextCellNum);
    ++nextTextCellNum;
    return (makeCellName(buf));
}



// convertTransform -- convert a GDSII transform to the intermediate form
//   elemType   type of GDSII element containing the transform (SREF, AREF,
//              or TEXT).  Used only for the warning message, if any.
//   elemName   for srefs and arefs, the name of the referenced structure;
//              for texts, the text string.
//              Used only for the warning message, if any.
//   strans     GDSII transform
//   transform  out: the internal transform is stored here
//
// This is invoked by beginSref(), beginAref(), and beginText().

void
GdsToOasisConverter::convertTransform (const char* elemType,
                                       const char* elemName,
                                       const GdsTransform& strans,
                                       /*out*/ TransformInfo* transform)
{
    // In OASIS it is a fatal error if the magnification is 0, but this
    // occurs in some GDSII files.  Change the mag to 1 with a warning.

    double  mag = strans.getMag();
    if (mag == 0.0) {
        warn("%s '%s' has invalid magnification 0.0; changed to 1.0",
             elemType, elemName);
        mag = 1.0;
    }

    // GDSII allows the magnification and angle to be absolute, but OASIS
    // does not.  Convert absolute values to relative with a warning.

    if (strans.angleIsAbsolute())
        warn("%s '%s' has absolute angle; changed to relative",
             elemType, elemName);
    if (strans.magIsAbsolute())
        warn("%s '%s' has absolute magnification; changed to relative",
             elemType, elemName);
    transform->init(mag, strans.getAngle(), strans.isReflected());
}



// convertPointList -- make OASIS point-list out of GDSII XY points

void
GdsToOasisConverter::convertPointList (const GdsPointList& points,
                                       bool isPolygon,
                                       /*out*/ Oasis::PointList* ptlist)
{
    PointList::ListType  ltype = getPointListType(points, isPolygon);
    ptlist->init(ltype);

    // Convert the vertex coordinates in the GdsPointList to offsets
    // with respect to the first, as required by Oasis::PointList.  In
    // GDSII polygons the first point is repeated as the last point; in
    // OASIS polygons it is not.  Note that operator- for Deltas checks
    // for overflow.

    GdsPointList::const_iterator  iter = points.begin();
    GdsPointList::const_iterator  end  = points.end();
    if (isPolygon)
        --end;          // ignore repeated point

    Delta  start(iter->x, iter->y);
    for ( ;  iter != end;  ++iter) {
        Delta  curr(iter->x, iter->y);
        ptlist->addPoint(curr - start);         // may throw overflow_error
    }
}



Oasis::PointList::ListType
GdsToOasisConverter::getPointListType (const GdsPointList& points,
                                       bool isPolygon)
{
    const GdsPoint*  pt = &*points.begin();     // get address of first point
    Uint        numPoints = points.size();
    Uint        j;

    // Try the different types of OASIS point-list in succession,
    // starting with the most compact types, to see which of them can be
    // used to represent the input.

    // Type 0.  Alternating horizontal and vertical edges with first
    // edge horizontal.  Check for zero-length edges because
    // paragraph 7.7.8 of the OASIS spec forbids them.  The number of
    // vertices in a type 0 or 1 polygon should be even and >= 4.
    // Because the first vertex is repeated as the last, that means
    // that numPoints must be odd and >= 5.

    if (!isPolygon  ||  (IsOdd(numPoints)  &&  numPoints >= 5)) {
        for (j = 1;  j < numPoints;  ++j) {
            if (IsOdd(j)) {
                // If edge is not horizontal or has zero length, give up.
                if (pt[j].y != pt[j-1].y  ||  pt[j].x == pt[j-1].x)
                    break;
            } else {
                // If edge is not vertical or has zero length, give up
                if (pt[j].x != pt[j-1].x  ||  pt[j].y == pt[j-1].y)
                    break;
            }
        }
        if (j == numPoints)
            return PointList::ManhattanHorizFirst;
    }

    // Type 1.  Alternating horizontal and vertical edges with first
    // edge vertical.  Same code as for type 0 except that the
    // conditions of the two inner ifs have been interchanged.

    if (!isPolygon  ||  (IsOdd(numPoints)  &&  numPoints >= 5)) {
        for (j = 1;  j < numPoints;  ++j) {
            if (IsOdd(j)) {
                // If edge is not vertical or has zero length, give up
                if (pt[j].x != pt[j-1].x  ||  pt[j].y == pt[j-1].y)
                    break;
            } else {
                // If edge is not horizontal or has zero length, give up.
                if (pt[j].y != pt[j-1].y  ||  pt[j].x == pt[j-1].x)
                    break;
            }
        }
        if (j == numPoints)
            return PointList::ManhattanVertFirst;
    }

    // Type 2.  Manhattan.  Coincident and collinear vertices are
    // permitted here.

    for (j = 1;  j < numPoints;  ++j) {
        if (pt[j].x != pt[j-1].x  &&  pt[j].y != pt[j-1].y)
            break;
    }
    if (j == numPoints)
        return PointList::Manhattan;

    // Type 3.  Octangular.

    for (j = 1;  j < numPoints;  ++j) {
        Delta  delta(CheckedMinus(pt[j].x, pt[j-1].x),
                     CheckedMinus(pt[j].y, pt[j-1].y) );
        if (! delta.isOctangular())
            break;
    }
    if (j == numPoints)
        return PointList::Octangular;

    // Don't bother with AllAngleDoubleDelta because it's too much work.

    return PointList::AllAngle;
}



// tryTrapezoid -- see if points in GDSII boundary form a trapezoid
//   points     GDSII point-list for the boundary
//   trap       out: on success the trapezoid's fields are stored here
//   pos        out: on success the trapezoid's position is stored here
//
// Returns true if the boundary is a trapezoid, false otherwise.
// The contents of *trap and *pos are not defined if this returns false.
//
// Some degenerate trapezoids are not recognized.  For example,
//     horizontal line:  0 0   5 0   0 0   5 0   0 0
//     slanted line:     0 0   5 5   0 0   5 5   0 0
//
// The first is a rectangle and the second a trapezoid, but both get
// translated into polygons.  The chances are that such degenerate
// boundaries don't occur in real GDSII files, so there is no reason to
// struggle to optimise them.

bool
GdsToOasisConverter::tryTrapezoid (const GdsPointList& points,
                                   /*out*/ TrapezoidInfo* trap,
                                   /*out*/ GdsPoint* pos)
{
    // For a BOUNDARY the last point must be the same as the first.  So
    // it can be a trapezoid only if it has 5 points (4 vertices).
    //
    // XXX: Some OASIS ctrapezoids are triangles.  Is it worthwhile to
    // check for those?  Right now we don't even try compressing the
    // trapezoids.

    if (points.size() != 5)
        return false;

    const GdsPoint*  pt = &*points.begin();
    return (tryVerticalTrapezoid(pt,   trap, pos)
         || tryVerticalTrapezoid(pt+1, trap, pos)
         || tryHorizontalTrapezoid(pt,   trap, pos)
         || tryHorizontalTrapezoid(pt+1, trap, pos) );
}



// tryVerticalTrapezoid -- see if four points form a vertical trapezoid
//   pt         array of four points
//   trap       out: on success the trapezoid's fields are stored here
//   pos        out: on success the trapezoid's position is stored here
// Returns true if the point vector forms a vertical trapezoid.

bool
GdsToOasisConverter::tryVerticalTrapezoid (const GdsPoint pt[],
                                           /*out*/ TrapezoidInfo* trap,
                                           /*out*/ GdsPoint* pos)
{
    // Check that the edges 0-1 and 2-3 are vertical.  The vertical
    // edges could be 1-2 and 3-0, but we need not check that because
    // tryTrapezoid() calls this twice, with pt[] offset by one vertex
    // the second time.

    if (pt[0].x != pt[1].x  ||  pt[2].x != pt[3].x)
        return false;

    // Check that the edges 0-3 and 1-2 do not cross (OASIS spec 28.9).
    // We cannot simply verify that (y0 > y1) == (y3 > y2) because two
    // adjacent points may coincide to make the trapezoid a triangle.
    // Oasis::Trapezoid allows that.

    if (pt[0].y > pt[1].y  &&  pt[3].y < pt[2].y)
        return false;
    if (pt[0].y < pt[1].y  &&  pt[3].y > pt[2].y)
        return false;

    /*------------------------------------------------------------------
    At this stage we know that the vertices must be numbered in one
    of these four ways.
       0 3    1 2    3 0    2 1
       1 2    0 3    2 1    3 0
    The first choice might look like one of these:

                       0 +            + 3     0 +              + 3
                         |\          /|         |\            /|
     0 +----------+ 3    | + 3    0 + |         | \          / |
       |          |      | |        | |       1 +  + 3    0 +  + 2
       |          |      | + 2    1 + |          \ |        | /
     1 +----------+ 2    |/          \|           \|        |/
                       1 +            + 2          + 2    1 +

    Or if it's degenerate it might look like one of these:

     0 +              + 3   0 +         + 0,1       + 0
       |\            /|       |\         \          |
       | \      0,1 + |       | + 2,3     \         |         + 0,1,2,3
       |  \          \|       |/           \        |
     1 +---+ 2,3      + 2   1 +             + 2,3   + 1,2,3

     --------------------------------------------------------------------*/

    trap->orient = Oasis::Trapezoid::Vertical;
    trap->width = abs(CheckedMinus(pt[0].x, pt[3].x));
    pos->x = min(pt[0].x, pt[3].x);

    // The Oasis::Trapezoid constructor needs two values delta_a and
    // delta_b.  If we call the four corners NW, NE, SW, SE, then
    //     delta_a = SW.y - SE.y
    //     delta_b = NW.y - NE.y
    // We need to compute these values along with y and height.


    // See if the trapezoid is really a rectangle.  Although the general
    // case below handles rectangles correctly, they occur often enough
    // to justify special treatment.  Setting both deltas to 0
    // identifies the trapezoid as a rectangle.

    if (pt[0].y == pt[3].y  &&  pt[1].y == pt[2].y) {
        pos->y = min(pt[0].y, pt[1].y);
        trap->height = abs(CheckedMinus(pt[0].y, pt[1].y));
        trap->delta_a = 0;
        trap->delta_b = 0;
        return true;
    }

    // Not a rectangle.  The trapezoid's Y position is the minimum of
    // all the vertex Y coordinates and its height is calculated from
    // the maximum of the Ys.

    pos->y = min(min(pt[0].y, pt[1].y), min(pt[2].y, pt[3].y));
    trap->height = CheckedMinus(max(max(pt[0].y, pt[1].y),
                                    max(pt[2].y, pt[3].y)),
                                pos->y);

    // The 0-1 edge may be to the left or right of the 2-3 edge.
    // Similarly the 0-3 edge may be above or below the 1-2 edge.
    // First compute delta_a and delta_b assuming the vertices are
    // positioned as shown in the diagrams above.

    trap->delta_a = CheckedMinus(pt[1].y, pt[2].y);
    trap->delta_b = CheckedMinus(pt[0].y, pt[3].y);

    // If the 0-3 edge is below the 1-2 edge, what we have called
    // delta_a is really delta_b, and vice versa.  Because of the
    // degenerate cases we must check both pairs of endpoints to see
    // which edge is below.

    if (pt[0].y < pt[1].y  ||  pt[3].y < pt[2].y)
        swap(trap->delta_a, trap->delta_b);

    // If the 3-2 edge is to the left of the 0-1 edge, we must negate
    // the values because we have computed E - W instead of W - E.

    if (pt[3].x < pt[0].x) {
        trap->delta_a = - trap->delta_a;
        trap->delta_b = - trap->delta_b;
    }

    return true;
}



bool
GdsToOasisConverter::tryHorizontalTrapezoid (const GdsPoint pt[],
                                             /*out*/ TrapezoidInfo* trap,
                                             /*out*/ GdsPoint*  pos)
{
    // The logic here is the same as in makeVerticalTrapezoid(), with
    // the appropriate changes for horizontal instead of vertical.

    if (pt[0].y != pt[1].y  ||  pt[2].y != pt[3].y)
        return false;

    if (pt[0].x > pt[1].x  &&  pt[3].x < pt[2].x)
        return false;
    if (pt[0].x < pt[1].x  &&  pt[3].x > pt[2].x)
        return false;

    //  0 +---------+ 1   0 +-----+ 1   0 +------+ 1      0 +------+ 1
    //     \       /       /       \       \      \        /      /
    //    3 +-----+ 2   3 +---------+ 2   3 +------+ 2  3 +------+ 2

    trap->orient = Oasis::Trapezoid::Horizontal;
    trap->height = abs(CheckedMinus(pt[0].y, pt[3].y));
    pos->y = min(pt[0].y, pt[3].y);

    // We need not check for a rectangle because makeVerticalTrapezoid()
    // was called before this function and checked for it.

    pos->x = min(min(pt[0].x, pt[1].x), min(pt[2].x, pt[3].x));
    trap->width = CheckedMinus(max(max(pt[0].x, pt[1].x),
                                   max(pt[2].x, pt[3].x)),
                               pos->x);

    // For horizontal trapezoids,
    //     delta_a = NW.x - SW.x
    //     delta_b = NE.x - SE.x

    trap->delta_a = CheckedMinus(pt[0].x, pt[3].x);
    trap->delta_b = CheckedMinus(pt[1].x, pt[2].x);

    if (pt[1].x < pt[0].x  ||  pt[2].x < pt[3].x)
        swap(trap->delta_a, trap->delta_b);

    if (pt[0].y < pt[3].y) {
        trap->delta_a = - trap->delta_a;
        trap->delta_b = - trap->delta_b;
    }

    return true;
}



// pathToRectangle -- compute rect params for single-segment horiz/vert path.
//   points     list of points in path
//   halfwidth, startExtn, endExtn
//              path parameters: half-width, start extension, end extension.
//              startExtn and endExtn may be negative.
//   pWidth     out: addr where rectangle's width should be stored
//   pHeight    out: addr where rectangle's height should be stored
//   pos        out: addr where rectangle's position should be stored
//
// beginPath() calls this for paths that have only two points (i.e.,
// one segment) and are horizontal or vertical.

void
GdsToOasisConverter::pathToRectangle (const GdsPointList& points,
                                      int halfwidth,
                                      int startExtn, int endExtn,
                                      /*out*/ int* pWidth,
                                      /*out*/ int* pHeight,
                                      /*out*/ GdsPoint* pos)
{
    assert (points.size() == 2);
    assert (points[0].x == points[1].x  ||  points[0].y == points[1].y);
    assert (halfwidth > 0  &&  halfwidth <= INT_MAX/2);

    int  totalExtn = CheckedPlus(startExtn, endExtn);
    int  width, height;

    // The rectangle for a horizontal line looks like this, if
    // points[0].x <= points[1].x.  If points[0].x > points[1].x instead,
    // interchange 0 and 1, and startExtn and endExtn.
    //
    //    +----------------------+ <- points[0].y + halfwidth
    //    |                      |
    //    |   +----------+       | <- points[0].y == points[1].y
    //    |                      |
    //    +----------------------+ <- points[0].y - halfwidth
    //    ^   ^          ^       ^
    //    |   |          |       |
    //    |   |          |       points[1].x + endExtn
    //    |   |          points[1].x
    //    |   points[0].x
    //    points[0].x - startExtn

    if (points[0].y == points[1].y) {           // horizontal line
        pos->y = points[0].y - halfwidth;
        height = 2*halfwidth;

        if (points[0].x <= points[1].x) {
            pos->x = CheckedMinus(points[0].x, startExtn);
            width = CheckedMinus(points[1].x, points[0].x);
        } else {
            pos->x = CheckedMinus(points[1].x, endExtn);
            width = CheckedMinus(points[0].x, points[1].x);
        }
        width = CheckedPlus(width, totalExtn);
        if (width < 0)
            width = 0;
    }
    else {                                      // vertical line
        pos->x = points[0].x - halfwidth;
        width = 2*halfwidth;
        if (points[0].y <= points[1].y) {
            pos->y = CheckedMinus(points[0].y, startExtn);
            height = CheckedMinus(points[1].y, points[0].y);
        } else {
            pos->y = CheckedMinus(points[1].y, endExtn);
            height = CheckedMinus(points[0].y, points[1].y);
        }
        height = CheckedPlus(height, totalExtn);
        if (height < 0)
            height = 0;
    }

    *pWidth = width;
    *pHeight = height;
}



// warn -- warn about GDSII features not supported by OASIS
//   fmt        printf() format string for warning message
//   ...        arguments (if any) for the format string
// If the warning handler is non-Null, invokes its warn() method with
// the warning message.
//
// Precondition:
//     setLocator() must have been called earlier.

void
GdsToOasisConverter::warn (const char* fmt, ...)
{
    assert (locator != Null);

    if (warnHandler == Null)
        return;

    char        buf[256];
    const Uint  bufsize = sizeof(buf);

    Uint  n = SNprintf(buf, bufsize, "file '%s'", locator->getFileName());

    const char*  strName = locator->getStructureName();
    if (n < bufsize-1  &&  strName != Null)
        n += SNprintf(buf+n, bufsize-n, ", structure '%s'", strName);

    if (n < bufsize-1) {
        n += SNprintf(buf+n, bufsize-n,
                      ", element at offset %lld:\n    warning: ",
                      static_cast<llong>(locator->getOffset()));
    }

    va_list  ap;
    va_start(ap, fmt);
    if (n < bufsize-1)
        VSNprintf(buf+n, bufsize-n, fmt, ap);
    va_end(ap);

    warnHandler(buf);
}



// displayMessage -- display a message
// This is used for the messages displayed when convOptions.verbose is
// true.  It is like warn(), except that no context is supplied.

void
GdsToOasisConverter::displayMessage (const char* fmt, ...)
{
    char  buf[256];

    va_list  ap;
    va_start(ap, fmt);
    VSNprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (warnHandler != Null)
        warnHandler(buf);
}


//======================================================================
//                         External interface
//======================================================================


// ConvertGdsToOasis
//   infilename         pathname of input GDSII file
//   outfilename        pathname of output OASIS file to create
//   warner             callback to display warnings.  Takes a
//                      const char* argument, the warning message.
//   options            options for the conversion process
// The output file is created with mode (0666 & ~umask).

void
ConvertGdsToOasis (const char* infilename, const char* outfilename,
                   WarningHandler warner, const GdsToOasisOptions& options)
{
    GdsToOasisConverter  converter(outfilename, warner, options);
    GdsParser  parser(infilename, options.gdsFileType, warner);
    parser.parseFile(&converter);
}


}  // namespace GdsiiOasis
