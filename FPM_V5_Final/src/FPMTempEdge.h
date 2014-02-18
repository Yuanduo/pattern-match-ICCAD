#ifndef _FPMTEMPEDGE_H_
#define _FPMTEMPEDGE_H_
#include<vector>
#include<iostream>
#include "FPMPattern.h"
#include "FPMRect.h"
using namespace std;
namespace FPM{

typedef std::pair<int,int> TempNode;
struct FPMTempEdge
{
      TempNode tempNode;   
      int weight;
};   

typedef vector<TempNode> TempNodeVector;
typedef vector<FPMTempEdge> FPMTempEdgeVector;
typedef FPMTempEdgeVector::iterator FPMTempEdgeVectorIter;
typedef vector<int> IntVector;
std::ostream & operator << (std::ostream &out,FPMTempEdgeVector &pt);
bool DeletNode(FPMTempEdgeVector &node_vector,const TempNode &temp_node);

//FPMEdgeVector SortX(FPMPattern &pattern);
//FPMEdgeVector SortY(FPMPattern &pattern);
//FPMTempEdgeVector SweepEdgeHorizontal(FPMPattern &pattern);
//FPMTempEdgeVector SweepEdgeVertical(FPMPattern &pattern);
//TempNodeVector FindNode(FPMTempEdgeVector &node_vector,int temp_node_id);
void SortX(FPMEdgeVector &xvector);
void SortY(FPMEdgeVector &yvector);
void SweepEdgeHorizontal(FPMPattern &pattern, FPMTempEdgeVector &temp_edge_vector);
void SweepEdgeVertical(FPMPattern &pattern, FPMTempEdgeVector &temp_edge_vector);
void FindNode(FPMTempEdgeVector &node_vector,int temp_node_id, TempNodeVector &temp_node_vector);
}
#endif
