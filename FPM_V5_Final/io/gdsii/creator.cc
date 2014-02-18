// gdsii/creator.cc -- high-level interface for creating GDSII Stream files
//
// last modified:   10-Dec-2004  Fri  12:24
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cstring>
#include "creator.h"

namespace Gdsii {

using namespace std;
using namespace SoftJin;


// constructor
//   fname      pathname of output file
//   ftype      type of file: uncompressed or compressed

GdsCreator::GdsCreator (const char* fname, FileHandle::FileType ftype)
  : writer(fname, ftype)
{
}


/*virtual*/
GdsCreator::~GdsCreator() { }


/*virtual*/ void
GdsCreator::gdsVersion (int version)
{
    // <file> ::= HEADER <libheader> {<structure>}* ENDLIB

    writer.beginFile();
    writer.writeShortRecord(GRT_HEADER, version);
}



/*virtual*/ void
GdsCreator::beginLibrary (const char*  libName,
                          const GdsDate&  modTime,
                          const GdsDate&  accTime,
                          const GdsUnits&  units,
                          const GdsLibraryOptions&  options)
{
    // <libheader> ::= BGNLIB [LIBDIRSIZE] [SRFNAME] [LIBSECUR]
    //                 LIBNAME [REFLIBS] [FONTS] [ATTRTABLE] [GENERATIONS]
    //                 [<FormatType>] UNITS

    writer.beginRecord(GRT_BGNLIB);
    writeDate(modTime);
    writeDate(accTime);
    writer.endRecord();

    if (options.contains(GdsLibraryOptions::REC_LIBDIRSIZE))
        writer.writeShortRecord(GRT_LIBDIRSIZE, options.getLibdirsize());
    if (options.contains(GdsLibraryOptions::REC_SRFNAME))
        writer.writeStringRecord(GRT_SRFNAME, options.getSrfname());

    if (options.contains(GdsLibraryOptions::REC_LIBSECUR)) {
        writer.beginRecord(GRT_LIBSECUR);
        for (Uint j = 0;  j < options.numAclEntries();  ++j) {
            GdsAclEntry  acl = options.getAclEntry(j);
            writer.writeShort(acl.groupid);
            writer.writeShort(acl.userid);
            writer.writeShort(acl.rights);
        }
        writer.endRecord();
    }

    writer.writeStringRecord(GRT_LIBNAME, libName);

    if (options.contains(GdsLibraryOptions::REC_REFLIBS)) {
        writer.beginRecord(GRT_REFLIBS);
        for (Uint j = 0;  j < options.numReflibs();  ++j)
            writer.writeString(options.getReflib(j));
        writer.endRecord();
    }

    if (options.contains(GdsLibraryOptions::REC_FONTS)) {
        writer.beginRecord(GRT_FONTS);
        for (Uint j = 0;  j < 4;  ++j)
            writer.writeString(options.getFont(j));
        writer.endRecord();
    }

    if (options.contains(GdsLibraryOptions::REC_ATTRTABLE))
        writer.writeStringRecord (GRT_ATTRTABLE, options.getAttrtable());

    if (options.contains(GdsLibraryOptions::REC_GENERATIONS))
        writer.writeShortRecord (GRT_GENERATIONS, options.getGenerations());

    // Optional <FormatType>
    // <FormatType> ::= FORMAT | FORMAT {MASK}+ ENDMASKS

    if (options.contains(GdsLibraryOptions::REC_FORMAT)) {
        writer.writeShortRecord (GRT_FORMAT, options.getFormat());
        if (options.contains(GdsLibraryOptions::REC_MASK)) {
            for (Uint j = 0;  j < options.numMasks();  ++j)
                writer.writeStringRecord (GRT_MASK, options.getMask(j));
            writer.writeEmptyRecord(GRT_ENDMASKS);
        }
    }

    writer.beginRecord(GRT_UNITS);
    writer.writeDouble(units.dbToUser);
    writer.writeDouble(units.dbToMeter);
    writer.endRecord();
}



/*virtual*/ void
GdsCreator::endLibrary()
{
    writer.writeEmptyRecord(GRT_ENDLIB);
    writer.endFile();
}



/*virtual*/ void
GdsCreator::beginStructure (const char*  structureName,
                            const GdsDate&  createTime,
                            const GdsDate&  modTime,
                            const GdsStructureOptions&  options)
{
    // <structure> ::= BGNSTR STRNAME [STRCLASS] {<element>}* ENDSTR

    writer.beginRecord(GRT_BGNSTR);
    writeDate(createTime);
    writeDate(modTime);
    writer.endRecord();

    writer.writeStringRecord(GRT_STRNAME, structureName);
    if (options.contains(GdsStructureOptions::REC_STRCLASS))
        writer.writeBitArrayRecord(GRT_STRCLASS, options.getStrclass());
}



/*virtual*/ void
GdsCreator::endStructure() {
    writer.writeEmptyRecord(GRT_ENDSTR);
}



/*virtual*/ void
GdsCreator::beginBoundary (int layer, int datatype,
                           const GdsPointList&  points,
                           const GdsElementOptions&  options)
{
    // <boundary> ::= BOUNDARY [ELFLAGS] [PLEX] LAYER DATATYPE XY

    assert (layer >= 0  &&  layer <= MaxLayer);
    assert (datatype >= 0  &&  datatype <= MaxDatatype);
    assert (points.size() >= MinBoundaryPoints);
    assert (points.size() <= MaxBoundaryPoints);

    writer.writeEmptyRecord(GRT_BOUNDARY);
    writeElementOptions(options);               // write ELFLAGS and PLEX
    writer.writeShortRecord(GRT_LAYER, layer);
    writer.writeShortRecord(GRT_DATATYPE, datatype);
    writeXYRecord(points);
}



/*virtual*/ void
GdsCreator::beginPath (int layer, int datatype,
                       const GdsPointList&  points,
                       const GdsPathOptions&  options)
{
    // <path> ::= PATH [ELFLAGS] [PLEX] LAYER DATATYPE [PATHTYPE]
    //            [WIDTH] [BGNEXTN] [ENDEXTN] XY

    assert (layer >= 0  &&  layer <= MaxLayer);
    assert (datatype >= 0  &&  datatype <= MaxDatatype);
    assert (points.size() >= MinPathPoints);
    assert (points.size() <= MaxPathPoints);

    writer.writeEmptyRecord(GRT_PATH);
    writeElementOptions(options);               // write ELFLAGS and PLEX
    writer.writeShortRecord(GRT_LAYER, layer);
    writer.writeShortRecord(GRT_DATATYPE, datatype);

    if (options.contains(GdsPathOptions::REC_PATHTYPE))
        writer.writeShortRecord(GRT_PATHTYPE, options.getPathtype());

    if (options.contains(GdsPathOptions::REC_WIDTH))
        writer.writeIntRecord(GRT_WIDTH, options.getWidth());

    // BGNEXTN and ENDEXTN can appear only for pathtype 4 (PathtypeCustom).
    // The spec does not say whether PathtypeCustom _requires_ these two
    // records; so don't enforce that.

    if (options.contains(GdsPathOptions::REC_BGNEXTN))
        writer.writeIntRecord(GRT_BGNEXTN, options.getBeginExtn());
    if (options.contains(GdsPathOptions::REC_ENDEXTN))
        writer.writeIntRecord(GRT_ENDEXTN, options.getEndExtn());

    writeXYRecord(points);
}



/*virtual*/ void
GdsCreator::beginSref (const char* sname,
                       int x, int y,
                       const GdsTransform&  strans,
                       const GdsElementOptions&  options)
{
    // <sref> ::= SREF [ELFLAGS] [PLEX] SNAME [<strans>] XY

    writer.writeEmptyRecord(GRT_SREF);
    writeElementOptions(options);               // write ELFLAGS and PLEX

    // XXX: check name for validity?
    writer.writeStringRecord(GRT_SNAME, sname);
    writeStrans(strans);

    xyPoints.clear();
    xyPoints.addPoint(x, y);
    writeXYRecord(xyPoints);
}



/*virtual*/ void
GdsCreator::beginAref (const char* sname,
                       Uint numCols, Uint numRows,
                       const GdsPointList&  points,
                       const GdsTransform&  strans,
                       const GdsElementOptions&  options)
{
    // <aref> ::= AREF [ELFLAGS] [PLEX] SNAME [<strans>] COLROW XY

    assert (numCols > 0  &&  numCols <= MaxArefColumns);
    assert (numRows > 0  &&  numRows <= MaxArefRows);
    assert (points.size() == 3);

    writer.writeEmptyRecord(GRT_AREF);
    writeElementOptions(options);               // write ELFLAGS and PLEX

    // XXX: check name for validity?
    writer.writeStringRecord(GRT_SNAME, sname);
    writeStrans(strans);

    writer.beginRecord(GRT_COLROW);
    writer.writeShort(numCols);
    writer.writeShort(numRows);
    writer.endRecord();

    writeXYRecord(points);
}



/*virtual*/ void
GdsCreator::beginNode (int layer, int nodetype,
                       const GdsPointList&  points,
                       const GdsElementOptions&  options)
{
    // <node> ::= NODE [ELFLAGS] [PLEX] LAYER NODETYPE XY

    assert (layer >= 0  &&  layer <= MaxLayer);
    assert (nodetype >= 0  &&  nodetype <= MaxNodetype);
    assert (points.size() >= MinNodePoints);
    assert (points.size() <= MaxNodePoints);

    writer.writeEmptyRecord(GRT_NODE);
    writeElementOptions(options);               // write ELFLAGS and PLEX
    writer.writeShortRecord(GRT_LAYER, layer);
    writer.writeShortRecord(GRT_NODETYPE, nodetype);
    writeXYRecord(points);
}



/*virtual*/ void
GdsCreator::beginBox (int layer, int boxtype,
                      const GdsPointList&  points,
                      const GdsElementOptions&  options)
{
    // <box> ::= BOX [ELFLAGS] [PLEX] LAYER BOXTYPE XY

    assert (layer >= 0  &&  layer <= MaxLayer);
    assert (boxtype >= 0  &&  boxtype <= MaxBoxtype);
    assert (points.size() == 5);

    writer.writeEmptyRecord(GRT_BOX);
    writeElementOptions(options);               // write ELFLAGS and PLEX
    writer.writeShortRecord(GRT_LAYER, layer);
    writer.writeShortRecord(GRT_BOXTYPE, boxtype);

    writeXYRecord(points);
}



/*virtual*/ void
GdsCreator::beginText (int layer, int texttype,
                       int x, int y,
                       const char*  text,
                       const GdsTransform&  strans,
                       const GdsTextOptions&  options)
{
    // <text> ::= TEXT [ELFLAGS] [PLEX] LAYER TEXTTYPE [PRESENTATION]
    //            [PATHTYPE] [WIDTH] [<strans>] XY STRING

    writer.writeEmptyRecord(GRT_TEXT);
    writeElementOptions(options);               // write ELFLAGS and PLEX

    writer.writeShortRecord(GRT_LAYER, layer);
    writer.writeShortRecord(GRT_TEXTTYPE, texttype);

    if (options.contains(GdsTextOptions::REC_PRESENTATION)) {
        Uint  font =  options.getFont();
        Uint  hjust = options.getHorizJustify();
        Uint  vjust = options.getVertJustify();
        Uint  bitArray = (font << 4) | (vjust << 2) | hjust;
        writer.writeBitArrayRecord(GRT_PRESENTATION, bitArray);
    }

    if (options.contains(GdsTextOptions::REC_PATHTYPE))
        writer.writeShortRecord(GRT_PATHTYPE, options.getPathtype());
    if (options.contains(GdsTextOptions::REC_WIDTH))
        writer.writeIntRecord(GRT_WIDTH, options.getWidth());
    writeStrans(strans);

    xyPoints.clear();
    xyPoints.addPoint(x, y);
    writeXYRecord(xyPoints);

    writer.writeStringRecord(GRT_STRING, text);
}



/*virtual*/ void
GdsCreator::addProperty (int attr, const char* value)
{
    // <property> ::= PROPATTR PROPVALUE

    assert (attr >= MinPropAttr  &&  attr <= MaxPropAttr);
    writer.writeShortRecord(GRT_PROPATTR, attr);
    writer.writeStringRecord(GRT_PROPVALUE, value);

    // XXX: check limit on total length of all properties?
}



/*virtual*/ void
GdsCreator::endElement()
{
    writer.writeEmptyRecord(GRT_ENDEL);
}


//----------------------------------------------------------------------
// Private methods


// writeDate -- add date to BGNLIB or BGNSTR record.

void
GdsCreator::writeDate (const GdsDate& gdate)
{
    writer.writeShort(gdate.year);
    writer.writeShort(gdate.month);
    writer.writeShort(gdate.day);
    writer.writeShort(gdate.hour);
    writer.writeShort(gdate.minute);
    writer.writeShort(gdate.second);
}



// writeElementOptions -- write the ELFLAGS and PLEX records

void
GdsCreator::writeElementOptions (const GdsElementOptions& options)
{
    if (options.contains(GdsElementOptions::REC_ELFLAGS)) {
        Uint  bitArray = 0;
        if (options.isExternal())   bitArray |= 0x02;
        if (options.isTemplate())   bitArray |= 0x01;
        writer.writeBitArrayRecord(GRT_ELFLAGS, bitArray);
    }
    if (options.contains(GdsElementOptions::REC_PLEX))
        writer.writeIntRecord(GRT_PLEX, options.getPlex());
}



void
GdsCreator::writeStrans (const GdsTransform& strans)
{
    // <strans> ::= STRANS [MAG] [ANGLE]

    if (! strans.contains(GdsTransform::REC_STRANS))
        return;

    Uint  bitArray = 0;
    if (strans.isReflected())     bitArray |= 0x8000;
    if (strans.magIsAbsolute())   bitArray |= 0x0004;
    if (strans.angleIsAbsolute()) bitArray |= 0x0002;
    writer.writeBitArrayRecord(GRT_STRANS, bitArray);

    if (strans.contains(GdsTransform::REC_MAG))
        writer.writeDoubleRecord(GRT_MAG, strans.getMag());
    if (strans.contains(GdsTransform::REC_ANGLE))
        writer.writeDoubleRecord(GRT_ANGLE, strans.getAngle());
}



void
GdsCreator::writeXYRecord (const GdsPointList& points)
{
    writer.beginRecord(GRT_XY);
    GdsPointList::const_iterator  iter = points.begin();
    GdsPointList::const_iterator  end  = points.end();
    for ( ;  iter != end;  ++iter) {
        writer.writeInt(iter->x);
        writer.writeInt(iter->y);
    }
    writer.endRecord();
}


}  // namespace Gdsii
