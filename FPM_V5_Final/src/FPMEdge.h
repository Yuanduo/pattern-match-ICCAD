#ifndef _FPMEDGE_H_
#define _FPMEDGE_H_
#include "FPMPoint.h"
#include <iostream>
#include <vector>
namespace FPM{
struct FPMEdge
{
  FPMPoint point;
  int length;
  int edge_id;
  bool type;//0 is vertical,1 is horizonatl
  bool bound;//0 represents not bounding, 1 represents bounding
  int belong_id;
};

typedef std::vector<FPMEdge> FPMEdgeVector;
/*
std::ostream & operator << (std::ostream &out,FPMEdgeVector &pt)
{
    for(int i=0;i<pt.size();i++)
    {
       out << "point x is: " << pt[i].point.x<<endl;
       out << "point y is: "<< pt[i].point.y<<endl;
       out << "edge_id is: "<<pt[i].edge_id<<endl;
       out << "belong_id is: "<<pt[i].belong_id<<endl;
       out << endl;
    }
    return out;
};
*/
}
#endif
