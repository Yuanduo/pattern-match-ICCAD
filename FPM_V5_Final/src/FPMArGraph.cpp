#include "FPMArGraph.h"
#include "vf2_sub_state.h"
#include "ull_sub_state.h"
#include "vf_sub_state.h"
#include "vf2_state.h"
#include "argraph.h"
#include "EdgeComparator.h"
#include "EdgeDestroyer.h"
#include <vector>
#include <ctime>

using namespace std;

int FPM_thu_num;
const int FPM_thu_num_MAX =30000;

bool my_visitor(int n,node_id ni1[],node_id ni2[],void *user_data){
  //cout<<"in vistor, n is : "<<n<<endl;
  //for(int i=0;i<n;i++)
    //cout<<"matching pair:"<<ni1[i]<<" "<<ni2[i]<<endl;
  ++FPM_thu_num;
  vector<FPMResultPair> *result=(vector<FPMResultPair>*)user_data;
  FPMResultPair temp_result;
  temp_result.sub_result = new node_id[n];
  temp_result.target_result = new node_id[n];
  for(int i=0;i<n;i++)
  {
    temp_result.sub_result[i] =ni1[i];
    temp_result.target_result[i] =ni2[i];
  }
  result->push_back(temp_result);
  /*
  if(FPM_thu_num==FPM_thu_num_MAX){
     return true;
  }
  */
  
  return false;
}

vector<FPMResultPair> FPMArGraph::getMatch(){
  //Build Graph
  ARGraph<void,int> target_graph(&target_edit);
  ARGraph<void,int> sub_graph(&sub_edit);
  //for(int i=0;i<target_
    sub_graph.SetEdgeComparator(new EdgeComparator(0));
  
  VF2SubState s0(&sub_graph,&target_graph);
  
  vector<FPMResultPair> result;
   
  FPM_thu_num=0;
   match(&s0,my_visitor,&result);
 
  return result;

}

