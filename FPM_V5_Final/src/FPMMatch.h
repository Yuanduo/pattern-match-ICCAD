#ifndef FPMMATCH_H_
#define FPMMATCH_H_

#include <fstream>
#include "FPMArGraph.h"
#include "FPMTempEdge.h"
namespace FPM{
using namespace std;
  struct FPMPoint;

bool my_visitor(int n,node_id ni1[],node_id ni2[],void *user_data);
ARGraph<void,int> *toGetSubG(int sub_num,const FPMTempEdgeVector &sub_source,int *F,int* tempWeghht2);
ARGraph<void,int> *toGetTargetG(int target_num,const FPMTempEdgeVector &target_source,int* tempWeight);
void toMatchEdge(ARGraph<void,int> *sub_graph,ARGraph<void,int> *target_graph,int sub_num,int* F,   vector<FPMResultPair> &result_pair);

void reOutput(const FPMPattern& sublayout,FPMResultPair &result,vector<FPMPoint>& mset,int num);

//void reOutput(const FPMPattern &pattern, const FPMPattern &sublayout, FPMResultPair &result, const FPMTempEdgeVector &source, std::ofstream &fout);

void reOutput2(const FPMPattern &sublayout,vector<FPMPoint>& mset);

void reOutput3(const FPMPattern &sublayout,const FPMEdgeVector &edge_vector,const FPMRect &bbox,FPMResultPair &result,vector<FPMPoint>& mset);

}



#endif
