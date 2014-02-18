// oasis/oasis-print.cc -- print contents of OASIS files using OasisPrinter
//
// last modified:   25-Dec-2009  Fri  09:46
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// usage:  oasis-print [-c cellname] [-lntvx] oasis-file

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <unistd.h>

#include "misc/utils.h"
#include "printer.h"
#include "parser.h"

using namespace std;
using namespace SoftJin;
using namespace Oasis;


const char  UsageMessage[] =
"usage:  %s [-c cellname] [-lntvx] oasis-file\n"
"Options:\n"
"    -c cellname\n"
"        Select cell.  Print only the contents of the specified cell.\n"
"        The default is to print the entire file.\n"
"\n"
"    -l  Ignore LAYERNAME records.\n"
"\n"
"    -n  Do not insist on strict conformance to the OASIS specification.\n"
"        The default is to abort for (almost) any deviation.\n"
"\n"
"    -t  Ignore TEXT and TEXTSTRING records.\n"
"\n"
"    -v  Ignore the validation scheme and signature in the END record.\n"
"\n"
"    -x  Ignore XNAME, XELEMENT, and XGEOMETRY records.\n"
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

    const char*  cellname = Null;
    OasisParserOptions  parserOptions;

    int  opt;
    opterr = 0;
    while ((opt = getopt(argc, argv, "c:lntvx")) != EOF) {
        switch (opt) {
            case 'c':  cellname = optarg;                         break;
            case 'l':  parserOptions.wantLayerName     = false;   break;
            case 'n':  parserOptions.strictConformance = false;   break;
            case 't':  parserOptions.wantText          = false;   break;
            case 'v':  parserOptions.wantValidation    = false;   break;
            case 'x':  parserOptions.wantExtensions    = false;   break;
            default:   UsageError();
        }
    }
    if (optind != argc-1)
        UsageError();
    const char*  infilename = argv[optind];

    try {
        OasisParser   parser(infilename, DisplayWarning, parserOptions);
        OasisPrinter  printer(stdout);

        if (cellname == Null)
            parser.parseFile(&printer);
        else if (! parser.parseCell(cellname, &printer))
            FatalError("file '%s' has no cell '%s'", infilename, cellname);

        if (fclose(stdout) == EOF)
            FatalError("cannot close standard output: %s", strerror(errno));
    }
    catch (const std::exception& exc) {
        FatalError("%s", exc.what());
    }
    return 0;
}
