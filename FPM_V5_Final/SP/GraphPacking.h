#ifndef __GRAPHPACKING_H__
#define __GRAPHPACKING_H__
#include <vector>
#include <utility>

namespace RECTPACKING {

class SPPackingCommand;
class Layout;

void LongestGraphPacking(std::vector<std::pair<int, int> > &horCons, std::vector<std::pair<int, int> > &verCons, Layout &layout);
void topoSortForSP(std::vector<std::pair<int, int> > &horCons, std::vector<std::pair<int, int> > &verCons, Layout &layout, std::vector<int> &s1, std::vector<int> &s2);

}

#endif	//__GRAPHPACKING_H__
