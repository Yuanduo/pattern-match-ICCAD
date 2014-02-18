// oasis/oasis-validate.cc -- verify CRC/checksum of OASIS file
//
// last modified:   26-Dec-2009  Sat  10:08
//
// Copyright (c) 2009 SoftJin Technologies Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// usage:  oasis-validate oasis-file

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <unistd.h>

#include "misc/utils.h"
#include "parser.h"

using namespace std;
using namespace SoftJin;
using namespace Oasis;


static void
UsageError() {
    fprintf(stderr, "%s oasis-file\n", GetProgramName());
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
    if (argc != 2)
        UsageError();

    try {
        OasisParserOptions  parserOptions;
        parserOptions.strictConformance = false;
        OasisParser  parser(argv[1], DisplayWarning, parserOptions);
        parser.validateFile();

        // validateFile() throws runtime_error if the validation fails.
        // If we reach this point, the validation has succeeded.

        Validation  val = parser.parseValidation();
        if (val.scheme == Validation::None)
            printf("the file has no validation signature\n");
        else
            printf("%s 0x%08x validated\n", val.schemeName(), val.signature);
    }
    catch (const std::exception& exc) {
        FatalError("%s", exc.what());
    }
    return 0;
}
