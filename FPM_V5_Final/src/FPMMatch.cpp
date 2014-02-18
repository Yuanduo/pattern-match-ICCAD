#include "FPMMatch.h"
#include "FPMTempEdge.h"
#include "argraph.h"
#include "argedit.h"
#include "FPMLayout.h"
#include <vector>
#include <math.h>
#include <iostream>
#include <sstream>
#include <fstream.h>
#include <ctime>
#include "vf2_sub_state.h"
#include "ull_sub_state.h"
#include "vf_sub_state.h"
#include "vf2_state.h"
#include "EdgeComparator.h"
#include "EdgeDestroyer.h"
#include "PiontComparator.h"


extern int WIDTH_DIFF;
extern int HEIGHT_DIFF;
extern int GRAPH_EDGE_DIFF;
extern int POLY_EDGE_DIFF;
namespace FPM{

using namespace std;

int FPM_thu_num;
const int FPM_thu_num_MAX =30000;

bool my_visitor(int n,node_id ni1[],node_id ni2[],void *user_data){
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
  if(FPM_thu_num==FPM_thu_num_MAX){
     return true;
  }
  return false;
}

ARGraph<void,int> *toGetSubG(int sub_num,const FPMTempEdgeVector &sub_source,int *F,int* tempWeight2)
{
  int *rF=new int[F[sub_num-1]+1];
  //int rf_size=F[sub_num-1]+1;
  for(int i=0;i<sub_num;i++)
  { 
     rF[F[i]]=i;
  }
  ARGEdit sub_edit;
  for(int i=0;i<sub_num;i++)
    sub_edit.InsertNode(NULL);     
  //int* tempWeight2=new int[sub_source.size()];
  for(int i=0;i<sub_source.size();i++)      
  { tempWeight2[i]=sub_source[i].weight;
    sub_edit.InsertEdge(rF[sub_source[i].tempNode.first],rF[sub_source[i].tempNode.second],&tempWeight2[i]);
  }

  //ARGraph<void,int> sub_graph(&sub_edit);
  ARGraph<void,int> *sub_graph = new ARGraph<void,int>(&sub_edit);

  sub_graph->SetEdgeComparator(new EdgeComparator(GRAPH_EDGE_DIFF));
  //sub_graph->SetNodeComparator(new PointComparator(POLY_EDGE_DIFF));
  delete [] rF;
  
  return sub_graph;
}

ARGraph<void,int> *toGetTargetG(int target_num,const FPMTempEdgeVector &target_source,int* tempWeight)
{ ARGEdit target_edit;
  for(int i=0;i<target_num;i++)
    target_edit.InsertNode(NULL);
  //int* tempWeight=new int[target_source.size()];
  for(int i=0;i<target_source.size();i++)
  { tempWeight[i]=target_source[i].weight;
    target_edit.InsertEdge(target_source[i].tempNode.first,target_source[i].tempNode.second,&tempWeight[i]);
  }
//  ARGraph<void,int> target_graph(&target_edit);
  ARGraph<void,int> *target_graph = new ARGraph<void,int>(&target_edit);
  
  return target_graph;
}

void toMatchEdge(ARGraph<void,int> *sub_graph, ARGraph<void,int> *target_graph,
                  int sub_num, int* F, vector<FPMResultPair>& result_pair)
{
  //cout<<"out the sub graph :"<<target_graph->NodeCount()<<endl;
  //for(int i=0;i<target_graph->NodeCount();i++)
  //{ cout<<i<<": "<<target_graph->InEdgeCount(i)<<" "<<target_graph->OutEdgeCount(i)<<endl;
  //}
   VF2SubState s0(sub_graph, target_graph);
   FPM_thu_num=0;
  // cout<<"in"<<sub_graph->NodeCount()<<" "<<target_graph->NodeCount()<<endl;
   match(&s0,my_visitor,&result_pair);   
   for(int i=0;i<result_pair.size();i++)
   {
      for(int j=0;j<sub_num;j++)
      {
         result_pair[i].sub_result[j] = F[result_pair[i].sub_result[j]];
      }
   }
}

/*
void reOutput(const FPMPattern &pattern, const FPMPattern &sublayout, FPMResultPair &result, const FPMTempEdgeVector &source, ofstream &fout)
{ // cout<<"come intytrytrytr"<<endl;
  int x,y;
  int target;
  int count_i=0;
  if (pattern.horizontal_edge.size()!=0&&pattern.vertical_edge.size()!=0)
  {
    for(int i=0;i<pattern.m_edge.size();i++)
    {   
       if(result.sub_result[i]==pattern.horizontal_edge[0].edge_id)
       {
          target=i;
          break;
       }
       count_i=i;
    }
    if(count_i==pattern.m_edge.size()-1)
    {  fout<<"mtach success, but erros occur while output."<<endl;
       return;
    }
    /* cout << pattern.bbox.lb.y << endl;
     cout << pattern.bbox.height << endl;
     cout << pattern.horizontal_edge[0].point.y << endl;
     cout << target << endl;
     cout << result.target_result[target] << endl;
     cout << sublayout.m_edge[result.target_result[target]].point.y << endl;
     */
     /*
     y=(pattern.bbox.lb.y*2+pattern.bbox.height)/2-pattern.horizontal_edge[0].point.y+sublayout.m_edge[result.target_result[target]].point.y;
     count_i=0;
     for(int i=0;i<pattern.m_edge.size();i++)
     {
       if(result.sub_result[i]==pattern.vertical_edge[0].edge_id)
       {
        target=i;
        break;
       }
       count_i=i;
     }
    if(count_i==pattern.m_edge.size()-1)
    {  fout<<"mtach success, but erros occur while output."<<endl;
       return;
    }
     x=(pattern.bbox.lb.x*2+pattern.bbox.width)/2-pattern.vertical_edge[0].point.x+sublayout.m_edge[result.target_result[target]].point.x;
  }
  else
  { 
   //cout<<"else"<<endl;
    for(int i=0;i<pattern.m_edge.size();i++)
    {
      if(result.sub_result[i]==pattern.m_edge[0].edge_id)
      {
        target=i;
        break;
      }
      count_i=i;
    }
    if(count_i==pattern.m_edge.size()-1)
    {  fout<<"mtach success, but erros occur while output."<<endl;
       return;
    }
    x=(pattern.bbox.lb.x*2+pattern.bbox.width)/2-pattern.m_edge[0].point.x+sublayout.m_edge[result.target_result[target]].point.x;
    y=(pattern.bbox.lb.y*2+pattern.bbox.height)/2-pattern.m_edge[0].point.y+sublayout.m_edge[result.target_result[target]].point.y;
  }
  
  fout<<"x:"<<x<<" "<<"y:"<<y<<endl;
  //FPMLayout layout;
  //layout.drawBox(sublayout,x,y);
  //cout<<"come out"<<endl;
}
*/

void reOutput2(const FPMPattern &sublayout,vector<FPMPoint>& mset)
{ 
	int x,y;
  x=sublayout.bbox.lb.x+sublayout.bbox.width/2;
  y=sublayout.bbox.lb.y+sublayout.bbox.height/2;

  FPMPoint pnt;
  pnt.x = x;
  pnt.y = y;
  mset.push_back(pnt);
  //fout<<"x:"<<x<<" "<<"y:"<<y<<endl;
}

void reOutput3(const FPMPattern &sublayout,const FPMEdgeVector &edge_vector,const FPMRect &bbox,FPMResultPair &result,vector<FPMPoint>& mset)
{
	FPMEdgeVector h_edge,v_edge;
  for(int i=0;i<edge_vector.size();i++)
  {
		if(edge_vector[i].type==0)
			v_edge.push_back(edge_vector[i]);
		else
			h_edge.push_back(edge_vector[i]);
  }
	int x,y;
  int target = -1;
  if (h_edge.size()!=0 && v_edge.size()!=0)
  {
    for(int i=0;i<edge_vector.size();i++)
    {
       if(result.sub_result[i] == h_edge[0].edge_id)
       {
          target=i;
          break;
       }
    }
    if(target < 0)
    {
      cout << "ERROR: hor edge not found" << endl;
      return;
    }
    if (target >= edge_vector.size() || result.target_result[target] >= sublayout.m_edge.size())
   	{
      cout << "ERROR: index out of bound" << endl;
      return;
    }
    /*
		y=(bbox.lb.y*2+bbox.height)/2
		-h_edge[0].point.y
		+sublayout.m_edge[result.target_result[target]].point.y;
		*/
		y = sublayout.m_edge[result.target_result[target]].point.y
				- (h_edge[0].point.y - bbox.lb.y);
		
     target = -1;
     for(int i=0;i<edge_vector.size();i++)
     {
       if(result.sub_result[i]==v_edge[0].edge_id)
       {
        target=i;
        break;
       }
     }
    if(target < 0)
    {
      cout << "ERROR: hor edge not found" << endl;
      return;
    }
    if (target >= edge_vector.size() || result.target_result[target] >= sublayout.m_edge.size())
    {
      cout << "ERROR: index out of bound" << endl;
      return;
    }
    
     //x=(bbox.lb.x*2+bbox.width)/2-v_edge[0].point.x+sublayout.m_edge[result.target_result[target]].point.x;
    x = sublayout.m_edge[result.target_result[target]].point.x
    		- (v_edge[0].point.x - bbox.lb.x);
  }
  else
  {
  	cout << "ERROR: Impossible branch" << endl;
  	return;
  	
   //cout<<"else"<<endl;
    for(int i=0;i<edge_vector.size();i++)
    {
      if(result.sub_result[i] == edge_vector[0].edge_id)
      {
        target=i;
        break;
      }
    }
    if(target < 0)
    {
      return;
    }
    
    if (target >= edge_vector.size() || result.target_result[target] >= sublayout.m_edge.size())
      return;
    
    x=(bbox.lb.x*2+bbox.width)/2-edge_vector[0].point.x+sublayout.m_edge[result.target_result[target]].point.x;
    y=(bbox.lb.y*2+bbox.height)/2-edge_vector[0].point.y+sublayout.m_edge[result.target_result[target]].point.y;
  }
  
  if (x < sublayout.bbox.lb.x || x > sublayout.bbox.lb.x+sublayout.bbox.width
  		|| y < sublayout.bbox.lb.y || y > sublayout.bbox.lb.y+sublayout.bbox.height)
 	{
 		cout << "ERROR: point out of bbox of sublayout" << endl;
 		return;
 	}
 	
  FPMPoint pnt;
  pnt.x = x;
  pnt.y = y;  
  mset.push_back(pnt);
  
  FPMLayout layout;
  layout.drawBox(sublayout,x,y);
  cout<<"come out"<<endl;
  
}

/*void reOutput(const FPMPattern &pattern, const FPMPattern &sublayout, FPMResultPair &result, const FPMTempEdgeVector &source, ofstream &fout)
{ // cout<<"come intytrytrytr"<<endl;
  int x,y;
  int target = -1;
  if (pattern.horizontal_edge.size()!=0&&pattern.vertical_edge.size()!=0)
  {
    for(int i=0;i<pattern.m_edge.size();i++)
    {   
       if(result.sub_result[i]==pattern.horizontal_edge[0].edge_id)
       {
          target=i;
          break;
       }
    }
    if(target < 0)
    {
      fout<<"mtach success, but erros occur while output."<<endl;
      return;
    }
   
    if (target >= pattern.m_edge.size() || result.target_result[target] >= sublayout.m_edge.size())
      return;
    
     y=(pattern.bbox.lb.y*2+pattern.bbox.height)/2-pattern.horizontal_edge[0].point.y+sublayout.m_edge[result.target_result[target]].point.y;

     target = -1;
     for(int i=0;i<pattern.m_edge.size();i++)
     {
       if(result.sub_result[i]==pattern.vertical_edge[0].edge_id)
       {
        target=i;
        break;
       }
     }
    if(target < 0)
    {
      fout<<"mtach success, but erros occur while output."<<endl;
      return;
    }
    if (target >= pattern.m_edge.size() || result.target_result[target] >= sublayout.m_edge.size())
      return;
    
     x=(pattern.bbox.lb.x*2+pattern.bbox.width)/2-pattern.vertical_edge[0].point.x+sublayout.m_edge[result.target_result[target]].point.x;
  }
  else
  { 
   //cout<<"else"<<endl;
    for(int i=0;i<pattern.m_edge.size();i++)
    {
      if(result.sub_result[i]==pattern.m_edge[0].edge_id)
      {
        target=i;
        break;
      }
    }
    if(target < 0)
    {
      fout<<"mtach success, but erros occur while output."<<endl;
      return;
    }
    
    if (target >= pattern.m_edge.size() || result.target_result[target] >= sublayout.m_edge.size())
      return;
    
    x=(pattern.bbox.lb.x*2+pattern.bbox.width)/2-pattern.m_edge[0].point.x+sublayout.m_edge[result.target_result[target]].point.x;
    y=(pattern.bbox.lb.y*2+pattern.bbox.height)/2-pattern.m_edge[0].point.y+sublayout.m_edge[result.target_result[target]].point.y;
  }
  
  fout<<"x:"<<x<<" "<<"y:"<<y<<endl;*/
  //FPMLayout layout;
  //layout.drawBox(sublayout,x,y);
  //cout<<"come out"<<endl;

void reOutput(const FPMPattern& sublayout,FPMResultPair &result,vector<FPMPoint>& mset,int num)
{
	int x_max=INT_MIN;
	int y_max=INT_MIN;
  int x_min=INT_MAX;
  int y_min=INT_MAX;
  
  for(int i=0;i<num;i++)
  {
    if(sublayout.m_edge[result.target_result[i]].type==0)
    {
      if(sublayout.m_edge[result.target_result[i]].point.x>x_max)
        x_max=sublayout.m_edge[result.target_result[i]].point.x;
      if(sublayout.m_edge[result.target_result[i]].point.x<x_min)
        x_min=sublayout.m_edge[result.target_result[i]].point.x;
      if(sublayout.m_edge[result.target_result[i]].point.y<y_min)
        y_min=sublayout.m_edge[result.target_result[i]].point.y;
      if(sublayout.m_edge[result.target_result[i]].point.y+sublayout.m_edge[result.target_result[i]].length>y_max)
        y_max=sublayout.m_edge[result.target_result[i]].point.y+sublayout.m_edge[result.target_result[i]].length;
    }
    else
    {
      if(sublayout.m_edge[result.target_result[i]].point.y>y_max)
        y_max=sublayout.m_edge[result.target_result[i]].point.y;
      if(sublayout.m_edge[result.target_result[i]].point.y<y_min)
        y_min=sublayout.m_edge[result.target_result[i]].point.y;
      if(sublayout.m_edge[result.target_result[i]].point.x<x_min)
        x_min=sublayout.m_edge[result.target_result[i]].point.x;
      if(sublayout.m_edge[result.target_result[i]].point.x+sublayout.m_edge[result.target_result[i]].length>x_max)
        x_max=sublayout.m_edge[result.target_result[i]].point.x+sublayout.m_edge[result.target_result[i]].length;   
    }
  }
  FPMPoint fp;
  fp.x=(x_max+x_min)/2;
  fp.y=(y_max+y_min)/2;
  mset.push_back(fp);
  
}








}

