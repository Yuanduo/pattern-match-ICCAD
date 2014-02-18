#include <iostream>
#include "FPMTempEdge.h"
#include "FPMRect.h"
#include <ctime>

namespace FPM{
using namespace std;

ostream & operator << (ostream &out,FPMTempEdgeVector &pt)
{
    for(int i=0;i<pt.size();i++)
    {
       out << "first element of pair is: " << pt[i].tempNode.first<<endl;
       out << "second element of pair is: "<< pt[i].tempNode.second<<endl;
       out << "distance between two elements is: "<<pt[i].weight<<endl;
       out << endl;
    }
    return out;
}

static bool cmpX(const FPMEdge &e1, const FPMEdge &e2)
{
  return e1.point.x < e2.point.x;
}

void SortX(FPMEdgeVector &xvector)
{
  sort(xvector.begin(), xvector.end(), cmpX);
  /*
    bool exchange;
    FPMEdgeVector xvector;
    FPMEdge temp_rect;
    xvector=pattern.horizontal_edge;
    for(int i=0;i<xvector.size();i++)
    {
        exchange=false;
        for(int j=xvector.size()-1;j>0;j--)
        {
            if(xvector[j].point.x<xvector[j-1].point.x)
            {
                temp_rect=xvector[j];
                xvector[j]=xvector[j-1];
                xvector[j-1]=temp_rect;
                exchange=true;
            }
        }
        if(!exchange){
            break;
        }
    }
    return xvector;
    */
}

static bool cmpY(const FPMEdge &e1, const FPMEdge &e2)
{
  return e1.point.y < e2.point.y;
}

void SortY(FPMEdgeVector &yvector)
{
  sort(yvector.begin(), yvector.end(), cmpY);
  /*
    bool exchange;
    FPMEdgeVector yvector;
    FPMEdge temp_rect;
    yvector=pattern.vertical_edge;
    for(int i=0;i<yvector.size();i++)
    {
        exchange=false;
        for(int j=yvector.size()-1;j>0;j--)
        {
            if(yvector[j].point.y<yvector[j-1].point.y)
            {
                temp_rect=yvector[j];
                yvector[j]=yvector[j-1];
                yvector[j-1]=temp_rect;
                exchange=true;
            }
        }
        if(!exchange){
            break;
        }
    }
    return yvector;
    */
}

bool DeletNode(FPMTempEdgeVector& node_vector, const TempNode &temp_node)
{
    int flag=0;
    FPMTempEdgeVectorIter iter;
    iter=node_vector.begin();
    while(iter!=node_vector.end())
    {
        if((*iter).tempNode==temp_node)
        {
            flag=1;
            node_vector.erase(iter);
            break;
        }
        iter++;
    }
    if(flag==1)return true;
    else return false;
}

void FindNode(FPMTempEdgeVector &node_vector,int temp_node_id, TempNodeVector &temp_node_vector)
{
    temp_node_vector.clear();
    TempNode empty_node;
    empty_node.first=-1;
    empty_node.second=-1;
    FPMTempEdgeVectorIter iter;
    iter=node_vector.begin();
    while(iter!=node_vector.end())
    {
        if((*iter).tempNode.first==temp_node_id)
        {
            temp_node_vector.push_back((*iter).tempNode);
        }
        iter++;
    }
}

bool TraverseGraph(FPMTempEdgeVector &temp_edge_vector,TempNode tempNode,int &flag,int target_first)
{
        TempNode middleNode,targetNode;
        TempNodeVector temp_node_vector;
        FindNode(temp_edge_vector,tempNode.second, temp_node_vector);
        for(int k=0;k<temp_node_vector.size();k++)
        {
            middleNode=temp_node_vector[k];
            while(true)
            {
                //cout<<"middleNode--->("<<middleNode.first<<","<<middleNode.second<<")"<<endl;
                targetNode.first=target_first;
                targetNode.second=middleNode.second;
                //cout<<"delet node----> ("<<targetNode.first<<","<<targetNode.second<<")"<<endl;
                if(DeletNode(temp_edge_vector,targetNode))
                {
                     //cout<<"delet success"<<endl;
                     flag=0;
                     tempNode.first=middleNode.first;
                     tempNode.second=middleNode.second;
                     if(TraverseGraph(temp_edge_vector,tempNode,flag,target_first))break;
                }
                else
                {
                    //cout<<"delet faile"<<endl;
                    tempNode.first=middleNode.first;
                    tempNode.second=middleNode.second;
                    if(TraverseGraph(temp_edge_vector,tempNode,flag,target_first))break;
                } 
               
            }        
        }
        return true;
}

void SweepEdgeHorizontal(FPMPattern &pattern, FPMTempEdgeVector &temp_edge_vector)//sweep horizontal edge
{
  temp_edge_vector.clear();
    FPMEdge edge;
    int j;
//long start,end;
//cout<<pattern.horizontal_edge.size()<<" ";
    FPMEdgeVector xvector=pattern.horizontal_edge;
    SortX(xvector);
//cout<<xvector.size()<<endl;
    //cout<<"xvector size is--->"<<xvector.size()<<endl;
    for(int i=0;i<xvector.size();i++)
    {
        //cout<<xvector[i];
        //getchar();
        j=i+1;
        while(j<xvector.size())
        {
            //
            if(xvector[i].point.x+xvector[i].length>xvector[j].point.x&&xvector[i].point.y>xvector[j].point.y)
            {
                //cout<<"up"<<endl;
                FPMTempEdge temp_edge;
                temp_edge.tempNode=make_pair(xvector[i].edge_id,xvector[j].edge_id);//make_pair("up","down")
                temp_edge.weight=xvector[i].point.y-xvector[j].point.y;
                temp_edge_vector.push_back(temp_edge);
                //cout<<temp_edge;
            }
            //
            if(xvector[i].point.x+xvector[i].length>xvector[j].point.x&&xvector[j].point.y>xvector[i].point.y)
            {
                //cout<<"down"<<endl;
                FPMTempEdge temp_edge;
                temp_edge.tempNode=make_pair(xvector[j].edge_id,xvector[i].edge_id);
                temp_edge.weight=xvector[j].point.y-xvector[i].point.y;
                temp_edge_vector.push_back(temp_edge);
                //cout<<temp_edge;
            }
            j++;
        }
    }
    
/*
start=clock();

end=clock();
cout<<" "<<end-start<<" ";
start=clock();


end=clock();
cout<<end-start<<endl;
  */        

/*
    TempNode tempNode,middleNode,targetNode,emptyNode;
    TempNodeVector temp_node_vector;
    FPMTempEdgeVectorIter iter;
    emptyNode.first=-1;
    emptyNode.second=-1;
    iter=temp_edge_vector.begin();
    int target_first,flag=1;
    
    while(iter!=temp_edge_vector.end())
    {   
        tempNode=(*iter).tempNode;
        //cout<<"tempNode---->("<<tempNode.first<<","<<tempNode.second<<")"<<endl;
        target_first=tempNode.first;
        TraverseGraph(temp_edge_vector,tempNode,flag,target_first);
        if(flag==0)
        {
            iter=temp_edge_vector.begin();
            flag=1;
            continue;
        }
        else  iter++;
    }
*/
}

void SweepEdgeVertical(FPMPattern &pattern, FPMTempEdgeVector &temp_edge_vector)//sweep vertical edge
{
  temp_edge_vector.clear();
    FPMEdge temp_rect;
    int j;
    FPMEdgeVector yvector=pattern.vertical_edge;
    SortY(yvector);
    for(int i=0;i<yvector.size();i++)
    {
        j=i+1;
        while(j<yvector.size())
        {
            //
            if((yvector[i].point.y+yvector[i].length>yvector[j].point.y)&&yvector[i].point.x>yvector[j].point.x)
            {
                FPMTempEdge temp_edge;
                temp_edge.tempNode=make_pair(yvector[i].edge_id,yvector[j].edge_id);//make_pair("right","left")
                temp_edge.weight=yvector[i].point.x-yvector[j].point.x;
                temp_edge_vector.push_back(temp_edge);
            }
            //
            if((yvector[i].point.y+yvector[i].length>yvector[j].point.y)&&yvector[j].point.x>yvector[i].point.x)
            {
                FPMTempEdge temp_edge;
                temp_edge.tempNode=make_pair(yvector[j].edge_id,yvector[i].edge_id);
                temp_edge.weight=yvector[j].point.x-yvector[i].point.x;
                temp_edge_vector.push_back(temp_edge);
            }
            j++;
        }
     }

  /*  
    TempNode tempNode,middleNode,targetNode,emptyNode;
    FPMTempEdgeVectorIter iter;
    emptyNode.first=-1;
    emptyNode.second=-1;
    iter=temp_edge_vector.begin();
    int target_first,flag=1;
    while(iter!=temp_edge_vector.end())
    {
        tempNode=(*iter).tempNode;
        target_first=tempNode.first;
        TraverseGraph(temp_edge_vector,tempNode,flag,target_first);
        if(flag==0)
        {
            iter=temp_edge_vector.begin();
            flag=1;
            continue;
        }
        else  iter++;

    }
    
*/
}

/*
FPMEdgeVector SortX(FPMPattern &pattern)
{
    bool exchange;
    FPMEdgeVector xvector;
    FPMEdge temp_rect;
    xvector=pattern.horizontal_edge;
    for(int i=0;i<xvector.size();i++)
    {
        exchange=false;
        for(int j=xvector.size()-1;j>0;j--)
        {
            if(xvector[j].point.x<xvector[j-1].point.x)
            {
                temp_rect=xvector[j];
                xvector[j]=xvector[j-1];
                xvector[j-1]=temp_rect;
                exchange=true;
            }
        }
        if(!exchange){
            break;
        }
    }
    return xvector;
}

FPMEdgeVector SortY(FPMPattern &pattern)
{
    bool exchange;
    FPMEdgeVector yvector;
    FPMEdge temp_rect;
    yvector=pattern.vertical_edge;
    for(int i=0;i<yvector.size();i++)
    {
        exchange=false;
        for(int j=yvector.size()-1;j>0;j--)
        {
            if(yvector[j].point.y<yvector[j-1].point.y)
            {
                temp_rect=yvector[j];
                yvector[j]=yvector[j-1];
                yvector[j-1]=temp_rect;
                exchange=true;
            }
        }
        if(!exchange){
            break;
        }
    }
    return yvector;
}

bool DeletNode(FPMTempEdgeVector& node_vector, const TempNode &temp_node)
{
    int flag=0;
    FPMTempEdgeVectorIter iter;
    iter=node_vector.begin();
    while(iter!=node_vector.end())
    {
        if((*iter).tempNode==temp_node)
        {
            flag=1;
            node_vector.erase(iter);
            break;
        }
        iter++;
    }
    if(flag==1)return true;
    else return false;
}

TempNodeVector FindNode(FPMTempEdgeVector &node_vector,int temp_node_id)
{
    TempNodeVector temp_node_vector;
    TempNode empty_node;
    empty_node.first=-1;
    empty_node.second=-1;
    FPMTempEdgeVectorIter iter;
    iter=node_vector.begin();
    while(iter!=node_vector.end())
    {
        if((*iter).tempNode.first==temp_node_id)
        {
            temp_node_vector.push_back((*iter).tempNode);
        }
        iter++;
    }
    return temp_node_vector;
}

bool TraverseGraph(FPMTempEdgeVector &temp_edge_vector,TempNode tempNode,int &flag,int target_first)
{
        TempNode middleNode,targetNode;
        TempNodeVector temp_node_vector;
        temp_node_vector=FindNode(temp_edge_vector,tempNode.second);
        for(int k=0;k<temp_node_vector.size();k++)
        {
            middleNode=temp_node_vector[k];
            while(true)
            {
                //cout<<"middleNode--->("<<middleNode.first<<","<<middleNode.second<<")"<<endl;
                targetNode.first=target_first;
                targetNode.second=middleNode.second;
                //cout<<"delet node----> ("<<targetNode.first<<","<<targetNode.second<<")"<<endl;
                if(DeletNode(temp_edge_vector,targetNode))
                {
                     //cout<<"delet success"<<endl;
                     flag=0;
                     tempNode.first=middleNode.first;
                     tempNode.second=middleNode.second;
                     if(TraverseGraph(temp_edge_vector,tempNode,flag,target_first))break;
                }
                else
                {
                    //cout<<"delet faile"<<endl;
                    tempNode.first=middleNode.first;
                    tempNode.second=middleNode.second;
                    if(TraverseGraph(temp_edge_vector,tempNode,flag,target_first))break;
                } 
               
            }        
        }
        return true;
}


FPMTempEdgeVector SweepEdgeHorizontal(FPMPattern &pattern)//sweep horizontal edge
{
    FPMTempEdgeVector temp_edge_vector;
    FPMEdgeVector xvector;
    FPMEdge edge;
    int j;
//long start,end;
//cout<<pattern.horizontal_edge.size()<<" ";
    xvector=SortX(pattern);
//cout<<xvector.size()<<endl;
    //cout<<"xvector size is--->"<<xvector.size()<<endl;
    for(int i=0;i<xvector.size();i++)
    {
        //cout<<xvector[i];
        //getchar();
        j=i+1;
        while(j<xvector.size())
        {
            //
            if(xvector[i].point.x+xvector[i].length>xvector[j].point.x&&xvector[i].point.y>xvector[j].point.y)
            {
                //cout<<"up"<<endl;
                FPMTempEdge temp_edge;
                temp_edge.tempNode=make_pair(xvector[i].edge_id,xvector[j].edge_id);//make_pair("up","down")
                temp_edge.weight=xvector[i].point.y-xvector[j].point.y;
                temp_edge_vector.push_back(temp_edge);
                //cout<<temp_edge;
            }
            //
            if(xvector[i].point.x+xvector[i].length>xvector[j].point.x&&xvector[j].point.y>xvector[i].point.y)
            {
                //cout<<"down"<<endl;
                FPMTempEdge temp_edge;
                temp_edge.tempNode=make_pair(xvector[j].edge_id,xvector[i].edge_id);
                temp_edge.weight=xvector[j].point.y-xvector[i].point.y;
                temp_edge_vector.push_back(temp_edge);
                //cout<<temp_edge;
            }
            j++;
        }
    }
    
/*
start=clock();

end=clock();
cout<<" "<<end-start<<" ";
start=clock();


end=clock();
cout<<end-start<<endl;
  */        

/*
    TempNode tempNode,middleNode,targetNode,emptyNode;
    TempNodeVector temp_node_vector;
    FPMTempEdgeVectorIter iter;
    emptyNode.first=-1;
    emptyNode.second=-1;
    iter=temp_edge_vector.begin();
    int target_first,flag=1;
    
    while(iter!=temp_edge_vector.end())
    {   
        tempNode=(*iter).tempNode;
        //cout<<"tempNode---->("<<tempNode.first<<","<<tempNode.second<<")"<<endl;
        target_first=tempNode.first;
        TraverseGraph(temp_edge_vector,tempNode,flag,target_first);
        if(flag==0)
        {
            iter=temp_edge_vector.begin();
            flag=1;
            continue;
        }
        else  iter++;
    }
*/
/*
    return temp_edge_vector;

}

FPMTempEdgeVector SweepEdgeVertical(FPMPattern &pattern)//sweep vertical edge
{
    FPMTempEdgeVector temp_edge_vector;
    FPMEdgeVector yvector;
    FPMEdge temp_rect;
    int j;
    yvector=SortY(pattern);
    for(int i=0;i<yvector.size();i++)
    {
        j=i+1;
        while(j<yvector.size())
        {
            //
            if((yvector[i].point.y+yvector[i].length>yvector[j].point.y)&&yvector[i].point.x>yvector[j].point.x)
            {
                FPMTempEdge temp_edge;
                temp_edge.tempNode=make_pair(yvector[i].edge_id,yvector[j].edge_id);//make_pair("right","left")
                temp_edge.weight=yvector[i].point.x-yvector[j].point.x;
                temp_edge_vector.push_back(temp_edge);
            }
            //
            if((yvector[i].point.y+yvector[i].length>yvector[j].point.y)&&yvector[j].point.x>yvector[i].point.x)
            {
                FPMTempEdge temp_edge;
                temp_edge.tempNode=make_pair(yvector[j].edge_id,yvector[i].edge_id);
                temp_edge.weight=yvector[j].point.x-yvector[i].point.x;
                temp_edge_vector.push_back(temp_edge);
            }
            j++;
        }
     }

  /*  
    TempNode tempNode,middleNode,targetNode,emptyNode;
    FPMTempEdgeVectorIter iter;
    emptyNode.first=-1;
    emptyNode.second=-1;
    iter=temp_edge_vector.begin();
    int target_first,flag=1;
    while(iter!=temp_edge_vector.end())
    {
        tempNode=(*iter).tempNode;
        target_first=tempNode.first;
        TraverseGraph(temp_edge_vector,tempNode,flag,target_first);
        if(flag==0)
        {
            iter=temp_edge_vector.begin();
            flag=1;
            continue;
        }
        else  iter++;

    }
    
*/
/*
    return temp_edge_vector;
}
*/

}
