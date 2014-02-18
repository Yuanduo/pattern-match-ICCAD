// oasis/printer.cc -- print logical contents of OASIS file
//
// last modified:   01-Jan-2010  Fri  17:04
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cerrno>
#include <cstring>
#include "misc/utils.h"
#include "oasis.h"
#include "printer.h"


namespace Oasis {

using namespace std;
using namespace SoftJin;


namespace {


// GetStringSize -- return size of string as a Ulong
// str.size() returns string::size_type (size_t), which is a nuisance to
// print if we don't want to assume that printf() supports %zu.  (This
// is the same problem we have with off_t.)  To get around it, we
// convert the return value to Ulong and use %lu.

inline Ulong
GetStringSize (const string& str) {
    return str.size();
}



// GetPrintName -- get C string from OasisName

inline const char*
GetPrintName (OasisName* oname)
{
    assert (oname->hasName());
    return (oname->getName().c_str());
}



inline const char*
GetRepTypeName (RepetitionType repType)
{
    static const char*  names[] = {
        "Repeat",       // 0
        "Matrix",       // 1
        "UniformX",     // 2
        "UniformY",     // 3
        "VaryingX",     // 4
        "GridVaryingX", // 5
        "VaryingY",     // 6
        "GridVaryingY", // 7
        "TiltedMatrix", // 8
        "Diagonal",     // 9
        "Arbitrary",    // 10
        "GridArbitrary" // 11
    };

    return names[repType];
}



inline const char*
GetPointListTypeName (PointList::ListType ltype)
{
    static const char*  names[] = {
        "1-deltas, horizontal first",   // 0  ManhattanHorizFirst
        "1-deltas, vertical first",     // 1  ManhattanVertFirst
        "2-deltas",                     // 2  Manhattan
        "3-deltas",                     // 3  Octangular
        "g-deltas",                     // 4  AllAngle
        "double-deltas"                 // 5  AllAngleDoubleDelta
    };

    return names[ltype];
}


} // unnamed namespace
//----------------------------------------------------------------------


// OasisPrinter constructor
//   fp         output file

OasisPrinter::OasisPrinter (FILE* fp) {
    fpout = fp;
}


/*virtual*/
OasisPrinter::~OasisPrinter() { }



// Now come the OasisBuilder virtual methods that OasisParser invokes.
// Each just prints its arguments.


/*virtual*/ void
OasisPrinter::beginFile (const string& version,
                         const Oreal&  unit,
                         Validation::Scheme valScheme)
{
    // print() is a wrapper for fprintf() that checks the return value.

    print("OASIS version = \"%s\", unit = ", makePrintable(version));
    printOreal(unit);

    const char*  scheme;
    switch (valScheme) {
        case Validation::None:   scheme = "none";       break;
        case Validation::CRC32:  scheme = "crc32";      break;
        default:                 scheme = "checksum32"; break;
    }
    print(", validation scheme = %s\n", scheme);
}



/*virtual*/ void
OasisPrinter::beginCell (CellName* cellName)
{
    print("\nCELL \"%s\"\n", GetPrintName(cellName));
}



/*virtual*/ void
OasisPrinter::endFile() {
    print("-- EOF --\n");
}


/*virtual*/ void
OasisPrinter::endCell() {
    print("-- end of cell --\n");
}



/*virtual*/ void
OasisPrinter::beginPlacement (CellName*  cellName,
                              long x, long y,
                              const Oreal&  mag,
                              const Oreal&  angle,
                              bool flip,
                              const Repetition* rep)
{
    // Whenever a record takes more than one line, indent each line
    // after the first.  indprint() is like print() except that it takes
    // an initial argument, the indentation level, and prints spaces for
    // the indentation before the actual stuff.

    print("placement: \"%s\"\n", GetPrintName(cellName));
    indprint(1, "x = %ld, y = %ld\n", x, y);

    indprint(1, "mag = ");  printOreal(mag);
    print(", angle = ");    printOreal(angle);
    if (flip)
        print(", flip");
    print("\n");

    printRepetition(rep);
}



/*virtual*/ void
OasisPrinter::beginText (Ulong textlayer, Ulong texttype,
                         long x, long y,
                         TextString*  textString,
                         const Repetition*  rep)
{
    print("text: \"%s\"\n", makePrintable(textString->getName()));
    indprint(1, "textlayer = %lu, texttype = %lu\n", textlayer, texttype);
    indprint(1, "x = %ld, y = %ld\n", x, y);
    printRepetition(rep);
}



/*virtual*/ void
OasisPrinter::beginRectangle (Ulong layer, Ulong datatype,
                              long x, long y,
                              long width, long height,
                              const Repetition* rep)
{
    print("rectangle\n");
    indprint(1, "layer = %lu, datatype = %lu\n", layer, datatype);
    indprint(1, "x = %ld, y = %ld\n", x, y);
    indprint(1, "width = %ld, height = %ld\n", width, height);
    printRepetition(rep);
}



/*virtual*/ void
OasisPrinter::beginPolygon (Ulong layer, Ulong datatype,
                            long x, long y,
                            const PointList& ptlist,
                            const Repetition* rep)
{
    print("polygon\n");
    indprint(1, "layer = %lu, datatype = %lu\n", layer, datatype);
    indprint(1, "x = %ld, y = %ld\n", x, y);
    printPointList(ptlist);
    printRepetition(rep);
}



/*virtual*/ void
OasisPrinter::beginPath (Ulong layer, Ulong datatype,
                         long x, long  y,
                         long halfwidth,
                         long startExtn, long endExtn,
                         const PointList&  ptlist,
                         const Repetition* rep)
{
    print("path\n");
    indprint(1, "layer = %lu, datatype = %lu\n", layer, datatype);
    indprint(1, "x = %ld, y = %ld\n", x, y);
    indprint(1, "halfwidth = %ld, start_extn = %ld, end_extn = %ld\n",
             halfwidth, startExtn, endExtn);
    printPointList(ptlist);
    printRepetition(rep);
}



/*virtual*/ void
OasisPrinter::beginTrapezoid (Ulong layer, Ulong datatype,
                              long x, long  y,
                              const Trapezoid& trap,
                              const Repetition* rep)
{
    print("trapezoid\n");
    if (trap.isCompressed())
        indprint(1, "compress_type = %d\n", trap.getCompressType());
    indprint(1, "layer = %lu, datatype = %lu\n", layer, datatype);
    indprint(1, "x = %ld, y = %ld\n", x, y);

    const char*  orient = (trap.getOrientation() == Trapezoid::Vertical)
                              ? "vertical" : "horizontal";
    indprint(1,"%s, width = %ld, height = %ld, delta_a = %ld, delta_b = %ld\n",
             orient,
             trap.getWidth(), trap.getHeight(),
             trap.getDelta_A(), trap.getDelta_B());

    Delta  vertices[4];
    trap.getVertices(vertices);
    indprint(1, "SW = (%ld,%ld), SE = (%ld,%ld), "
                "NE = (%ld,%ld), NW = (%ld,%ld)\n",
             vertices[0].x, vertices[0].y,
             vertices[1].x, vertices[1].y,
             vertices[2].x, vertices[2].y,
             vertices[3].x, vertices[3].y);
    printRepetition(rep);

}



/*virtual*/ void
OasisPrinter::beginCircle (Ulong layer, Ulong datatype,
                           long x, long y,
                           long radius,
                           const Repetition* rep)
{
    print("circle\n");
    indprint(1, "layer = %lu, datatype = %lu\n", layer, datatype);
    indprint(1, "x = %ld, y = %ld\n", x, y);
    indprint(1, "radius = %ld\n", radius);
    printRepetition(rep);
}



/*virtual*/ void
OasisPrinter::beginXElement (Ulong attribute, const string& data)
{
    print("xelement\n");
    indprint(1, "attribute = %lu\n", attribute);
    indprint(1, "data length = %lu\n", GetStringSize(data));
}



/*virtual*/ void
OasisPrinter::beginXGeometry (Ulong layer, Ulong datatype,
                              long x, long y,
                              Ulong attribute,
                              const string& data,
                              const Repetition* rep)
{
    print("xgeometry\n");
    indprint(1, "layer = %lu, datatype = %lu\n", layer, datatype);
    indprint(1, "x = %ld, y = %ld\n", x, y);
    indprint(1, "attribute = %lu\n", attribute);
    indprint(1, "data length = %lu\n", GetStringSize(data));
    printRepetition(rep);
}



/*virtual*/ void
OasisPrinter::addFileProperty (Property* prop) {
    printProperty(0, prop);     // the first arg is the indentation level
}


/*virtual*/ void
OasisPrinter::addCellProperty (Property* prop) {
    printProperty(0, prop);
}


/*virtual*/ void
OasisPrinter::addElementProperty (Property* prop) {
    printProperty(1, prop);
}



/*virtual*/ void
OasisPrinter::registerCellName (CellName* cellName) {
    printName("cellname", cellName);
}


/*virtual*/ void
OasisPrinter::registerTextString (TextString* textString) {
    printName("textstring", textString);
}


/*virtual*/ void
OasisPrinter::registerPropName (PropName* propName) {
    printName("propname", propName);
}


/*virtual*/ void
OasisPrinter::registerPropString (PropString* propString) {
    printName("propstring", propString);
}


/*virtual*/ void
OasisPrinter::registerLayerName (LayerName* layerName) {
    printName("layername", layerName);
}


/*virtual*/ void
OasisPrinter::registerXName (XName* xname) {
    printName("xname", xname);
}


//----------------------------------------------------------------------
// Everything below is private.


// printName -- print an OasisName and its properties
//   nameType   what to print as the type of name
//   oname      instance of OasisName or a subclass

void
OasisPrinter::printName (const char* nameType, OasisName* oname)
{
    print("%s \"%s\"\n", nameType, makePrintable(oname->getName()));
    PropertyList::const_iterator  iter = oname->getPropertyList().begin(),
                                   end = oname->getPropertyList().end();
    for ( ;  iter != end;  ++iter)
        printProperty(1, *iter);
}



void
OasisPrinter::printProperty (Uint indentLevel, const Property* prop)
{
    indprint(indentLevel, "property %s%s with %lu value%s:\n",
             GetPrintName(prop->getName()),
             (prop->isStandard() ? " (standard)" : ""),
             static_cast<Ulong>(prop->numValues()),
             (prop->numValues() > 1) ? "s" : "");

    // Print the values one per line, indented by an extra level.

    PropValueVector::const_iterator  iter = prop->beginValues(),
                                      end = prop->endValues();
    for ( ;  iter != end;  ++iter)
        printPropValue(indentLevel+1, *iter);
}



void
OasisPrinter::printPropValue (Uint indentLevel, const PropValue* propval)
{
    indprint(indentLevel, "type %2d:  ", propval->getType());
    switch (propval->getType()) {
        case PV_Real_PosInteger:
        case PV_Real_NegInteger:
        case PV_Real_PosReciprocal:
        case PV_Real_NegReciprocal:
        case PV_Real_PosRatio:
        case PV_Real_NegRatio:
        case PV_Real_Float32:
        case PV_Real_Float64:
            printOreal(propval->getRealValue());
            break;

        case PV_UnsignedInteger:
            print("%llu", propval->getUnsignedIntegerValue());
            break;

        case PV_SignedInteger:
            print("%lld", propval->getSignedIntegerValue());
            break;

        case PV_AsciiString:
            print("ascii string \"%s\"",
                  makePrintable(propval->getStringValue()));
            break;

        case PV_BinaryString:
            print("binary string, length = %lu",
                  GetStringSize(propval->getStringValue()));
            break;

        case PV_NameString:
            print("name string \"%s\"", propval->getStringValue().c_str());
            break;

        case PV_Ref_AsciiString: {
            const string&  str(propval->getPropStringValue()->getName());
            print("ref ascii string \"%s\"", makePrintable(str));
            break;
        }
        case PV_Ref_BinaryString: {
            const string&  str(propval->getPropStringValue()->getName());
            print("ref binary string, length = %lu", GetStringSize(str));
            break;
        }
        case PV_Ref_NameString:
            print("ref name string \"%s\"",
                  GetPrintName(propval->getPropStringValue()));
            break;

        default:
            assert (false);
    }
    print("\n");
}



void
OasisPrinter::printRepetition (const Repetition* rep)
{
    if (rep == Null)
        return;

    // Print the repetition on one or more lines.  The first line has
    // the repeptition type both as a name and as a number (in case
    // names like TiltedMatrix convey nothing to the user).  It is
    // indented by one level to match the other lines of the element.
    // The following lines, if any, contain the repetition parameters
    // indented by two levels.

    RepetitionType  repType = rep->getType();
    indprint(1, "repetition type = %d (%s)\n",
             repType, GetRepTypeName(repType));

    const  Ulong  SpacesPerLine = 10;
        // For VaryingX, GridVaryingX, VaryingY, and GridVaryingY,
        // how many offsets to print on each line.

    const  Ulong  DeltasPerLine = 5;
        // For Arbitrary and GridArbitrary, how many deltas to print on
        // each line.

    switch (repType) {
        case Rep_ReusePrevious:
            // nothing to print
            break;

        // repetition type = Matrix (1)
        //     xdimen = AAA, ydimen = BBB, xspace = CCC, yspace = DDD

        case Rep_Matrix:
            indprint(2, "xdimen = %lu, ydimen = %lu, "
                        "xspace = %ld, yspace = %ld\n",
                     rep->getMatrixXdimen(), rep->getMatrixYdimen(),
                     rep->getMatrixXspace(), rep->getMatrixYspace());
            break;

        // repetition type = UniformX (2)
        //     dimen = AAA, xspace = BBB

        case Rep_UniformX:
            indprint(2, "dimen = %lu, xspace = %ld\n",
                     rep->getDimen(), rep->getUniformXspace());
            break;

        case Rep_UniformY:
            indprint(2, "dimen = %lu, yspace = %ld\n",
                     rep->getDimen(), rep->getUniformYspace());
            break;

        // repetition type = VaryingX (4)
        //     NNN points
        //     0  X1  X2  X3  X4      <---- the X offsets (first is always 0)
        //     X5  X6  ...
        //
        // For Rep_GridVaryingX, the first two lines will be:
        // repetition type = GridVaryingX (5)
        //     NNN points, grid = GGG

        case Rep_VaryingX:
        case Rep_GridVaryingX: {
            Ulong  dimen = rep->getDimen();
            indprint(2, "dimen = %lu", dimen);
            if (repType == Rep_GridVaryingX)
                print(", grid = %lu", rep->getGrid());

            for (Ulong j = 0;  j < dimen;  ++j) {
                if (j % SpacesPerLine == 0) {
                    print("\n");
                    printIndentation(2);
                }
                print("%ld  ", rep->getVaryingXoffset(j));
            }
            print("\n");
            break;
        }

        // This is the same as the code above, except with y instead of x.

        case Rep_VaryingY:
        case Rep_GridVaryingY: {
            Ulong  dimen = rep->getDimen();
            indprint(2, "dimen = %lu", dimen);
            if (repType == Rep_GridVaryingY)
                print(", grid = %lu", rep->getGrid());

            for (Ulong j = 0;  j < dimen;  ++j) {
                if (j % SpacesPerLine == 0) {
                    print("\n");
                    printIndentation(2);
                }
                print("%ld  ", rep->getVaryingYoffset(j));
            }
            print("\n");
            break;
        }

        case Rep_TiltedMatrix: {
            Delta  ndelta = rep->getMatrixNdelta();
            Delta  mdelta = rep->getMatrixMdelta();
            indprint(2, "ndimen = %lu, mdimen = %lu, "
                        "ndelta = (%ld,%ld), mdelta = (%ld,%ld)\n",
                     rep->getMatrixNdimen(), rep->getMatrixMdimen(),
                     ndelta.x, ndelta.y, mdelta.x, mdelta.y);
            break;
        }

        case Rep_Diagonal: {
            Delta  delta = rep->getDiagonalDelta();
            indprint(2, "dimen = %lu, delta = (%ld,%ld)\n",
                     rep->getDimen(), delta.x, delta.y);
            break;
        }

        case Rep_Arbitrary:
        case Rep_GridArbitrary: {
            Ulong  dimen = rep->getDimen();
            indprint(2, "dimen = %lu", dimen);
            if (repType == Rep_GridArbitrary)
                print(", grid = %lu", rep->getGrid());

            for (Ulong j = 0;  j < dimen;  ++j) {
                if (j % DeltasPerLine == 0) {
                    print("\n");
                    printIndentation(2);
                }
                Delta  delta = rep->getDelta(j);
                print("(%ld,%ld) ", delta.x, delta.y);
            }
            print("\n");
            break;
        }
    }
}



void
OasisPrinter::printPointList (const PointList& ptlist)
{
    // The first line has the point-list type and number of points.
    // The remaining lines have the offsets of the points.  Indent the
    // first line by one level to match the other lines of the element,
    // and indent the remaining lines by two levels.

    indprint(1, "point-list type = %d (%s), dimen = %lu",
             ptlist.getType(),
             GetPointListTypeName(ptlist.getType()),
             static_cast<Ulong>(ptlist.numPoints()) );

    const  Ulong  DeltasPerLine = 5;
        // How many deltas to print on each line.

    Ulong  n = 0;
    PointList::const_iterator  iter = ptlist.begin(),
                                end = ptlist.end();
    for ( ;  iter != end;  ++iter, ++n) {
        if (n % DeltasPerLine == 0) {
            print("\n");
            printIndentation(2);
        }
        print("(%ld,%ld) ", iter->x, iter->y);
    }
    print("\n");
}



void
OasisPrinter::printOreal (const Oreal& val)
{
    if (val.isInteger())
        print("%ld", val.getIntValue());
    else if (val.isRational())
        print("%ld/%ld", val.getNumerator(), val.getDenominator());
    else {
        // Add the suffix 'f' for 32-bit floats
        const char*  fmt = (val.isFloatSingle() ? "%.7gf" : "%.15g");
        print(fmt, val.getValue());
    }
}



// makePrintable -- make printable representation of a-string or b-string
// Returns a pointer to the printable string, which is a class member.
//
// We call this function to make printable the control characters in
// a-strings and in the b-strings of PropStrings and XNames.  Although
// a-strings should contain only printable chars, the parser allows
// control chars when its strictConformance option is false.

const char*
OasisPrinter::makePrintable (const string& str)
{
    return (MakePrintableString(str.data(), str.size(),
                                printBuf, sizeof(printBuf)));
}



// print -- fprintf() wrapper that checks the return value

void
OasisPrinter::print (const char* fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    if (vfprintf(fpout, fmt, ap) < 0)
        abortPrinter();
    va_end(ap);
}



// indprint -- print stuff preceded by spaces for indentation
//   indentLevel   logical indentation level (0, 1, 2, ...)
//   fmt        printf() format strings
//   ...        args for fmt, if any

void
OasisPrinter::indprint (Uint indentLevel, const char* fmt, ...)
{
    printIndentation(indentLevel);
    va_list  ap;
    va_start(ap, fmt);
    if (vfprintf(fpout, fmt, ap) < 0)
        abortPrinter();
    va_end(ap);
}



void
OasisPrinter::printIndentation (Uint indentLevel)
{
    static const char  indent[] = "                                ";

    size_t  nspaces = 4 * indentLevel;
    assert (nspaces < sizeof indent);
    if (fwrite(indent, 1, nspaces, fpout) != nspaces)
        abortPrinter();
}



// abortPrinter -- throw runtime_error for write error

void
OasisPrinter::abortPrinter()
{
    ThrowRuntimeError("write failed: %s", strerror(errno));
}



} // namespace Oasis
