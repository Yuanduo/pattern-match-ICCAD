// oasis/builder.cc -- interface between OasisParser and client code
//
// last modified:   02-Jan-2010  Sat  16:22
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include "builder.h"

namespace Oasis {


// Provide default builder methods that do nothing so that derived
// classes need supply only the methods they need.


/*virtual*/ void
OasisBuilder::beginFile (const std::string&,
                         const Oreal&,
                         Validation::Scheme)  { }

/*virtual*/ void
OasisBuilder::endFile()  { }

/*virtual*/ void
OasisBuilder::beginCell (CellName*)  { }

/*virtual*/ void
OasisBuilder::endCell()  { }


/*virtual*/ void
OasisBuilder::beginPlacement (CellName*,
                              long, long,
                              const Oreal&,
                              const Oreal&,
                              bool,
                              const Repetition*)  { }

/*virtual*/ void
OasisBuilder::beginText (Ulong, Ulong,
                         long, long,
                         TextString*,
                         const Repetition*)  { }

/*virtual*/ void
OasisBuilder::beginRectangle (Ulong, Ulong,
                              long, long,
                              long, long,
                              const Repetition*)  { }

/*virtual*/ void
OasisBuilder::beginPolygon (Ulong, Ulong,
                            long, long,
                            const PointList&,
                            const Repetition*)  { }

/*virtual*/ void
OasisBuilder::beginPath (Ulong, Ulong,
                         long, long,
                         long,
                         long, long,
                         const PointList&,
                         const Repetition*)  { }

/*virtual*/ void
OasisBuilder::beginTrapezoid (Ulong, Ulong,
                              long, long,
                              const Trapezoid&,
                              const Repetition*)  { }

/*virtual*/ void
OasisBuilder::beginCircle (Ulong, Ulong,
                           long, long,
                           long,
                           const Repetition*)  { }

/*virtual*/ void
OasisBuilder::beginXElement (Ulong, const std::string&)  { }

/*virtual*/ void
OasisBuilder::beginXGeometry (Ulong, Ulong,
                              long, long,
                              Ulong,
                              const std::string&,
                              const Repetition*)  { }

/*virtual*/ void
OasisBuilder::endElement()  { }

/*virtual*/ void
OasisBuilder::addCellProperty (Property*)  { }

/*virtual*/ void
OasisBuilder::addFileProperty (Property*)  { }

/*virtual*/ void
OasisBuilder::addElementProperty (Property*)  { }

/*virtual*/ void
OasisBuilder::registerCellName   (CellName*)  { }

/*virtual*/ void
OasisBuilder::registerTextString (TextString*)  { }

/*virtual*/ void
OasisBuilder::registerPropName   (PropName*)  { }

/*virtual*/ void
OasisBuilder::registerPropString (PropString*)  { }

/*virtual*/ void
OasisBuilder::registerLayerName  (LayerName*)  { }

/*virtual*/ void
OasisBuilder::registerXName      (XName*)  { }


}  // namespace Oasis
