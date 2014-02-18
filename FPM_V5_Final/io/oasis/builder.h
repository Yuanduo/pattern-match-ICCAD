// oasis/builder.h -- interface between OasisParser and client code
//
// last modified:   02-Jan-2010  Sat  22:04
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// A parser for OASIS files can be used in many ways.  So instead of
// creating a data structure directly, we use a Builder class (see the
// Design Patterns book) to decouple the parser from the rest of the
// application.  OasisParser's constructor is passed a pointer to a
// OasisBuilder.  As the parser recognizes each part of the input file, it
// invokes the builder's method for that part.
//
// To use the parser, derive a class from OasisBuilder, override the
// methods for the file parts that interest you, and pass an instance
// of the class to OasisParser.
//
// OasisBuilder's secondary role is as the interface to OasisCreator, which
// is derived from OasisBuilder.  OasisCreator is the high-level interface
// for creating OASIS Stream files.


#ifndef OASIS_BUILDER_H_INCLUDED
#define OASIS_BUILDER_H_INCLUDED

#include <string>
#include "names.h"
#include "oasis.h"
#include "trapezoid.h"

namespace Oasis {

using SoftJin::Ulong;


// OasisBuilder -- callbacks for OasisParser.
// OasisParser::parseFile() invokes the builder methods in the order
// given by the Extended BNF description below.  The notation  { foo }*
// means zero or more occurrences of foo.  The indentation in the RHS of
// the productions is only for clarity.
//
// <file> ::= beginFile()
//                { <name> }*
//                { addFileProperty() }*
//                { <cell> }*
//            endFile()
//
// <name> ::= registerCellName()
//          | registerTextString()
//          | registerPropName()
//          | registerPropString()
//          | registerLayerName()
//          | registerXName()
//
// <cell> ::= beginCell()
//                { addCellProperty() }*
//                { <element> }*
//            endCell()
//
// <element> ::= <beginElem>
//                   { addElementProperty() }*
//               endElement()
//
// <beginElem> ::= beginPlacement()
//               | beginText()
//               | beginRectangle()
//               | beginPolygon()
//               | beginPath()
//               | beginTrapezoid()
//               | beginCircle()
//               | beginXElement()
//               | beginXGeometry()
//
// OasisParser::parseCell() invokes the builder methods as for <cell>.
// beginTrapezoid() is invoked for both TRAPEZOID and CTRAPEZOID
// records.  There are no callbacks for XYABSOLUTE and XYRELATIVE
// records.  OasisParser swallows them, converting to absolute all
// coordinates passed to the builder methods.
//
// The registerFooName() methods are invoked for all names that appear
// in <name> and CELL records anywhere in the file.  The arguments point
// to name objects that the parser owns and will retain while it exists.
//
// The parser parses all <name> records and their associated PROPERTY
// records before parsing anything else, so you are guaranteed that the
// names referenced in the other methods have been resolved.
//
// Filters
//
// OasisBuilders can be linked to form something like a Unix pipeline.
// The stages in this pipeline work on cells, elements, and properties
// rather than bytes.  Typically each virtual method invokes the
// corresponding method of the next stage in the pipeline.  Each stage
// can transform what it gets in any way so long as it invokes the next
// builder's methods in the order given by EBNF grammar above.  For
// example, it can delete selected cells or elements.
//
// Each builder has a pointer, 'next', which points to the next builder
// in the pipeline.  This is analogous to stdout in Unix filters.
// Builders that act as the sink in a pipeline ignore it.  It may be set
// and read using setNext() and getNext().


class OasisBuilder {
protected:
    OasisBuilder* next;         // next stage in pipeline of builders

public:
    explicit
    OasisBuilder (OasisBuilder* next = Null)   { this->next = next; }

    virtual       ~OasisBuilder() { }

    void          setNext (OasisBuilder* next) { this->next = next; }
    OasisBuilder* getNext() const              { return next;       }

    //-------------------------------------------------------

    virtual void  beginFile (const std::string&  version,
                             const Oreal&        unit,
                             Validation::Scheme  valScheme);
        // version and unit come from the START record;
        // valScheme is the validation scheme from the END record.

    virtual void  endFile();

    virtual void  beginCell (CellName* cellName);
    virtual void  endCell();


    virtual void  beginPlacement (CellName*  cellName,
                                  long x, long y,
                                  const Oreal&  mag,
                                  const Oreal&  angle,
                                  bool flip,
                                  const Repetition*  rep);

    virtual void  beginText (Ulong textlayer, Ulong texttype,
                             long x, long y,
                             TextString* text,
                             const Repetition* rep);

    virtual void  beginRectangle (Ulong layer, Ulong datatype,
                                  long x, long y,
                                  long width, long height,
                                  const Repetition*  rep);

    virtual void  beginPolygon (Ulong layer, Ulong datatype,
                                long x, long y,
                                const PointList&  ptlist,
                                const Repetition*  rep);

    virtual void  beginPath (Ulong layer, Ulong datatype,
                             long x, long  y,
                             long halfwidth,
                             long startExtn, long endExtn,
                             const PointList&  ptlist,
                             const Repetition*  rep);

    virtual void  beginTrapezoid (Ulong layer, Ulong datatype,
                                  long x, long  y,
                                  const Trapezoid&  trap,
                                  const Repetition*  rep);

    virtual void  beginCircle (Ulong layer, Ulong datatype,
                               long x, long y,
                               long radius,
                               const Repetition*  rep);

    virtual void  beginXElement (Ulong attribute, const std::string& data);

    virtual void  beginXGeometry (Ulong layer, Ulong datatype,
                                  long x, long y,
                                  Ulong attribute,
                                  const std::string& data,
                                  const Repetition*  rep);

    virtual void  endElement();

    virtual void  addFileProperty (Property* prop);
    virtual void  addCellProperty (Property* prop);
    virtual void  addElementProperty (Property* prop);

    virtual void  registerCellName   (CellName*   cellName);
    virtual void  registerTextString (TextString* textString);
    virtual void  registerPropName   (PropName*   propName);
    virtual void  registerPropString (PropString* propString);
    virtual void  registerLayerName  (LayerName*  layerName);
    virtual void  registerXName      (XName*      xname);
};


}  // namespace Oasis

#endif  // OASIS_BUILDER_H_INCLUDED
