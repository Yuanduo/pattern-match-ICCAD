#ifndef __LAYOUTIO_H__
#define __LAYOUTIO_H__

#include <vector>
#include <string>
#include "Layout.h"

namespace RECTPACKING {
using namespace std;

class LayoutIO
{
public:
  LayoutIO() {}
  ~LayoutIO() {}
  void loadLayout(Layout &layout, string fileName);
  void saveLayout(Layout &layout, string fileName, bool app);
};

}

#endif	//__LAYOUTIO_H__
