// oasis/modal-vars.h -- state of modal variables defined by OASIS
//
// last modified:   05-Sep-2004  Sun  11:52
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// OasisParser and OasisCreator use the class ModalVars to store the
// state of the modal variables.

#ifndef OASIS_MODAL_VARS_H_INCLUDED
#define OASIS_MODAL_VARS_H_INCLUDED

#include <cassert>
#include "names.h"
#include "oasis.h"

namespace Oasis {

using SoftJin::Ulong;


// ModalVars -- state of modal variables while parsing or creating OASIS files.
// See Section 10 of the OASIS specification.
//
// The size variables geometryWidth, geometryHeight, pathHalfwidth, and
// circleRadius are declared `long' even though they are
// unsigned-integers in the spec.  This is to simplify the checks for
// integer overflow when these sizes are added to other other numbers
// like positions.  OasisParser verifies that numbers assigned to these
// variables are in range before converting from Ulong to long.
//
// The variable propertyIsStandard is not mentioned in the OASIS spec.
// It holds the value of the S bit in the PROPERTY record's info-byte.
// We need the variable to implement record type 29 (RID_PROPERTY_REPEAT),
// which reuses the entire contents of the last PROPERTY record.

class ModalVars {
public:
    // Bits that say which members are defined.  There are no bits for
    // placement[XY], geometry[XY], text[XY], and xyRelative.  Those are
    // always defined.

    enum {
        MV_REPETITION           = 0x0001,
        MV_PLACEMENT_CELL       = 0x0002,
        MV_LAYER                = 0x0004,
        MV_DATATYPE             = 0x0008,

        MV_TEXTLAYER            = 0x0010,
        MV_TEXTTYPE             = 0x0020,
        MV_TEXT_STRING          = 0x0040,

        MV_GEOMETRY_WIDTH       = 0x0080,
        MV_GEOMETRY_HEIGHT      = 0x0100,

        MV_POLYGON_POINTS       = 0x0200,
        MV_PATH_POINTS          = 0x0400,
        MV_PATH_HALFWIDTH       = 0x0800,
        MV_PATH_START_EXTENSION = 0x1000,
        MV_PATH_END_EXTENSION   = 0x2000,

        MV_CTRAPEZOID_TYPE      = 0x4000,
        MV_CIRCLE_RADIUS        = 0x8000,

        MV_LAST_PROPERTY_NAME   = 0x10000,
        MV_PROPERTY_IS_STANDARD = 0x20000,
        MV_LAST_VALUE_LIST      = 0x40000,
    };

private:
    int         mvFlags;                // OR of the MV_xxx enumerators above

    Repetition  repetition;
    long        placementX, placementY;
    CellName*   placementCell;
    Ulong       layer;
    Ulong       datatype;
    long        textX, textY;
    Ulong       textlayer;
    Ulong       texttype;
    TextString* textString;
    long        geometryX, geometryY;
    long        geometryWidth, geometryHeight;
    bool        xyRelative;
    PointList   polygonPoints;
    PointList   pathPoints;
    long        pathHalfwidth;
    long        pathStartExtn, pathEndExtn;
    Uint        ctrapezoidType;
    long        circleRadius;
    PropName*   lastPropertyName;
    bool        propertyIsStandard;
    PropValueVector  lastValueList;

public:
                ModalVars();
                ~ModalVars();
    void        reset();

    // define -- mark a modal variable as being defined
    // isdef  -- check if a modal variable is defined
    // The 'bit' argument to these two methods is one of the MV_* enumerators.

    void        define (int bit)      { mvFlags |= bit;         }
    bool        isdef (int bit) const { return (mvFlags & bit); }

    //--------------------------------------------------
    // set methods

    void        xySetRelative (bool val) { xyRelative = val; }

    void        setPlacementX (long x) { placementX = x; }
    void        setPlacementY (long y) { placementY = y; }
    void        setGeometryX (long x)  { geometryX = x;  }
    void        setGeometryY (long y)  { geometryY = y;  }
    void        setTextX (long x)      { textX = x;      }
    void        setTextY (long y)      { textY = y;      }

    void        setRepetition (const Repetition& rep) {
                    repetition = rep;
                    define(MV_REPETITION);
                }
    void        setPlacementCell (CellName* cellName) {
                    placementCell = cellName;
                    define(MV_PLACEMENT_CELL);
                }
    void        setLayer (Ulong layer) {
                    this->layer = layer;
                    define(MV_LAYER);
                }
    void        setDatatype (Ulong datatype) {
                    this->datatype = datatype;
                    define(MV_DATATYPE);
                }
    void        setTextlayer (Ulong textlayer) {
                    this->textlayer = textlayer;
                    define(MV_TEXTLAYER);
                }
    void        setTexttype (Ulong texttype) {
                    this->texttype = texttype;
                    define(MV_TEXTTYPE);
                }
    void        setTextString (TextString* ts) {
                    textString = ts;
                    define(MV_TEXT_STRING);
                }
    void        setGeometryWidth (long width) {
                    assert (width >= 0);
                    geometryWidth = width;
                    define(MV_GEOMETRY_WIDTH);
                }
    void        setGeometryHeight (long height) {
                    assert (height >= 0);
                    geometryHeight = height;
                    define(MV_GEOMETRY_HEIGHT);
                }
    void        setPolygonPoints (const PointList& ptlist) {
                    polygonPoints = ptlist;
                    define(MV_POLYGON_POINTS);
                }
    void        setPathPoints (const PointList& ptlist) {
                    pathPoints = ptlist;
                    define(MV_PATH_POINTS);
                }
    void        setPathHalfwidth (long halfwidth) {
                    assert (halfwidth >= 0);
                    pathHalfwidth = halfwidth;
                    define(MV_PATH_HALFWIDTH);
                }
    void        setPathStartExtension (long extn) {
                    pathStartExtn = extn;
                    define(MV_PATH_START_EXTENSION);
                }
    void        setPathEndExtension (long extn) {
                    pathEndExtn = extn;
                    define(MV_PATH_END_EXTENSION);
                }
    void        setCTrapezoidType (Uint type) {
                    ctrapezoidType = type;
                    define(MV_CTRAPEZOID_TYPE);
                }
    void        setCircleRadius (long radius) {
                    assert (radius >= 0);
                    circleRadius = radius;
                    define(MV_CIRCLE_RADIUS);
                }
    void        setLastPropertyName (PropName* propname) {
                    lastPropertyName = propname;
                    define(MV_LAST_PROPERTY_NAME);
                }
    void        setPropertyIsStandard (bool isStd) {
                    propertyIsStandard = isStd;
                    define(MV_PROPERTY_IS_STANDARD);
                }
    void        setLastValueList (const PropValueVector& values) {
                    lastValueList = values;
                    define(MV_LAST_VALUE_LIST);
                }

    // Functions to avoid copying Repetition and PointList objects.
    // Instead of constructing an object elsewhere and calling setFoo()
    // to copy the object into the corresponding modal variable,
    // OasisParser gets a reference to the modal variable, sets its
    // value, and then calls defineFoo() to tell this class that the
    // value is defined.

    Repetition& getRepetitionRef()    { return repetition;    }
    PointList&  getPolygonPointsRef() { return polygonPoints; }
    PointList&  getPathPointsRef()    { return pathPoints;    }

    void        defineRepetition()    { define(MV_REPETITION);     }
    void        definePolygonPoints() { define(MV_POLYGON_POINTS); }
    void        definePathPoints()    { define(MV_PATH_POINTS);    }

    //--------------------------------------------------
    // get methods

    bool        xyIsRelative() const { return xyRelative;    }
    bool        xyIsAbsolute() const { return (!xyRelative); }

    long        getPlacementX() const  { return placementX; }
    long        getPlacementY() const  { return placementY; }
    long        getGeometryX()  const  { return geometryX;  }
    long        getGeometryY()  const  { return geometryY;  }
    long        getTextX()      const  { return textX;      }
    long        getTextY()      const  { return textY;      }

    const Repetition&  getRepetition() const {
                    assert (isdef(MV_REPETITION));
                    return repetition;
                }

    CellName*   getPlacementCell() const {
                    assert (isdef(MV_PLACEMENT_CELL));
                    return placementCell;
                }
    Ulong       getLayer() const {
                    assert (isdef(MV_LAYER));
                    return layer;
                }
    Ulong       getDatatype() const {
                    assert (isdef(MV_DATATYPE));
                    return datatype;
                }
    Ulong       getTextlayer() const {
                    assert (isdef(MV_TEXTLAYER));
                    return textlayer;
                }
    Ulong       getTexttype() const {
                    assert (isdef(MV_TEXTTYPE));
                    return texttype;
                }
    TextString* getTextString() const {
                    assert (isdef(MV_TEXT_STRING));
                    return textString;
                }
    long        getGeometryWidth() const {
                    assert (isdef(MV_GEOMETRY_WIDTH));
                    return geometryWidth;
                }
    long        getGeometryHeight() const {
                    assert (isdef(MV_GEOMETRY_HEIGHT));
                    return geometryHeight;
                }
    const PointList&  getPolygonPoints() const {
                    assert (isdef(MV_POLYGON_POINTS));
                    return polygonPoints;
                }
    const PointList&  getPathPoints() const {
                    assert (isdef(MV_PATH_POINTS));
                    return pathPoints;
                }
    long        getPathHalfwidth() const {
                    assert (isdef(MV_PATH_HALFWIDTH));
                    return pathHalfwidth;
                }
    long        getPathStartExtension() const {
                    assert (isdef(MV_PATH_START_EXTENSION));
                    return pathStartExtn;
                }
    long        getPathEndExtension() const {
                    assert (isdef(MV_PATH_END_EXTENSION));
                    return pathEndExtn;
                }
    Uint        getCTrapezoidType() const {
                    assert (isdef(MV_CTRAPEZOID_TYPE));
                    return ctrapezoidType;
                }
    long        getCircleRadius() const {
                    assert (isdef(MV_CIRCLE_RADIUS));
                    return circleRadius;
                }
    PropName*   getLastPropertyName() const {
                    assert (isdef(MV_LAST_PROPERTY_NAME));
                    return lastPropertyName;
                }
    bool        getPropertyIsStandard() const {
                    assert (isdef(MV_PROPERTY_IS_STANDARD));
                    return propertyIsStandard;
                }
    const PropValueVector&  getLastValueList() const {
                    assert (isdef(MV_LAST_VALUE_LIST));
                    return lastValueList;
                }

private:
                ModalVars (const ModalVars&);           // forbidden
    void        operator= (const ModalVars&);           // forbidden
};


}  // namespace Oasis

#endif  // OASIS_MODAL_VARS_H_INCLUDED
