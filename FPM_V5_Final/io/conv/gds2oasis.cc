// programs/gds2oasis.cc -- translate GDSII files to OASIS
//
// last modified:   02-Mar-2006  Thu  13:00
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// usage:  gds2oasis [options] gdsii-file oasis-file
//
// This program reads the GDSII file gdsii-file and writes its OASIS
// equivalent to oasis-file.  See the usage message below for the options.

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <unistd.h>
#include "misc/utils.h"
#include "conv/gds-oasis.h"

using namespace std;
using namespace SoftJin;
using namespace GdsiiOasis;


namespace {

const char  UsageMessage[] =
"usage:  %s [-c none|crc|checksum] [-O opt] [-Dnprtvz] infile outfile\n"
"Options:\n"
"    -c none|crc|checksum\n"
"        The validation scheme to use for the OASIS file.\n"
"        The default is checksum.\n"
"\n"
"    -D  Preserve duplicate elements.  Normally duplicates are deleted\n"
"        because they are redundant.\n"
"\n"
"    -n  Write each name record before it is referenced.\n"
"        The default is to collect all the name records in strict-mode\n"
"        name tables at the end of the file.  That is normally preferred,\n"
"        but some vendors' OASIS tools process the file sequentially, and\n"
"        so require each name record to appear before it is referenced.\n"
"\n"
"    -O opt\n"
"        Set the optimization level to opt, which must be 0, 1, or 2.\n"
"        With a higher optimization level the program takes more time\n"
"        and memory to try to make the OASIS file smaller.\n"
"        The default level is 1.\n"
"\n"
"    -p  Convert single-segment horizontal and vertical PATHs to\n"
"        RECTANGLEs.  The default is to convert them to PATHs.\n"
"\n"
"    -r  Use relative xy-mode when writing the cells.  The default is\n"
"        to use absolute mode.\n"
"\n"
"    -t  Preserve text transforms.  By default gds2oasis ignores transforms\n"
"        in text elements because OASIS does not support them.  With this\n"
"        option it instead replaces each transformed text by a transformed\n"
"        PLACEMENT to a special cell that contains just one element,\n"
"        the text that was transformed.\n"
"\n"
"    -v  Verbose.  The converter writes a message to stderr each time it\n"
"        starts converting a new structure or new batch of elements.\n"
"\n"
"    -z  Compress the cells and name tables in the OASIS file,\n"
"        i.e., put them in CBLOCK records.\n"
"        The default is not to compress them.\n"
"\n"
"Arguments:\n"
"    infile\n"
"        Pathname of the GDSII or gzipped-GDSII file.  A gzipped file\n"
"        must have the suffix '.gz'.  May be a pipe, e.g., /dev/stdin,\n"
"        but that would be assumed to be uncompressed.\n"
"\n"
"    outfile\n"
"        Pathname of the OASIS file to create.\n"
"\n";



void
UsageError() {
    fprintf(stderr, UsageMessage, GetProgramName());
    exit(1);
}


void
DisplayWarning (const char* msg) {
    Error("%s", msg);
}



bool
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



bool
ParseOptLevel (const char* arg, /*out*/ Uint* pOptLevel)
{
    int  ch = static_cast<Uchar>(*arg);
    if (!isdigit(ch)  ||  arg[1] != Cnul)
        return false;
    *pOptLevel = ch - '0';
    return true;
}


} // unnamed namespace



int
main (int argc, char* argv[])
{
    SetProgramName(argv[0]);

    // Defaults for command-line options
    GdsToOasisOptions  options;
    options.gdsFileType     = FileHandle::FileTypeAuto;
    options.valScheme       = Validation::Checksum32;
    options.immediateNames  = false;
    options.compress        = false;
    options.deleteDuplicates= true;
    options.pathToRectangle = false;
    options.relativeMode    = false;
    options.transformTexts  = false;
    options.verbose         = false;
    options.optLevel        = 1;

    // Parse the command line.

    int  opt;
    opterr = 0;
    while ((opt = getopt(argc, argv, "c:DnO:prtvz")) != EOF) {
        switch (opt) {
            case 'c':
                if (! ParseValidationScheme(optarg, &options.valScheme)) {
                    Error("invalid validation scheme '%s'", optarg);
                    UsageError();
                }
                break;
            case 'D':
                options.deleteDuplicates = false;
                break;
            case 'n':
                options.immediateNames = true;
                break;
            case 'O':
                if (! ParseOptLevel(optarg, &options.optLevel)) {
                    Error("invalid optimization level '%s'", optarg);
                    UsageError();
                }
                break;
            case 'p':
                options.pathToRectangle = true;
                break;
            case 'r':
                options.relativeMode = true;
                break;
            case 't':
                options.transformTexts = true;
                break;
            case 'v':
                options.verbose = true;
                break;
            case 'z':
                options.compress = true;
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
        ConvertGdsToOasis(infilename, outfilename, DisplayWarning, options);
    }
    catch (const std::exception& exc) {
        FatalError("%s", exc.what());
    }
    return 0;
}
