#ifndef __RECT_H__
#define __RECT_H__
#include <ostream>
#include "Point.h"

namespace RECTPACKING {

struct Rect
{
  int width;
  int height;
  Point lb;
};

std::ostream & operator << (std::ostream &out, Rect &r);

}

#endif	//RECT_H
