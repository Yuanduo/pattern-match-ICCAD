// gdsii/creator.h -- high-level interface for creating GDSII Stream files
//
// last modified:   01-Mar-2005  Tue  20:48
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef GDSII_CREATOR_H_INCLUDED
#define GDSII_CREATOR_H_INCLUDED

#include "builder.h"
#include "writer.h"


namespace Gdsii {


// GdsCreator -- high-level interface for creating GDSII Stream files.
//
// This class is derived from GdsBuilder so that you can pass a
// GdsCreator object to GdsParser::parseFile() if you want to.  Except
// for setLocator(), you must call the methods of this class in the same
// order that GdsParser::parseFile() invokes the GdsBuilder methods.
// Calls to setLocator() are ignored.  See the comment before the
// GdsBuilder class declaration in builder.h for the sequence of calls.
//
// Implementation notes
//
// The data member xyPoints is used whenever a method wants a temporary
// GdsPointList.  By making it a class member instead of a local variable
// we avoid repeated construction and destruction of the vector it contains.

class GdsCreator : public GdsBuilder {
    GdsWriter           writer;         // low-level writing interface
    GdsPointList        xyPoints;       // used as a temporary

public:
                  GdsCreator (const char* fname, FileHandle::FileType ftype);
    virtual       ~GdsCreator();

    // virtual void  setLocator (const GdsLocator*);
    //    Inherit empty method from GdsBuilder; GdsCreator does not use this.
    
    virtual void  gdsVersion (int version);

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
                             const char* text,
                             const GdsTransform& strans,
                             const GdsTextOptions& options);

    virtual void  addProperty (int attr, const char* value);

    virtual void  endElement();

private:
    void        writeDate (const GdsDate& gdate);
    void        writeElementOptions (const GdsElementOptions& options);
    void        writeStrans (const GdsTransform& strans);
    void        writeXYRecord (const GdsPointList& points);

                GdsCreator (const GdsCreator&);         // forbidden
    void        operator= (const GdsCreator&);          // forbidden
};


}  // namespace Gdsii

#endif  // GDSII_CREATOR_H_INCLUDED
