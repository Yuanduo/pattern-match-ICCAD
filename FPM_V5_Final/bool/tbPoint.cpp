#include "tbPoint.h"

namespace KBool {

tbPoint::tbPoint(int x, int y){
  mx = x;
  my = y;
}

tbPoint::tbPoint(const tbPoint &p){
  mx = p.x();
  my = p.y();
}

int &tbPoint::x() { return mx; }
int &tbPoint::y() { return my; }
int tbPoint::x() const { return mx; }
int tbPoint::y() const { return my; }

void tbPoint::set(int x, int y)
{
  mx = x;
  my = y;
}

void tbPoint::setP(int x, int y)
{
  mx = x;
  my = y;
}

tbPoint & tbPoint::operator = (const tbPoint &p){
    if (this != &p)
    {
        mx = p.x();
        my = p.y();
    }
}

bool tbPoint::operator == (const tbPoint& p)
{
  if((mx == p.x()) && (my == p.y()))
    return true;
  else
    return false;
}

}

