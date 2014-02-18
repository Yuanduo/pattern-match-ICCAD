#ifndef FPMARGRAPH_H
#define FPMARGRAPH_H

#include "argraph.h"
#include <match.h>
#include "argedit.h"
#include "FPMTempEdge.h"
#include <vector>
#include <ctime>

using namespace std;
using namespace FPM;



struct FPMResultPair{
  node_id* sub_result;
  node_id* target_result;
};
   
bool my_visitor(int n, node_id ni1[],node_id ni2[],void *user_data);

class FPMArGraph{
  private:
    ARGEdit target_edit;
    ARGEdit sub_edit;
    int* tempWeight;
    int* tempWeight2;
  public:
    FPMArGraph(int target_num,int sub_num,FPMTempEdgeVector target_source,FPMTempEdgeVector sub_source,int* rF){
        //Insert node
      for(int i=0;i<target_num;i++)
        target_edit.InsertNode(NULL);
      for(int i=0;i<sub_num;i++)
        sub_edit.InsertNode(NULL);         
    //Insert edge
      tempWeight=new int[target_source.size()];
      tempWeight2=new int[sub_source.size()];
    
        for(int i=0;i<target_source.size();i++)
	{ tempWeight[i]=target_source[i].weight;
          //cout<<"now insert edge "<<i<<endl;
	  target_edit.InsertEdge(target_source[i].tempNode.first,target_source[i].tempNode.second,&tempWeight[i]);
      }
     

      for(int i=0;i<sub_source.size();i++)      
	{ tempWeight2[i]=sub_source[i].weight;
         // cout<<"now insert edge "<<i<<endl; 
	  sub_edit.InsertEdge(rF[sub_source[i].tempNode.first],rF[sub_source[i].tempNode.second],&tempWeight2[i]);
      }
    }
    ~FPMArGraph(){
        delete [] tempWeight;
        delete [] tempWeight2;
    
    }
 
    vector<FPMResultPair> getMatch();
            
};
#endif
