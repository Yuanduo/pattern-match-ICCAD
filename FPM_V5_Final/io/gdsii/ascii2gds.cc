// gdsii/ascii2gds.cc -- convert text form of GDSII back to GDSII Stream
//
// last modified:   29-Nov-2004  Mon  16:03
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// usage:  ascii2gds ascii-file gdsii-file
//
// This program reads the ASCII representation from ascii-file and
// writes the corresponding GDSII Stream file to gdsii-file.  Note
// that the both file names are required.  Use /dev/stdin to make this
// read from stdin.

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <unistd.h>

#include "misc/utils.h"
#include "asc-conv.h"


using namespace std;
using namespace SoftJin;
using namespace Gdsii;


const char  UsageMessage[] =
"usage:  %s ascii-file gdsii-file\n"
"The GDSII output is gzipped if gdsii-file ends with '.gz'\n"
"\n";



static void
UsageError()
{
    fprintf(stderr, UsageMessage, GetProgramName());
    exit(1);
}



int
main (int argc, char* argv[])
{
    SetProgramName(argv[0]);

    if (argc != 3)
        UsageError();
    const char*  infilename  = argv[1];
    const char*  outfilename = argv[2];

    try {
        AsciiToGdsOptions  options(FileHandle::FileTypeAuto);
        ConvertAsciiToGds(infilename, outfilename, options);
    } catch (const std::exception& exc) {
        FatalError("%s", exc.what());
    }
    return 0;
}
