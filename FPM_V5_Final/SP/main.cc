#include <cstdlib>
#include <string>
#include <cstring>
#include <iostream>
#include "LayoutIO.h"
#include "Layout.h"
#include "SPPackingStrategy.h"
#include "SPPackingCommand.h"

using namespace std;
using namespace RECTPACKING;

int main(int argc, char **argv)
{
  LayoutIO layoutIO;
  Layout layout;
  if (argc < 3)
  {
    cerr << "Error in arguments" << endl;
    exit(-1);
  }
  string inFileName, outFileName = "tmpOut.txt";
  for(int i = 1; i < argc; i++)
  {
    //txt input
    if (strcmp(argv[i], "-in") == 0)
    {
      inFileName = argv[++i];
    }
    if (strcmp(argv[i], "-out") == 0)
    {
      outFileName = argv[++i];
    }
  }
  //testing ...
  layoutIO.loadLayout(layout, inFileName);
  SPPackingStrategy *spstrategy = new SPPackingStrategy();
  layout.setPackingStrategy(spstrategy);

  spstrategy->initialPacking(layout);
  spstrategy->compPackingCommand(layout);
  spstrategy->compPackingLayout(layout);

  layoutIO.saveLayout(layout, outFileName, false);
  layoutIO.saveLayout(layout, outFileName, true);

  return 0;
}
