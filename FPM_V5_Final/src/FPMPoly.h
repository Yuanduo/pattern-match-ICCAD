#ifndef __FPMPOLY_H__
#define __FPMPOLY_H__
#include <vector>
#include <ostream>
#include "FPMPoint.h"

namespace FPM {

struct FPMPoly
{
  int layer;
  std::vector<FPMPoint> ptlist;
  
  bool isClockwise();
  void makeCounterClockwise();
};

std::ostream & operator << (std::ostream &out, FPMPoly &p);

}

#endif  //FPMPOLY_H
