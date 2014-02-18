#ifndef EDGECOMPARATOR_H
#define EDGECOMPARATOR_H

#include <algorithm>
#include "argraph.h"

class EdgeComparator: public AttrComparator{
private:
  int floatNum;

public:
  EdgeComparator(int fn) 
  {
    floatNum=fn;
  }
  
  virtual bool compatible(void *pa, void *pb)
  {
    int c = std::abs(*((int *)pa)-*((int *)pb));
    return c <= floatNum;
  }
};

#endif
