#include <iostream>
#include "FPMPoly.h"

namespace FPM {
using namespace std;

ostream & operator << (ostream &out, FPMPoly &p)
{
  out << "layer = " << p.layer;
  const int DatasPerLine = 5;
  for (int i = 0; i < (int)p.ptlist.size(); ++ i)
  {
    if (i % DatasPerLine == 0)
    {
    	out << endl;
    }
    out << "(" << p.ptlist[i].x << "," << p.ptlist[i].y << ") ";
  }
  out << endl;
  return out;
}

bool FPMPoly::isClockwise()
{
	int pointsize = ptlist.size();
	int maxY = ptlist[0].y;
	int maxID = 0;
	
	for (int i = 1; i < pointsize; ++ i)
	{
		if (maxY < ptlist[i].y)
		{
			maxY = ptlist[i].y;
			maxID = i;
		}
	}
	
	int i1 = (maxID - 1 + pointsize) % pointsize;
	int i2 = maxID;
	int i3 = (maxID + 1) % pointsize;
	
	float k = ((float)ptlist[i2].x - ptlist[i1].x) * ((float)ptlist[i3].y - ptlist[i2].y)
	          - ((float)ptlist[i2].y - ptlist[i1].y) * ((float)ptlist[i3].x - ptlist[i2].x);
	return (k < 0);
}

void FPMPoly::makeCounterClockwise() {
  int pointsize = ptlist.size();
	int maxY = ptlist[0].y;
	int maxID = 0;
	
	for (int i = 1; i < pointsize; ++ i)
	{
		if (maxY < ptlist[i].y)
		{
			maxY = ptlist[i].y;
			maxID = i;
		}
	}
	
	int i1 = (maxID - 1 + pointsize) % pointsize;
	int i2 = maxID;
	int i3 = (maxID + 1) % pointsize;
	
	float k = ((float)ptlist[i2].x - ptlist[i1].x) * ((float)ptlist[i3].y - ptlist[i2].y)
	          - ((float)ptlist[i2].y - ptlist[i1].y) * ((float)ptlist[i3].x - ptlist[i2].x);
	if (k < 0)
	{
		vector<FPMPoint> new_ptlist;
		for (int i = pointsize - 1; i >= 0; -- i)
		{
			new_ptlist.push_back(ptlist[i]);
		}
		ptlist = new_ptlist;
	}
	
	return;
}

}
