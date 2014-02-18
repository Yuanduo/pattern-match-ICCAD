// oasis/printer.h -- print logical contents of OASIS file
//
// last modified:   30-Dec-2009  Wed  22:09
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// OasisPrinter is a subclass of OasisBuilder whose methods merely print
// their arguments.  You can examine the logical contents of an OASIS
// file by passing an instance of OasisPrinter to OasisParser.  "Logical
// contents" means that you don't see the actual file contents because
// the parser resolves reference numbers in the file to names and fills
// in implicit values using the modal variables.
//
// OasisPrinter works at the semantic level.  That is how it differs
// from OasisToAsciiConverter, which also prints the contents of OASIS
// files in a readable form, but works at the syntactic level and shows
// more or less exactly what is in the file.  The output of
// OasisToAsciiConverter can be converted back to OASIS; the output
// format of OasisPrinter cannot.


#ifndef OASIS_PRINTER_H_INCLUDED
#define OASIS_PRINTER_H_INCLUDED

#include <cstdio>
#include <string>
#include "port/compiler.h"
#include "builder.h"


namespace Oasis {

using std::FILE;
using std::string;
using SoftJin::Ulong;


class OasisPrinter : public OasisBuilder {
    FILE*       fpout;          // opened for writing on filename
    char        printBuf[256];  // for printable versions of [ab]-strings
public:
                  OasisPrinter (FILE* fp);
    virtual       ~OasisPrinter();

    virtual void  beginFile (const std::string&  version,
                             const Oreal&        unit,
                             Validation::Scheme  valScheme);
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

    // virtual void  endElement();
    //     Inherit empty implementation from OasisBuilder

    virtual void  addFileProperty (Property* prop);
    virtual void  addCellProperty (Property* prop);
    virtual void  addElementProperty (Property* prop);

    virtual void  registerCellName   (CellName*   cellName);
    virtual void  registerTextString (TextString* textString);
    virtual void  registerPropName   (PropName*   propName);
    virtual void  registerPropString (PropString* propString);
    virtual void  registerLayerName  (LayerName*  layerName);
    virtual void  registerXName      (XName*      xname);

private:
    void        printName (const char* nameType, OasisName* oname);
    void        printProperty (Uint indentLevel, const Property* prop);
    void        printPropValue (Uint indentLevel, const PropValue* propval);
    void        printRepetition (const Repetition* rep);
    void        printPointList (const PointList& ptlist);
    void        printOreal (const Oreal& val);
    const char* makePrintable (const string& str);
    void        print (const char* fmt, ...) SJ_PRINTF_ARGS(2,3);
    void        indprint (Uint indentLevel, const char* fmt, ...)
                                             SJ_PRINTF_ARGS(3,4);
    void        printIndentation (Uint indentLevel);
    void        abortPrinter();

private:
                OasisPrinter (const OasisPrinter&);     // forbidden
    void        operator= (const OasisPrinter&);        // forbidden
};


} // namespace Oasis

#endif  // OASIS_PRINTER_H_INCLUDED
