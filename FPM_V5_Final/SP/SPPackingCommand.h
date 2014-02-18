#ifndef __SPPACKINGCOMMAND_H__
#define __SPPACKINGCOMMAND_H__
#include "PackingCommand.h"

namespace RECTPACKING {

class SPPackingCommand : public PackingCommand
{
public:
  SPPackingCommand(std::vector<int> &s1, std::vector<int> &s2):PackingCommand() {
    m_s1 = s1;
    m_s2 = s2;
  }
  ~SPPackingCommand() {}
  void interpretToLayout(Layout &layout);
  void interpretToCommand(Layout &layout);
  void dump(std::ostream &out);
  void next();
  
protected:
  void change(std::vector<int> &m_s);
  
  std::vector<int> m_s1;
  std::vector<int> m_s2;
};

}

#endif	//__SPPACKINGCOMMAND_H__
