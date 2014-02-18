#ifndef __FPMRECT_H__
#define __FPMRECT_H__
#include <ostream>
#include "FPMPoint.h"

namespace FPM {

struct FPMRect
{
  int width;
  int height;
  int layer;
  int rect_id;
  FPMPoint lb;
  //wangqin changed
  bool touchArray[4];
  //int touchValue;
};

typedef vector<FPMRect> FPMRectVector;
std::ostream & operator << (std::ostream &out, FPMRect &r);

}

#endif	//FPMRECT_H
