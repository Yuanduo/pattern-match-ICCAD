// gdsii/gds-dot.cc -- generate 'dot' input for structure DAG of GDSII file
//
// last modified:   01-Jan-2010  Fri  21:40
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// usage:  gds-dot gdsii-file
//
// This is a simple example of how to use GdsGraphBuilder.
// dot is a program for drawing digraphs.  It is part of the graphviz
// package.  See
//     http://www.research.att.com/sw/tools/graphviz/

#include <cstdio>
#include "port/hash-table.h"
#include "misc/stringpool.h"
#include "misc/utils.h"
#include "parser.h"


using namespace std;
using namespace SoftJin;
using namespace Gdsii;


//----------------------------------------------------------------------

// DotGraphBuilder -- generate 'dot' graph file from SREF/AREF data.
// For each structure X that contains an SREF or AREF referring to
// structure Y, the graph generated will contain an edge X -> Y.
//
// currCell
//      The cell (structure) currently being processed.
//
// srefs
//      The names of structures already referenced by currCell.  We use
//      this to avoid creating redundant edges in the graph.
//
// namePool
//      Storage for the name strings.


class DotGraphBuilder : public GdsGraphBuilder {
    typedef  HashSet<const char*, hash<const char*>, StringEqual>  NameSet;

    const char*   currCell;
    NameSet       srefs;
    StringPool    namePool;

public:
                  DotGraphBuilder();
    virtual       ~DotGraphBuilder();

    virtual void  beginLibrary (const char* libname);
    virtual void  enterStructure (const char* sname);
    virtual void  addSref (const char* sname);
    virtual void  endLibrary();
};


DotGraphBuilder::DotGraphBuilder()  { }
DotGraphBuilder::~DotGraphBuilder() { }


/*virtual*/ void
DotGraphBuilder::beginLibrary (const char* libname) {
    printf("digraph \"%s\" {\n", libname);
}



/*virtual*/ void
DotGraphBuilder::enterStructure (const char* sname)
{
    srefs.clear();
    namePool.clear();
    printf("    \"%s\";\n", sname);
    currCell = namePool.newString(sname);
}



/*virtual*/ void
DotGraphBuilder::addSref (const char* sname)
{
    if (srefs.find(sname) == srefs.end()) {
        printf("    \"%s\" -> \"%s\";\n", currCell, sname);
        srefs.insert(namePool.newString(sname));
    }
}



/*virtual*/ void
DotGraphBuilder::endLibrary() {
    printf("}\n");
}


//----------------------------------------------------------------------


const char  UsageMessage[] =
"usage:  %s gdsii-file\n"
"gdsii-file is assumed to be gzipped if its name ends with '.gz'.\n"
"\n";


int
main (int argc, char* argv[])
{
    SetProgramName(argv[0]);
    if (argc != 2) {
        fprintf(stderr, UsageMessage, GetProgramName());
        return 1;
    }
    try {
        GdsParser  parser(argv[1], FileHandle::FileTypeAuto, Null);
        DotGraphBuilder  dotgen;
        parser.buildStructureGraph(&dotgen);
    }
    catch (const exception& exc) {
        FatalError("%s", exc.what());
    }
    return 0;
}
