// gdsii/gds-copy.cc -- copy a GDSII Stream file the hard way
//
// last modified:   28-Nov-2004  Sun  14:58
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// usage:  gds-copy infile outfile
//
// This is a simple test of GdsParser and GdsCreator.  Since GdsCreator
// is derived from GdsBuilder, we can pass it as the builder argument
// to GdsParser::parseFile().  GdsCreator's output should be identical
// to GdsParser's input.

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "misc/utils.h"
#include "creator.h"
#include "parser.h"


using namespace std;
using namespace SoftJin;
using namespace Gdsii;


const char  UsageMessage[] =
"usage:  %s infile outfile\n"
"\n"
"infile  is assumed to be gzipped GDSII if its name ends with .gz.\n"
"outfile is automatically gzipped if its name ends with .gz\n"
"\n";



static void
UsageError() {
    fprintf(stderr, UsageMessage, GetProgramName());
    exit(1);
}


static void
DisplayWarning (const char* msg) {
    Error("%s", msg);
}


int
main (int argc, char* argv[])
{
    SetProgramName(argv[0]);
    if (argc != 3)
        UsageError();

    try {
        GdsParser  parser(argv[1], FileHandle::FileTypeAuto, DisplayWarning);
        GdsCreator creator(argv[2], FileHandle::FileTypeAuto);
        parser.parseFile(&creator);
    }
    catch (const exception& exc) {
        FatalError("%s", exc.what());
    }
    return 0;
}
