#ifndef __PACKINGCOMMAND_H__
#define __PACKINGCOMMAND_H__
#include <vector>
#include <ostream>

namespace RECTPACKING {
class Layout;
class PackingCommand
{
public:
  PackingCommand() {}
  virtual ~PackingCommand() {}
  virtual void interpretToLayout(Layout &layout) = 0;
  virtual void interpretToCommand(Layout &layout) = 0;
  virtual void dump(std::ostream &out) = 0;
  virtual void next() = 0;
};

std::ostream & operator << (std::ostream &out, PackingCommand &pcmd);

}

#endif	//PACKINGCOMMAND_H
