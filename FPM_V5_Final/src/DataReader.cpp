#include <string>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <unistd.h>

#include "DataReader.h"
#include "FPMLayout.h"
#include "OasisReader.h"
#include "misc/utils.h"
#include "OasisReader.h"
#include "oasis/parser.h"
using namespace std;
using namespace SoftJin;
using namespace Oasis;

namespace FPM {

static void
DisplayWarning (const char* msg) {
    Error("%s", msg);
}

bool DataReader::ReadOASIS(string fileName, FPMLayout &layout)
{
    OasisParserOptions  parserOptions;
    parserOptions.wantLayerName     = false;
    parserOptions.strictConformance = false;
    parserOptions.wantText          = false;
    parserOptions.wantValidation    = false;
    parserOptions.wantExtensions    = false;
    const char*  infilename = fileName.c_str();

    printf("Start Reading OASIS layout ...\n");
    try {
        OasisParser   parser(infilename, DisplayWarning, parserOptions);
        OasisReader   reader(stdout, layout);
        
        parser.parseFile(&reader);
    }
    catch (const std::exception& exc) {
        FatalError("%s", exc.what());
    }
    
    printf("Finished Reading OASIS layout.\n");
    return true;
}

}
