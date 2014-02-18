#include <iostream>
#include "FPMPattern.h"
#include "FPMRect.h"
#include "FPMLayout.h"
#include "FPMTempEdge.h"
#include "FPMMatch.h"
#include "Plot.h"
#include <vector>
#include <map>
#include <utility>
#include <cctype>
#include <iterator>
#include <climits>
#include <string>
#include <cassert>
#include <cstdio>
#include <cmath>
//#define DEBUGINFO 1
namespace FPM {
using namespace std;
using namespace PLOT;

void FPMPattern::clearMem()
{
  clearRectMem();
  clearPolyMem();
}
void FPMPattern::clearPolyMem()
{
  for (int j = 0; j < m_poly_fulls.size(); ++j)
  {
    m_poly_fulls[j].p.ptlist.clear();
  }
  m_poly_fulls.clear();
}
void FPMPattern::clearRectMem()
{
  rect_set.clear();
}

ostream & operator << (ostream &out, FPMPattern &pt)
{
  out<<"bbox: "<<pt.bbox.lb.x<<" "<<pt.bbox.lb.y<<" "<<pt.bbox.width<<" "<<pt.bbox.height<<endl;
  out << "number of polygons in pattern: " << pt.m_poly_fulls.size() << endl;
  out << "number of rect_set in pattern: " << pt.rect_set.size()<<endl;
  //for (int i = 0; i < pt.m_poly_fulls.size(); ++ i)
  //{
  //	out << "if full: " << pt.m_poly_fulls[i].full << endl << pt.m_poly_fulls[i].p;
  //}
  for(int i=0;i<pt.rect_set.size();i++)
  {
     out<<pt.rect_set[i].rect_id<<" "<<pt.rect_set[i].lb.x<<" "<<pt.rect_set[i].lb.y<<" "<<pt.rect_set[i].width<<" "<<pt.rect_set[i].height<<endl;
  }
  return out;
}
//wangqin changed
static void compactPolygon(FPMPoly  &p)
{
    int index = 0;
    while(p.ptlist.size() > 3 && index < p.ptlist.size())
    {
        int x = p.ptlist[index].x;
        int y = p.ptlist[index].y;        
        if (index == 0)
        {
            int preX = p.ptlist[p.ptlist.size()-1].x;
            int preY = p.ptlist[p.ptlist.size()-1].y;
            int nextX = p.ptlist[1].x;
            int nextY = p.ptlist[1].y;
            if ((preX == x && x == nextX) || (preY == y && y == nextY))
            {
                p.ptlist.erase(p.ptlist.begin());
                --index;
            }
        }
        else if (index == p.ptlist.size()-1)
        {
            int preX = p.ptlist[index-1].x;
            int preY = p.ptlist[index-1].y;
            int nextX = p.ptlist[0].x;
            int nextY = p.ptlist[0].y;
            if ((preX == x && x == nextX) || (preY == y && y == nextY))
            {
                assert(index >= 0);
                p.ptlist.erase(p.ptlist.begin()+index);
                --index;
                --index;
            }
        }
        else
        {
            assert(index >= 1);
            assert(index+1 < p.ptlist.size());
            int preX = p.ptlist[index-1].x;
            int preY = p.ptlist[index-1].y;
            int nextX = p.ptlist[index+1].x;
            int nextY = p.ptlist[index+1].y;
            if ((preX == x && x == nextX) || (preY == y && y == nextY))
            {
                p.ptlist.erase(p.ptlist.begin()+index);
                --index;
            }
        }
        ++ index;
    }
    // assert(p.ptlist.size() > 3);
}

#define BITARR

bool PTR(FPMPattern &pt,bool save)
{
	FPMLayout layout;
	pt.rect_set.clear();
	int m_rect_num=0;
	multimap<int, int> corner;
	int polygon_num=pt.m_poly_fulls.size();
	int n,x,y,index=0;
	//int bvalue=0;
//	cout<<"in PTR"<<endl;
//	cout<<"polygon num--->"<<polygon_num<<endl;
	for(int i=0;i<polygon_num;i++)
	{
		n=pt.m_poly_fulls[i].p.ptlist.size();
		if (n > 4)
			compactPolygon(pt.m_poly_fulls[i].p);
		n=pt.m_poly_fulls[i].p.ptlist.size();
		
		int x_min=INT_MAX,y_min=INT_MAX,x_max=INT_MIN,y_max=INT_MIN;
#ifdef DEBUGINFO
		cout<<"Start to fracture the polygon"<<i<<":";
#endif
		for(int j=0;j<n;j++)
		{
			x=pt.m_poly_fulls[i].p.ptlist[j].x;
			y=pt.m_poly_fulls[i].p.ptlist[j].y;
#ifdef DEBUGINFO
			cout<<"["<<x<<","<<y<<"]";
#endif
			if(x<x_min)
				x_min=x;
			if(x>x_max)
				x_max=x;
			if(y<y_min)
				y_min=y;
			if(y>y_max)
				y_max=y;
		}
#ifdef DEBUGINFO
		cout<<"n--->"<<n<<endl;
#endif
		if (n > 4)//polygon
		{

			corner.clear();
			for(int j=0;j<n;j++)
			{
				x=pt.m_poly_fulls[i].p.ptlist[j].x;
				y=pt.m_poly_fulls[i].p.ptlist[j].y;
				corner.insert(make_pair(y,x));
			}
			int x_k = INT_MAX, x_l= INT_MAX, x_m = INT_MAX;
			int y_k, y_l, y_m;
			
			while (!corner.empty())
			{
				multimap<int, int>::iterator lowest_beg = corner.begin();
				multimap<int, int>::iterator lowest_end = corner.upper_bound(lowest_beg -> first);
				multimap<int, int>::iterator Pk_it = lowest_beg;
				multimap<int, int>::iterator Pl_it = lowest_beg;
				multimap<int, int>::iterator Pm_it = lowest_end;

				x_k = INT_MAX, x_l= INT_MAX, x_m = INT_MAX;
				y_k = lowest_beg -> first;
				y_l = lowest_beg -> first;

				while (lowest_beg != lowest_end)
				{
					if (x_k > lowest_beg -> second)
					{
						x_l = x_k;
						x_k = lowest_beg -> second;
						Pl_it = Pk_it;
						Pk_it = lowest_beg;
					}
					else if (x_l > lowest_beg -> second)
					{
						x_l = lowest_beg -> second;
						Pl_it = lowest_beg;
					}
					lowest_beg++;
				}
				while (lowest_end != corner.end())
				{
					if (x_k <= lowest_end -> second && x_l >= lowest_end -> second)
					{
						x_m = lowest_end -> second;
						y_m = lowest_end -> first;
						Pm_it = lowest_end;
						break;
					}
					lowest_end++;
				}

				FPMRect temp;
				/*
				   temp.x_min = x_k;
				   temp.y_min = y_k;
				   temp.x_max = x_l;
				   temp.y_max = y_m;
				   temp.color = -1;
				 */
				int width=x_l-x_k;
				int height=y_m-y_k;
				FPMPoint point;
				point.x=x_k;
				point.y=y_k;
				temp.width=width;
				temp.height=height;
				temp.lb=point;
				temp.rect_id=m_rect_num;
				/////////////////////////////////
				//compare x1, y1, x2, y2 with bbox
#ifdef BITARR
				bool bit_array[4]={0,0,0,0};
				if(temp.lb.x==pt.bbox.lb.x)bit_array[0]=1;
				if(temp.lb.y==pt.bbox.lb.y)bit_array[1]=1;
				if(temp.lb.x+temp.width==pt.bbox.lb.x+pt.bbox.width)bit_array[2]=1;
				if(temp.lb.y+temp.height==pt.bbox.lb.y+pt.bbox.height)bit_array[3]=1;
				//bvalue=0;

				for(index=0;index<4;index++)
				{
					temp.touchArray[index]=bit_array[index];
					//if(bit_array[index]=='1')
					//bvalue=bvalue+(int)pow((double)2,(double)index);
				}
#endif
				//cout<<"in pattern touchArray[0]: "<<temp.touchArray[0]<<" touchArray[1]: "<<temp.touchArray[1]<<" touchArray[2]: "<<temp.touchArray[2]<<" touchArray[3]: "<<temp.touchArray[3]<<endl;
				//cout<<"in PTR num "<<m_rect_num<<" touchArray"<<temp.touchArray[0]<<" "<<temp.touchArray[1]<<" "<<temp.touchArray[2]<<" "<<temp.touchArray[3]<<endl;
				//getchar();
				/////////////////////////////////
				//temp.touchValue=bvalue;	
				pt.rect_set.push_back(temp);
				//pt.touchBBox.push_back(bvalue);
				//Polygon.index.push_back(m_rect_num);
				++ m_rect_num;
#ifdef DEBUGINFO
				//printf("Fractured Rectangle [%d,%d]-[%d,%d]\n", temp.x_min, temp.y_min, temp.x_max, temp.y_max);
#endif
				//assert(temp.lb.x >= x_min && temp.y_min >= y_min && temp.x_max <= x_max && temp.y_max <= y_max);

				corner.erase(Pk_it);
				corner.erase(Pl_it);
				multimap<int, int>::iterator beg = corner.lower_bound(y_m);
				multimap<int, int>::iterator end = corner.upper_bound(y_m);
				multimap<int, int>::iterator old_corner_1;
				multimap<int, int>::iterator old_corner_2;
				bool flag_1 = false, flag_2 = false;
				while (beg != end)
				{
					if (flag_1 && flag_2)
						break;
					if (beg -> second == x_k)
					{
						flag_1 = true;
						old_corner_1 = beg;
					}
					else if (beg -> second == x_l)
					{
						flag_2 = true;
						old_corner_2 = beg;
					}
					beg++;
				}
				if (!flag_1)
					corner.insert(make_pair(y_m, x_k));
				else
					corner.erase(old_corner_1);
				if (!flag_2)
					corner.insert(make_pair(y_m, x_l));
				else
					corner.erase(old_corner_2);
			}
			
			if (save)
			{
				cout<<pt.rect_set.size()<<endl;
				//cout<<x_min<<" "<<y_min<<" "<<x_max<<" "<<y_max<<endl;
				//for (int k = 0; k < (int)Polygon.rect_set.size(); k++)
				//    fout<<Polygon.rect_set[k].x_min<<" "<<Polygon.rect_set[k].y_min<<" "<<Polygon.rect_set[k].x_max<<" "<<Polygon.rect_set[k].y_max<<endl;
			}
		}
		else
		{
			if (save)
			{
				cout<<1<<endl;
				//fout<<x_min<<" "<<y_min<<" "<<x_max<<" "<<y_max<<endl;
				//fout<<x_min<<" "<<y_min<<" "<<x_max<<" "<<y_max<<endl;
			}

			//pt.rect_set.clear();
			FPMRect temp;
			/*
			   temp.x_min = x_min;
			   temp.y_min = y_min;
			   temp.x_max = x_max;
			   temp.y_max = y_max;
			   temp.color = -1;
			 */
#ifdef DEBUGINFO
			cout<<"it is a rect"<<endl;
#endif
			int width=x_max-x_min;
			int height=y_max-y_min;
			FPMPoint point;
			point.x=x_min;
			point.y=y_min;
			temp.width=width;
			temp.height=height;
			temp.lb=point;
			temp.rect_id=m_rect_num;
			//compare x1, y1, x2, y2 with bbox
#ifdef BITARR
			bool bit_array[4]={0,0,0,0};
			if(temp.lb.x==pt.bbox.lb.x)bit_array[0]=1;
			if(temp.lb.y==pt.bbox.lb.y)bit_array[1]=1;
			if(temp.lb.x+temp.width==pt.bbox.lb.x+pt.bbox.width)bit_array[2]=1;
			if(temp.lb.y+temp.height==pt.bbox.lb.y+pt.bbox.height)bit_array[3]=1;
			//bvalue=0;
			for(index=0;index<4;index++)
			{
				temp.touchArray[index]=bit_array[index];
				//if(bit_array[index]==1)
				//bvalue=bvalue+(int)pow((double)2,(double)index);
			}
#endif
			//cout<<"in PTR num "<<m_rect_num<<" touchArray"<<temp.touchArray[0]<<" "<<temp.touchArray[1]<<" "<<temp.touchArray[2]<<" "<<temp.touchArray[3]<<endl;
			//getchar();
			/////////////////////////////////
			//temp.touchValue=bvalue;
			pt.rect_set.push_back(temp);
			//pt.touchBBox.push_back(bvalue);
			//Polygon.index.push_back(m_rect_num);
			++m_rect_num;
		}
		//Polygon.bounding_box.x_min = x_min;
		//Polygon.bounding_box.y_min = y_min;
		//Polygon.bounding_box.x_max = x_max;
		//Polygon.bounding_box.y_max = y_max;
		//m_polygons.push_back(Polygon);
	}
	if (save)
	{
		cout<< m_rect_num<<endl;
		//fout.close();
	}
	//layout.drawRects(pt,"ptr_pattern");
#ifdef DEBUGINFO
	cout << "Finished fracturing polygons into rectangles."<<endl;
	cout << "Number of rectangles after fracturing: " << m_rect_num << endl;
#endif
  for (int i = 0;i < pt.m_poly_fulls.size();++i) {
  	pt.m_poly_fulls[i].p.ptlist.clear();
  }
  pt.m_poly_fulls.clear();
	return true;   
}

/*
void duplicate(vector<FPMPattern>& patterns,bool** f) {
	int boundArray[4]={0,0,0,0};
  FPMTempEdgeVector mother_edge_vertical,mother_edge_horizontal,sub_edge_vertical,sub_edge_horizontal;
  int mother_node_vertical,mother_node_horizontal,sub_node_vertical,sub_node_horizontal,match_num=0;
	cout << "patterns.size() = " << patterns.size() << endl;

	for(int i = 0;i < patterns.size();++i)
  {
//   	cout << "i = " << i << endl;
//    cout << m_subLayouts[i] << endl;
//    drawPattern(m_subLayouts[i],subLayout_file);
    //construct motherGraph
#ifdef DEBUGINFO
    cout<<"construct motherGraph"<<endl;
#endif
    mother_edge_vertical = SweepEdgeHorizontal(patterns[i]);
    mother_edge_horizontal = SweepEdgeVertical(patterns[i]);
    mother_node_vertical = patterns[i].rect_set.size();
    mother_node_horizontal = patterns[i].rect_set.size();
    for(int j = i + 1;j < patterns.size();++j)
    {
//   	cout << "j = " << j << endl;
//   	cout << total_patterns[j] << endl;
//   	drawPattern(total_patterns[j],pattern_file);
      	
#ifdef DEBUGINFO
      cout<<"construct subGraph"<<endl;
#endif
	    sub_edge_vertical = SweepEdgeHorizontal(patterns[j]);
	    sub_edge_horizontal = SweepEdgeVertical(patterns[j]);
	    sub_node_vertical = patterns[j].rect_set.size();
      sub_node_horizontal = patterns[j].rect_set.size();
	    //begin to match
	    vector<int *> result;
	    toMatchPattern(mother_node_vertical,sub_node_vertical,mother_edge_vertical,sub_edge_vertical,mother_edge_horizontal,sub_edge_horizontal,result);
		  if(result.size() != 0)
	    {   
#ifdef DEBUGINFO
	    cout<<"direct match success."<<endl;
#endif
	    for(int x = 0;x < result.size();++x)
	    {
	      boundArray[0] = 0;
	      boundArray[1] = 0;
	      boundArray[2] = 0;
	      boundArray[3] = 0;
	      if(toMatchRect(patterns[j].bbox,result[x],sub_node_vertical,patterns[i].rect_set,patterns[j].rect_set,boundArray))
	      { 
#ifdef DEBUGINFO
	        cout<<"rect match success."<<endl;  
	        cout<<"Now entering sliding matching!!!!"<<endl;
#endif
	        if(toMatchSliding(result[x],sub_node_vertical,patterns[j].rect_set,patterns[i].rect_set,sub_edge_vertical,sub_edge_horizontal,boundArray,patterns[j]))
	        {
#ifdef DEBUGINFO
		        cout<<"Sliding Match succeed!-------------------"<<endl;
#endif
		        f[i][j] = f[j][i] = true;
//		        cout << "i = " << i << endl << patterns[i] << endl;
//		        cout << "j = " << j << endl << patterns[j] << endl;
		        break;
#ifdef DEBUGINFO
		        cout<<"ffs"<<endl;
#endif
		            
	        }
	      }
	    }
////////////////////HY////////////////////////
	    for (int x = 0;x < result.size(); ++ x) {
	     	delete[] result[x];
	    }
	    result.clear();
/////////////////////////////////////////////
	    }
    }
  }
}
*/
}
