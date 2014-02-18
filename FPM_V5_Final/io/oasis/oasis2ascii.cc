// oasis/oasis2ascii.cc -- print ASCII representation of OASIS file
//
// last modified:   24-Dec-2009  Thu  16:57
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// usage:  oasis2ascii [-1] [-a] oasis-file [ascii-file]
//
// This program reads the OASIS file oasis-file and writes an ASCII
// (text) representation of the records to ascii-file.  oasis-file must
// be seekable, i.e., it must be a regular file.  ascii-file defaults to
// /dev/stdout.

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <unistd.h>

#include "misc/utils.h"
#include "asc-conv.h"


using namespace std;
using namespace SoftJin;
using namespace Oasis;


const char  UsageMessage[] =
"usage:  %s [-1] [-a] [-c] oasis-file [ascii-file]\n"
"Options:\n"
"    -1   Print each record on a single line.  The default is\n"
"         to fold long lines.  This is digit one, not letter ell.\n"
"\n"
"    -a   Print the address (file offset) of each OASIS record.\n"
"\n"
"    ascii-file\n"
"         Write the text representation to ascii-file.\n"
"         The default is to write to standard output.\n"
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

    // Defaults for command-line options
    OasisToAsciiOptions  options;
    options.showOffsets     = false;
    options.foldLongLines   = true;
    const char*  outfilename = "/dev/stdout";

    // Parse the command line.

    int  opt;
    opterr = 0;
    while ((opt = getopt(argc, argv, "1a")) != EOF) {
        switch (opt) {
            case '1':  options.foldLongLines = false;   break;
            case 'a':  options.showOffsets = true;      break;
            default:   UsageError();
        }
    }
    if (optind != argc-1  &&  optind != argc-2)
        UsageError();

    const char*  infilename  = argv[optind];
    if (optind == argc-2)
        outfilename = argv[optind+1];

    // Convert

    try {
        ConvertOasisToAscii(infilename, outfilename, DisplayWarning, options);
    }
    catch (const std::exception& exc) {
        FatalError("%s", exc.what());
    }
    return 0;
}
