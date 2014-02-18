// gdsii/gds-cells.cc -- list the structures in a GDSII Stream file
//
// last modified:   26-Nov-2004  Fri  21:41
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// usage:  gds-cells gdsii-file

#include <iostream>
#include <iterator>

#include "misc/utils.h"
#include "file-index.h"
#include "parser.h"


using namespace std;
using namespace SoftJin;
using namespace Gdsii;


int
main (int argc, char* argv[])
{
    SetProgramName(argv[0]);
    if (argc != 2) {
        cerr << "usage: " << GetProgramName() << " gdsii-file\n";
        return 1;
    }
    try {
        GdsParser  parser(argv[1], FileHandle::FileTypeAuto, Null);
        FileIndex*  index = parser.makeIndex();
        index->getAllNames(ostream_iterator<const char*>(cout, "\n"));
    }
    catch (const exception& exc) {
        cerr << GetProgramName() << ": " << exc.what() << endl;
        return 1;
    }
    return 0;
}
