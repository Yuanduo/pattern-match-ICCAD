#ifndef __FPMPATTERN_H__
#define __FPMPATTERN_H__
#include <ostream>
#include "FPMPoly.h"
#include "FPMRect.h"
#include "FPMEdge.h"
#include <vector>

namespace FPM {

struct FPMPoly_full
{
	bool full;
	FPMPoly p;
};
  
struct FPMPattern
{
  //wangqin changed
  FPMRect bbox;
  FPMRectVector rect_set;
  std::vector< std::vector<FPMEdge> > m_edge_vector;
  std::vector<FPMEdge> m_edge;
  std::vector<FPMEdge> vertical_edge;
  std::vector<FPMEdge> horizontal_edge;
        
	std::vector<FPMPoly_full> m_poly_fulls;
  void clearPolyMem();
  void clearRectMem();
  void clearMem();
};
typedef std::vector<FPMPattern> FPMPatternVector;
typedef FPMPatternVector::iterator FPMPatternVectorIter;
typedef std::vector<FPMEdge> FPMEdgeVector;
typedef std::vector<FPMEdgeVector> FPMPolyEdgeVector;
typedef FPMEdgeVector::iterator FPMEdgeVectorIter; 
typedef FPMPolyEdgeVector::iterator FPMPolyEdgeVectorIter;
std:: ostream & operator << (std::ostream &out, FPMPattern &pt);
//wangqin changed
bool PTR(FPMPattern &pt,bool save);
static void compactPolygon(FPMPoly &p);
//void duplicate(vector<FPMPattern>& patterns,bool** f);

}

#endif  //FPMPATTERN_H

