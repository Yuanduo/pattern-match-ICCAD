#ifndef POINTCOMPARATOR_H
#define POINTCOMPARATOR_H

#include "argraph.h"
class PointComparator: public AttrComparator
{ 
private:
  int floatNum;
public:
  PointComparator(int fn)
  {
     floatNum=fn;
  }
virtual bool compatible(void *pa,void *pb)
  { 
    int *a=(int *)pa;
    int *b=(int *)pb;
  
    int c;
    c = std::abs(*a-*b); 
    if(c<=floatNum)
      return true;
    else 
      return false;
  }
};
#endif
