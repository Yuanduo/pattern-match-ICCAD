#include <cstdlib>
#include <iostream>
#include <fstream>
#include "LayoutIO.h"
#include "Layout.h"
#include "PackingCommand.h"
#include "Rect.h"

namespace RECTPACKING {
using namespace std;

void LayoutIO::loadLayout(Layout &layout, string fileName)
{
  int rect_num;
  cout<<"Start to load original rectangles."<<endl;
  ifstream fin(fileName.c_str());
  if (!fin)
  {
    cout<<"Can not open the input file " << fileName << endl;
    exit(-1);
  }
  fin>>rect_num;
  cout << "Rect num: " << rect_num << endl;
  for (int i = 0; i != rect_num; ++i)
  {
    Rect r;
    fin >> r.width >> r.height;
    layout.addRect(r);
  }
  fin.close();

  cout << "Finished loading the rectangular layout."<<endl;
  cout << "Total number of rectangles: " << rect_num << endl;
}

void LayoutIO::saveLayout(Layout &layout, string fileName, bool app)
{
  ofstream fout;
  if (app)
    fout.open(fileName.c_str(), ios_base::app);
  else
    fout.open(fileName.c_str());
  vector<Rect> &rects = layout.getRects();
  PackingCommand *pcmd = layout.getPackingStrategy()->getPackingCommand();
  fout << *pcmd;
  fout << "Total number of rectangles: " << rects.size() << endl;
  for (int i = 0; i < (int)rects.size(); ++ i)
  {
    fout << "Rect " << i << ":" << rects[i];
  }
  fout.close();
  cout << "Finished saving the packed layout with " << rects.size() << " rectangles" << endl;
}

}
