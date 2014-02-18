// gdsii/gds2ascii.cc -- convert GDSII Stream file to ASCII
//
// last modified:   28-Nov-2004  Sun  14:58
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// usage:  gds2ascii [-a] [-s structure-name] [-u] gdsii-file [ascii-file]
//
// This program reads the GDSII Stream file gdsii-file and writes an
// ASCII (text) representation of the records to ascii-file.  Note that
// the input file name is required.  Use /dev/stdin to make this program
// read from stdin.  The default for ascii-file is /dev/stdout.


#include <cstdio>
#include <cstdlib>
#include <exception>
#include <unistd.h>

#include "misc/utils.h"
#include "asc-conv.h"
#include "parser.h"


using namespace std;
using namespace SoftJin;
using namespace Gdsii;


const char  UsageMessage[] =
"usage:  %s [-a] [-s structure-name] [-u] gdsii-file [ascii-file]\n"
"Options:\n"
"    -a   Print the address (file offset) of each GDSII record\n"
"         along with its text representation.\n"
"\n"
"    -s structure-name\n"
"         Convert only the records in this structure to ASCII.\n"
"         The default is to convert the entire file.\n"
"\n"
"    -u   Convert XY coordinates to floating-point numbers in user units.\n"
"         The default is to print them as they are in the file, i.e.,\n"
"         integers in database units.\n"
"\n"
"Arguments\n"
"    gdsii-file\n"
"         This is assumed to be gzipped GDSII if its name ends with '.gz'.\n"
"\n"
"    ascii-file\n"
"         The text representation is written to ascii-file.\n"
"         The default is to write to standard output.\n"
"\n"
"With either -s or -u the resulting ASCII file cannot (for now) be\n"
"converted back to GDSII.\n"
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

    // Defaults for command-line options
    GdsToAsciiOptions   options(false, false, FileHandle::FileTypeAuto);
    const char*         structureName = Null;
    const char*         outfilename = "/dev/stdout";

    // Parse the command line.

    int  opt;
    opterr = 0;
    while ((opt = getopt(argc, argv, "as:u")) != EOF) {
        switch (opt) {
            case 'a':  options.showOffsets = true;      break;
            case 's':  structureName = optarg;          break;
            case 'u':  options.convertUnits = true;     break;
            default:   UsageError();
        }
    }
    if (optind != argc-1  &&  optind != argc-2)
        UsageError();

    const char*  infilename = argv[optind];
    if (optind == argc-2)
        outfilename = argv[optind+1];

    // Convert the entire file or just one structure.
    // Note that GdsToAsciiConverter::convertStructure(), which
    // ConvertGdsToAscii() invokes, throws runtime_error if infilename
    // has no structure named structureName.

    try {
        if (structureName == Null)
            ConvertGdsToAscii(infilename, outfilename, options);
        else
            ConvertGdsToAscii(infilename, outfilename, options, structureName);
    }
    catch (const std::exception& exc) {
        FatalError("%s", exc.what());
    }
    return 0;
}
