#ifndef __SPPACKINGSTRATEGY_H__
#define __SPPACKINGSTRATEGY_H__
#include "PackingStrategy.h"

namespace RECTPACKING {
class Layout;
class SPPackingStrategy : public PackingStrategy
{
public:
  SPPackingStrategy():PackingStrategy() {}
  ~SPPackingStrategy() {}
  void initialPacking(Layout &layout);
  void compPackingCommand(Layout &layout);
  void compPackingLayout(Layout &layout);
  void nextPackingCommand();
};

}

#endif	//PACKINGSTRATEGY_H
