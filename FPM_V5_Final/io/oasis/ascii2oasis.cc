// oasis/ascii2oasis.cc -- create OASIS file from its ASCII representation
//
// last modified:   05-Sep-2004  Sun  22:31
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// usage:  ascii2oasis [-c none|crc|checksum] ascii-file oasis-file
//
// This program reads the ASCII file ascii-file and writes its OASIS
// (binary) representation to oasis-file.  The -c option specifies the
// type of validation signature to compute for the OASIS file.  For CRC
// signatures, oasis-file must name a regular file if it already exists.
// If -z is specified, cells and name tables are compressed.

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <unistd.h>

#include "misc/utils.h"
#include "asc-conv.h"
#include "parser.h"


using namespace std;
using namespace SoftJin;
using namespace Oasis;


const char  UsageMessage[] =
"usage:  %s [-c none|crc|checksum] ascii-file oasis-file\n"
"Options:\n"
"    -c none|crc|checksum\n"
"        The validation scheme to use for the OASIS file.\n"
"        The scheme in the ASCII file's END record is ignored.\n"
"        The default is checksum.\n"
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



static bool
ParseValidationScheme (const char* arg, /*out*/ Validation::Scheme* pvalScheme)
{
    if (streq(arg, "none"))
        *pvalScheme = Validation::None;
    else if (streq(arg, "crc"))
        *pvalScheme = Validation::CRC32;
    else if (streq(arg, "checksum"))
        *pvalScheme = Validation::Checksum32;
    else
        return false;

    return true;
}



int
main (int argc, char* argv[])
{
    SetProgramName(argv[0]);

    // Defaults for command-line options
    AsciiToOasisOptions  options;
    options.valScheme = Validation::Checksum32;

    // Parse the command line.

    int  opt;
    opterr = 0;
    while ((opt = getopt(argc, argv, "c:")) != EOF) {
        switch (opt) {
            case 'c':
                if (! ParseValidationScheme(optarg, &options.valScheme))
                    UsageError();
                break;
            default:
                UsageError();
        }
    }
    if (optind != argc-2)
        UsageError();
    const char*  infilename  = argv[optind];
    const char*  outfilename = argv[optind+1];

    // Convert

    try {
        ConvertAsciiToOasis(infilename, outfilename, DisplayWarning, options);
    }
    catch (const std::exception& exc) {
        FatalError("%s", exc.what());
    }
    return 0;
}
