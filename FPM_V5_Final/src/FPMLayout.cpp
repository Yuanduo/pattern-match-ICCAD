#include <iostream>
#include <cassert>
#include <list>
#include "FPMLayout.h"
#include "Plot.h"
#include "FPMPattern.h"
#include "Kbool4Router.h"
#include "FPMTempEdge.h"
#include "argraph.h"
#include "FPMMatch.h"
#include "vf2_state.h"
#include "EdgeComparator.h"
#include "EdgeDestroyer.h"
#include <ctime>

extern int S1_DISTANCE;
extern int S2_DISTANCE;
extern int MATCH_COUNT;
extern int MEDGE_SIZE;
extern int ADD_COUNT;

namespace FPM {
using namespace std;
using namespace PLOT;


static bool checkPoly(FPMPoly &p)
{
  int preX = p.ptlist[p.ptlist.size()-1].x;
  int preY = p.ptlist[p.ptlist.size()-1].y;
  for (int j = 0; j < p.ptlist.size(); ++j)
  {
    if (p.ptlist[j].x != preX && p.ptlist[j].y != preY)
    {
      cout << "WARNING: non-rectangular polygons found and skipped!" << endl;
      return false;
    }
    preX = p.ptlist[j].x;
    preY = p.ptlist[j].y;
  }
  return true;
}

void FPMLayout::addPoly(FPMPoly &p) {
  if (checkPoly(p))
    m_polys.push_back(p);
}

bool FPMLayout::within_box(int x,int y, FPMRect mwindow)
{
   if(x>mwindow.lb.x&&x<mwindow.lb.x+mwindow.width&&y>mwindow.lb.y&&y<mwindow.lb.y+mwindow.height)return true;
   else return false; 		
}			 
			  
void FPMLayout::FinalResult(const std::vector<FPMPoint> &bad_point,const std::vector<FPMPoint> &good_point,ofstream &fout)
	{
		cout<<"bad point size: "<<bad_point.size()<<endl;
		cout<<"good point size: "<<good_point.size()<<endl;
		vector<FPMRect> mwindows;
		vector<int> mNum;
    FPMRect mwindow;
		
		
		//for bad point
		for(int i=0;i<bad_point.size();i++)
		{
			//compute window position
		  mwindow.lb.x=bad_point[i].x-600;
		  mwindow.lb.y=bad_point[i].y-600;
		  mwindow.width=1200;
		  mwindow.height=1200;	
		  //do wee need this window
		  bool needOne = true;
		  for(int k=0;k<mwindows.size();k++)
		  {
		  	  FPMRect win = mwindows[k];
          if (((float)(win.lb.x+600-(mwindow.lb.x+600))*(win.lb.x-(mwindow.lb.x+600))
                    +(float)(win.lb.y+600-(mwindow.lb.y+600))*(win.lb.y+600-(mwindow.lb.y+600))) <= (1200*0.5)*(1200*0.5))
          {
            needOne = false;
            break;
          }
		  }
		  if (needOne) //yes, we need one window
        {
          mwindows.push_back(mwindow);
          mNum.push_back(0);
        }
		}
		//cout<<"mwindows size---->"<<mwindows.size()<<endl;
		for(int i=0;i<bad_point.size();i++)
		{
		  //update the windows for covered matching points
      for (int k = 0; k < mwindows.size(); ++ k)
      {
          if (within_box(bad_point[i].x, bad_point[i].y ,mwindows[k]))
          {
            ++ mNum[k];
          }
      }
    }
    //cout<<"come in good point"<<endl;
    
    //for good point
    vector<FPMRect> mgoodwindows;
		vector<int> mgoodNum;
    FPMRect mgoodwindow;
		for(int i=0;i<good_point.size();i++)
		{
			//compute good window position
		  mgoodwindow.lb.x=good_point[i].x-600;
		  mgoodwindow.lb.y=good_point[i].y-600;
		  mgoodwindow.width=1200;
		  mgoodwindow.height=1200;	
		  //do wee need this good window
		  bool gneedOne = true;
		  for(int k=0;k<mgoodwindows.size();k++)
		  {
		  	  FPMRect gwin = mgoodwindows[k];
          if (((float)(gwin.lb.x+600-(mgoodwindow.lb.x+600))*(gwin.lb.x-(mgoodwindow.lb.x+600))
                    +(float)(gwin.lb.y+600-(mgoodwindow.lb.y+600))*(gwin.lb.y+600-(mgoodwindow.lb.y+600))) <= (1200*0.5)*(1200*0.5))
          {
            gneedOne = false;
            break;
          }
		  }
		  if (gneedOne) //yes, we need one good window
        {
          mgoodwindows.push_back(mgoodwindow);
          mgoodNum.push_back(0);
        }
		}
		//cout<<"good result finished"<<endl;
		for(int i=0;i<good_point.size();i++)
		{
		  //update the good windows for covered matching points
      for (int k = 0; k < mgoodwindows.size(); ++ k)
      {
          if (within_box(good_point[i].x, good_point[i].y ,mgoodwindows[k]))
          {
            ++ mgoodNum[k];
          }
      }
    }
   // cout<<"final result end"<<endl;
    //cout<<"mwindow size: "<<mwindows.size()<<endl;
    //claculate final result
    for (int i=0;i<mwindows.size();i++)
    {
    	//cout<<"come in last loop"<<endl;
    	if (mNum[i] < 1)
    	{
    		//cout<<"erase"<<endl;
      	mwindows.erase(mwindows.begin()+i);
      	-- i;
      	continue;
    	}
    	
    	FPMRect mrect=mwindows[i];
    	for(int j=0;j<mgoodwindows.size();j++)
    	{
    		//good box and bad box overlap enough
    		if (((float)(mrect.lb.x+600-(mgoodwindow.lb.x+600))*(mrect.lb.x-(mgoodwindow.lb.x+600))
                    +(float)(mrect.lb.y+600-(mgoodwindow.lb.y+600))*(mrect.lb.y+600-(mgoodwindow.lb.y+600))) <= (1200*0.5)*(1200*0.5))
          {
            if (mNum[i] < mgoodNum[j])
            {
            	mwindows.erase(mwindows.begin()+i);
            	-- i;
            }
          }
    	}
    	//cout<<"cout out final result"<<endl;
    	//give final output
    	
    }		
	
	  finalOutput(mwindows,fout); 
	   
	}

void FPMLayout::finalOutput(const std::vector<FPMRect> &windows,ofstream &fout)
{
  vector<FPMPoint> mset;
  
  cout<<"come in final output"<<endl;
  int x=0,y=0;
  for(int i=0;i<windows.size();i++)
  {
    x=windows[i].lb.x+600;
    y=windows[i].lb.y+600;
    fout<<"("<<x<<","<<y<<")"<<endl;
    FPMPoint pnt;
    pnt.x = x;
    pnt.y = y;
    mset.push_back(pnt);
  }
  cout<<"come out final result"<<endl;
  
  compScore(mset);
}

//compute the area of the covered part in rect by layout rects
static float compCoverArea(FPM::FPMRect &rect, FPM::FPMPattern &pattern)
{
  int LX = pattern.bbox.lb.x;
  int BY = pattern.bbox.lb.y;
  float coverArea = 0;
  for (int i = 0; i < pattern.rect_set.size(); ++ i)
  {
    FPM::FPMRect r1 = pattern.rect_set[i];
    r1.lb.x -= LX;
    r1.lb.y -= BY;
    if (!(r1.lb.x >= rect.lb.x + rect.width || 
        r1.lb.y >= rect.lb.y + rect.height ||
        rect.lb.x >= r1.lb.x + r1.width ||
        rect.lb.y >= r1.lb.y + r1.height))
    {
      FPM::FPMPoint lb, ra;
      lb.x = std::max(r1.lb.x, rect.lb.x);
      lb.y = std::max(r1.lb.y, rect.lb.y);
      ra.x = std::min(r1.lb.x + r1.width, rect.lb.x + rect.width);
      ra.y = std::min(r1.lb.y + r1.height, rect.lb.y + rect.height);
      coverArea += (float)(ra.x-lb.x)*(float)(ra.y-lb.y);
    }
  }
  
  if (coverArea > (float)rect.width*(float)rect.height)
  {
    printf("ERROR in computed cover area %f > %f in %s:%d\n", coverArea, (float)rect.width*(float)rect.height, __FILE__, __LINE__);
  }
  return coverArea;
}

static bool checkCoverArea(FPMPattern &pattern1, FPMPattern &pattern2)
{
  //transform rectangle coordinates of matchLayout
  float totalArea1 = 0, totalArea2 = 0, coverArea = 0;
  for (int i = 0; i < pattern1.rect_set.size(); ++ i)
  {
    totalArea1 += (float)pattern1.rect_set[i].width*(float)pattern1.rect_set[i].height;
  }
  for (int i = 0; i < pattern2.rect_set.size(); ++ i)
  {
    FPM::FPMRect r1 = pattern2.rect_set[i];
    totalArea2 += (float)r1.width*(float)r1.height;
  }
  
  if (totalArea2 == 0 || totalArea1 == 0)
    return false;
  
  //they should have same area to be duplicate
  if (fabs(totalArea1-totalArea2) > 0.01)
    return false;
  
  int LX2 = pattern2.bbox.lb.x;
  int BY2 = pattern2.bbox.lb.y;
  for (int i = 0; i < pattern2.rect_set.size(); ++ i)
  {
    FPM::FPMRect r1 = pattern2.rect_set[i];
    r1.lb.x -= LX2;
    r1.lb.y -= BY2;
    coverArea += compCoverArea(r1, pattern1);
  }
#ifdef DEBUGINFO
//  printf("Covered Area: %f, Total Area1: %f, Total Area2: %f\n", coverArea, totalArea1, totalArea2);
#endif
  
  if (coverArea/totalArea2 >= 0.995 && coverArea/totalArea1 >= 0.995)
    return true;
  
  return false;
}

ostream & operator << (ostream &out, FPMLayout &l)
{
  out << "============= Layout information =============" << endl;
  out << "------------- Rectangles -------------" << endl;
  for (int i = 0; i < (int)l.m_rects.size(); ++ i)
  {
    out << l.m_rects[i];
  }
  out << "------------- Polygons -------------" << endl;
  for (int i = 0; i < (int)l.m_polys.size(); ++ i)
  {
    out << l.m_polys[i];
  }

  return out;
}

bool FPMLayout::checkOverlapOnWindow() {
    FPMRectVector rects;
    for (int i = 0;i < m_polys.size();++i) {
      FPMPattern p;
      FPMPoly_full pf;
      pf.p = m_polys[i];
      p.m_poly_fulls.push_back(pf);
      PTR(p,false);
      rects.insert(rects.begin(),p.rect_set.begin(),p.rect_set.end());
    }
    for (int i = 0;i < m_rects.size();++i) {
      rects.push_back(m_rects[i]);
    }

    bool g = true;
    for (int i = 0;i < rects.size();++i) {
      for (int j = i + 1;j < rects.size();++j) {
	if (checkOverlap(rects[i],rects[j])) {
	  cout << "false!" << endl;
	  cout << rects[i] << endl;
	  cout << rects[j] << endl;
	  g = false;
	}
      }
    }
    return g;
  }


void FPMLayout::createSubLayoutsOnFrame() 
  {
    const int targetLayer = 10;
    
    const int frameLayer1 = 11;
    const int frameLayer2 = 12;

    vector<FPMPoly> target_polys;
    for (int i = 0; i < m_polys.size(); ++ i)
    {
      if (m_polys[i].layer == targetLayer)
      {
	m_polys[i].makeCounterClockwise();
	target_polys.push_back(m_polys[i]);
      }
    }

    vector<FPMRect> core_rects;
    vector<FPMRect> target_rects;
    for (int i = 0; i < m_rects.size(); ++ i)
    {
      if (m_rects[i].layer == frameLayer1 || m_rects[i].layer == frameLayer2)
      {
	core_rects.push_back(m_rects[i]);
      }
      else if (m_rects[i].layer == targetLayer)
      {
	target_rects.push_back(m_rects[i]);
      }
    }

    //cout << "------------- merge -------------" << endl;
    //cout << core_rects.size() << endl;
    for (int i = 0; i < core_rects.size(); ++ i)
    {
      //cout << core_rects[i];
      FPMPattern pt;
      for (int j = 0; j < target_polys.size(); ++ j)
      {
      	mergeRectPolygon(core_rects[i],target_polys[j],pt);
      }

      for (int j = 0; j < target_rects.size(); ++ j)
      {
        	mergeRectRect(core_rects[i],target_rects[j],pt);
      }
      pt.bbox = core_rects[i];	
      m_subLayouts.push_back(pt);
      //cout << pt;
    }    
  }


struct SweepLine
{
	float x;
	int index;
	int type;
};

struct Seg
{
	float y1;
	float y2;
	int index;
  
	bool operator ==(const Seg &s)
	{
		return (y1 == s.y1) && (y2 == s.y2) && (index == s.index);   
	}
};

static bool cmp(SweepLine l1, SweepLine l2)
{
  if (l1.x != l2.x)
    return l1.x < l2.x;
  else
    return l1.type < l2.type; 
}

static FPMRect BBox(FPMPoly &p)
{
	int lx = INT_MAX;
	int by = INT_MAX;
	int rx = INT_MIN;
	int ty = INT_MIN;
	for (int j = 0; j < p.ptlist.size(); ++ j)
	{
    //added points in the polygon should be in counter-clockwise
    if (lx > p.ptlist[j].x)
        lx = p.ptlist[j].x;
    if (by > p.ptlist[j].y)
        by = p.ptlist[j].y;
    if (rx < p.ptlist[j].x)
        rx = p.ptlist[j].x;
    if (ty < p.ptlist[j].y)
        ty = p.ptlist[j].y;
	}
  
  FPMRect r;
  r.lb.x = lx;
  r.lb.y = by;
  r.width = rx-lx;
  r.height = ty-by;
#ifdef DEBUGINFO
  printf("Bounding box of polygon: [%d,%d]-[%d,%d]\n", r.lb.x, r.lb.y, r.lb.x+r.width, r.lb.y+r.height);
  cout << "Polygon: ";
  for (int j = 0; j < p.ptlist.size(); j++)
  {
    int x = p.ptlist[j].x;
    int y = p.ptlist[j].y;
    cout << "[" << x << "," << y << "]";
  }
  cout << endl;
#endif
  
  return r;
}

void FPMLayout::createSubLayoutsOnWindow(int n)
{
  const int targetLayer = 10;
  //bounding box of the chip
  FPMPoint lb,ra;
  ra = lb = m_rects[0].lb;
  ra.x = ra.x + m_rects[0].width;
  ra.y = ra.y + m_rects[0].height;
  for (int i = 1;i < m_rects.size();++i) {
    if (m_rects[i].lb.x < lb.x) {
      lb.x = m_rects[i].lb.x;
    }
    if (m_rects[i].lb.y < lb.y) {
      lb.y = m_rects[i].lb.y;
    }
    if (m_rects[i].lb.x + m_rects[i].width > ra.x) {
      ra.x = m_rects[i].lb.x + m_rects[i].width;
    }
    if (m_rects[i].lb.y + m_rects[i].height > ra.y) {
      ra.y = m_rects[i].lb.y + m_rects[i].height;
    }
  }
  for (int i = 0;i < m_polys.size();++i) {
    for (int j = 0;j < m_polys[i].ptlist.size();++j) {
      if (m_polys[i].ptlist[j].x < lb.x) {
        lb.x = m_polys[i].ptlist[j].x;
      }else if (m_polys[i].ptlist[j].x > ra.x) {
        ra.x = m_polys[i].ptlist[j].x;
      }
      if (m_polys[i].ptlist[j].y < lb.y) {
        lb.y = m_polys[i].ptlist[j].y;
      }else if (m_polys[i].ptlist[j].y > ra.y) {
        ra.y = m_polys[i].ptlist[j].y;
      }
    }
  }
  
  if (n != 0) {
    FPMPattern pattern;
    for (int i = 0;i < m_rects.size();++i) {
      FPMPoly_full pf;
      FPMPoly p;
      for (int j = 0;j < 4;++j) {
        p.ptlist.push_back(m_rects[j].lb);
      }
      p.ptlist[2].x = p.ptlist[1].x = p.ptlist[1].x + m_rects[i].width;
      p.ptlist[3].y = p.ptlist[2].y = p.ptlist[2].y + m_rects[i].height;
      pf.p = p;
      pattern.m_poly_fulls.push_back(pf);
    }
    for (int i = 0;i < m_polys.size();++i) {
    	FPMPoly_full pf;
    	pf.p = m_polys[i];
    	pattern.m_poly_fulls.push_back(pf);
    }
    pattern.bbox.lb = lb;
    pattern.bbox.width = ra.x - lb.x;
    pattern.bbox.height = ra.y - lb.y;
    cout << "--subLayouts---" << endl;
    cout << pattern << endl;;
    m_subLayouts.push_back(pattern);
    return;
  }
  
  //compute the bounding boxes for each polygon and rectangle
  vector<FPMRect> bbs;
  vector<FPMPattern> tempSubLayouts;
  
  const int windowWidth = 4800;
  const int windowHeight = 4800;
  const int coreWidth = 1200;
  const int coreHeight = 1200;
  int xnum = (ra.x - lb.x - coreWidth - 1) / (windowWidth - coreWidth) + 1;
  int ynum = (ra.y - lb.y - coreHeight - 1) / (windowHeight - coreHeight) + 1;
  cout << "xnum = " << xnum << endl;
  cout << "ynum = " << ynum << endl;
  int window_num = xnum*ynum;
  cout << "window num = " << window_num << endl;
  for (int i = 0;i < xnum;++i) {
//    cout << "i = " << i << endl;
    for (int j = 0;j < ynum;++j) {
      FPMRect r;
      r.lb = lb;
      r.lb.x = r.lb.x + i * (windowWidth - coreWidth);
      r.lb.y = r.lb.y + j * (windowHeight - coreHeight);
      r.width = windowWidth;
      r.height = windowHeight;
      bbs.push_back(r);
      
      FPMPattern pt;
      pt.bbox = r;
      tempSubLayouts.push_back(pt);
    }
  }
  vector<int> rectid;
  for (int k = 0;k < m_rects.size();++k) {
    if(m_rects[k].layer==targetLayer)
    {
      bbs.push_back(m_rects[k]);
      rectid.push_back(k);
    }
  }
  vector<int> polyid;
  for (int k = 0;k < m_polys.size();++k) {
    if(m_polys[k].layer==targetLayer)
    {
      bbs.push_back(BBox(m_polys[k]));
      polyid.push_back(k);
    }
  }
  //check overlap
  int bbox_num = (int)bbs.size();
  int rect_num = (int)rectid.size();
  int poly_num = (int)polyid.size();
  vector<SweepLine> Info(2 * bbox_num);
  for (long i = 0; i < bbox_num; i++)
  {
    Info[2 * i].x = (double)bbs[i].lb.x;
    Info[2 * i].index = i;
    Info[2 * i].type = 0;
    Info[2 * i + 1].x = (double)bbs[i].lb.x+bbs[i].width;
    Info[2 * i + 1].index = i;
    Info[2 * i + 1].type = 1;
  }
  sort(Info.begin(), Info.end(), cmp);  

  list<Seg> current;
  for (int line = 0; line != (int)Info.size(); ++line)
  {
    Seg seg_new;
    seg_new.index = Info[line].index;
    seg_new.y1 = (double)bbs[Info[line].index].lb.y;
    seg_new.y2 = (double)bbs[Info[line].index].lb.y+bbs[Info[line].index].height;
    if (Info[line].type)
    {
      current.remove(seg_new);
    }
    else
    {
      list<Seg>::iterator list_it = current.begin();
      while (list_it != current.end())
      {
        if ((seg_new.y1 >= list_it->y1 && seg_new.y1 <= list_it->y2) || 
        (list_it->y1 >= seg_new.y1 && list_it->y1 <= seg_new.y2))
        {
          //the following two rects overlap
          int i = seg_new.index;
          int j = list_it->index;
#ifdef DEBUGINFO
          cout << "SweepLine overlap bbox pair <" << i << "," << j << ">" << endl;
#endif
          if ((i < window_num && j >= window_num) || (j < window_num && i >= window_num))
          {
            if (j < window_num && i >= window_num)
            {
              int temp = i;
              i = j;
              j = temp;
            }
            
            if (j-window_num < rect_num)
            {
              mergeRectRect(bbs[i], m_rects[rectid[j-window_num]], tempSubLayouts[i]);
            }
            else
            {
              if (j-window_num-rect_num >= poly_num)
              {
                cout << "ERROR: bbox index out of bound in " << __FILE__ << ":" << __LINE__ << endl;
              }
              else
              {
                mergeRectPolygon(bbs[i], m_polys[polyid[j-window_num-rect_num]], tempSubLayouts[i]);
              }
            }
          }
        }
        ++list_it;
      }
      current.push_back(seg_new);
    }
  }
  
  m_subLayouts.clear();
  for (int i = 0; i < tempSubLayouts.size(); ++ i)
  {
    if (tempSubLayouts[i].m_poly_fulls.size() + tempSubLayouts[i].rect_set.size() > 10)
    {
//      cout << tempSubLayouts[i];
      m_subLayouts.push_back(tempSubLayouts[i]);
    }
  }
  
  for (int i = 0; i < tempSubLayouts.size(); ++ i)
    tempSubLayouts[i].clearMem();
  tempSubLayouts.clear();
  
  cout << "Total " << m_subLayouts.size() << " sub layouts" << endl;
}

/*
void FPMLayout::createSubLayoutsOnWindow(int n) {
    const int targetLayer = 10;
    FPMPoint lb,ra;
    ra = lb = m_rects[0].lb;
    ra.x = ra.x + m_rects[0].width;
    ra.y = ra.y + m_rects[0].height;
    for (int i = 1;i < m_rects.size();++i) {
      if (m_rects[i].lb.x < lb.x) {
	lb.x = m_rects[i].lb.x;
      }
      if (m_rects[i].lb.y < lb.y) {
	lb.y = m_rects[i].lb.y;
      }
      if (m_rects[i].lb.x + m_rects[i].width > ra.x) {
	ra.x = m_rects[i].lb.x + m_rects[i].width;
      }
      if (m_rects[i].lb.y + m_rects[i].height > ra.y) {
	ra.y = m_rects[i].lb.y + m_rects[i].height;
      }
    }
    for (int i = 0;i < m_polys.size();++i) {
      for (int j = 0;j < m_polys[i].ptlist.size();++j) {
	if (m_polys[i].ptlist[j].x < lb.x) {
	  lb.x = m_polys[i].ptlist[j].x;
	}else if (m_polys[i].ptlist[j].x > ra.x) {
	  ra.x = m_polys[i].ptlist[j].x;
	}
	if (m_polys[i].ptlist[j].y < lb.y) {
	  lb.y = m_polys[i].ptlist[j].y;
	}else if (m_polys[i].ptlist[j].y > ra.y) {
	  ra.y = m_polys[i].ptlist[j].y;
	}
      }
    }

    if (n != 0) {
      FPMPattern pattern;
      for (int i = 0;i < m_rects.size();++i) {
	FPMPoly_full pf;
	FPMPoly p;
	for (int j = 0;j < 4;++j) {
	  p.ptlist.push_back(m_rects[j].lb);
	}
	p.ptlist[2].x = p.ptlist[1].x = p.ptlist[1].x + m_rects[i].width;
	p.ptlist[3].y = p.ptlist[2].y = p.ptlist[2].y + m_rects[i].height;
	pf.p = p;
	pattern.m_poly_fulls.push_back(pf);
      }
      for (int i = 0;i < m_polys.size();++i) {
	FPMPoly_full pf;
	pf.p = m_polys[i];
	pattern.m_poly_fulls.push_back(pf);
      }
      pattern.bbox.lb = lb;
      pattern.bbox.width = ra.x - lb.x;
      pattern.bbox.height = ra.y - lb.y;
      cout << "--subLayouts---" << endl;
      cout << pattern << endl;;
      m_subLayouts.push_back(pattern);
      return;
    }

   const int windowWidth = 4800;
   const int windowHeight = 4800;
    //const int windowWidth = 2400;
    //const int windowHeight = 2400;
    const int coreWidth = 1200;
    const int coreHeight = 1200;
    int xnum = (ra.x - lb.x - coreWidth - 1) / (windowWidth - coreWidth) + 1;
    int ynum = (ra.y - lb.y - coreHeight - 1) / (windowHeight - coreHeight) + 1;
    cout << "xnum = " << xnum << endl;
    cout << "ynum = " << ynum << endl;
    for (int i = 0;i < xnum;++i) {
      cout << "i = " << i << endl;
      if(i==1)return;
      for (int j = 0;j < ynum;++j) {
	      FPMRect r;
	      r.lb = lb;
	      r.lb.x = r.lb.x + i * (windowWidth - coreWidth);
	      r.lb.y = r.lb.y + j * (windowHeight - coreHeight);
	      r.width = windowWidth;
	      r.height = windowHeight;
	      FPMPattern pt;
	      pt.bbox = r;
	      for (int k = 0;k < m_rects.size();++k) {
                if(m_rects[k].layer==targetLayer)
                {
	            mergeRectRect(r,m_rects[k],pt);
                }
	      }
	      for (int k = 0;k < m_polys.size();++k) {
                if(m_polys[k].layer==targetLayer)
                {
	             mergeRectPolygon(r,m_polys[k],pt);
                }
        }
	      m_subLayouts.push_back(pt);
      }
    }
  }
*/

void FPMLayout::storeCoreRects()
{
  const int frameLayer1 = 21;
  const int frameLayer2 = 22;

  for (int i = 0; i < m_rects.size(); ++ i)
  {
    if (m_rects[i].layer == frameLayer1 || m_rects[i].layer == frameLayer2)
    {
      m_coreRects.push_back(m_rects[i]);
    }
  }
}

void FPMLayout::createSubLayouts(bool flag) {
    if (flag) {
      createSubLayoutsOnWindow(0);
    }
    else {
      createSubLayoutsOnFrame();
    }
    
  	storeCoreRects();

    return;
    
    clear();
    subLayouts_inisize = m_subLayouts.size();
/////////////////STD//////////////////////
    const int cutNum = 90;
    const int dropNum = 90;
//////////////////////////////////////////
    const int coreWidth = 1200;
    const int coreHeight = 1200;
    int num = m_subLayouts.size();
    for (int i = 0;i < num;++i) {
    	PTR(m_subLayouts[i],false);
    	if (m_subLayouts[i].rect_set.size() >= cutNum) {
    		for (int j = 0;j < 3;++j) {
    			for (int k = 0;k < 3;++k) {
    				FPMRect bbox;
    				bbox.width = coreWidth * 2;
    				bbox.height = coreHeight * 2;
    				bbox.lb.x = m_subLayouts[i].bbox.lb.x + j * coreWidth;
    				bbox.lb.y = m_subLayouts[i].bbox.lb.y + k * coreHeight;
    				FPMPattern pt;
    				pt.bbox = bbox;
    				int id = 0;
    				for (int t = 0;t < m_subLayouts[i].rect_set.size();++t) {
    					mergeRectRectToRect(bbox,m_subLayouts[i].rect_set[t],pt,id);
    				}
    				if (pt.rect_set.size() >= dropNum) {
    				  continue;
    				}
    				m_subLayouts.push_back(pt);
    			}
    		}
    		m_subLayouts.erase(m_subLayouts.begin() + i);
    		--i;
    		--num;
    	}
    }
    cout << "initial number of sublayouts = " << subLayouts_inisize << endl;
    cout << "cutted subLayouts start from " << num << endl;
  }

/*
//there are rectangles on layer 10 which remain to be consideration
void FPMLayout::createPatterns() 
{
	const int targetLayer = 10;
	const int coreLayer1 = 21;
	const int coreLayer2 = 22;
	
	vector<FPMPoly> target_polys;
	for (int i = 0; i < m_polys.size(); ++ i)
	{
		if (m_polys[i].layer == targetLayer)
		{
			m_polys[i].makeCounterClockwise();
			target_polys.push_back(m_polys[i]);
		}
	}
	
	vector<FPMRect> core_rects;
	vector<FPMRect> target_rects;
	for (int i = 0; i < m_rects.size(); ++ i)
	{
		if (m_rects[i].layer == coreLayer1 || m_rects[i].layer == coreLayer2)
		{
			core_rects.push_back(m_rects[i]);
		}
		else if (m_rects[i].layer == targetLayer)
		{
			target_rects.push_back(m_rects[i]);
		}
	}
	
	//cout << "------------- merge -------------" << endl;
	//cout << core_rects.size() << endl;
	for (int i = 0; i < core_rects.size(); ++ i)
	{
		//cout << core_rects[i];
		FPMPattern pt;
		for (int j = 0; j < target_polys.size(); ++ j)
		{
		  mergeRectPolygon(core_rects[i],target_polys[j],pt);
		}
		
		for (int j = 0; j < target_rects.size(); ++ j)
		{
			mergeRectRect(core_rects[i],target_rects[j],pt);
		}
	        pt.bbox = core_rects[i];	
		m_patterns.push_back(pt);
		//cout << pt;
	}
	
}
*/

//there are rectangles on layer 10 which remain to be consideration
void FPMLayout::createPatterns() 
{
	const int targetLayer = 10;
	const int coreLayer1 = 21;
	const int coreLayer2 = 22;
	
	vector<FPMPoly> target_polys;
	for (int i = 0; i < m_polys.size(); ++ i)
	{
		if (m_polys[i].layer == targetLayer)
		{
			m_polys[i].makeCounterClockwise();
			target_polys.push_back(m_polys[i]);
		}
	}
	
	vector<FPMRect> core_rects;
	vector<FPMRect> target_rects;
	for (int i = 0; i < m_rects.size(); ++ i)
	{
		if (m_rects[i].layer == coreLayer1 || m_rects[i].layer == coreLayer2)
		{
			core_rects.push_back(m_rects[i]);
		}
		else if (m_rects[i].layer == targetLayer)
		{
			target_rects.push_back(m_rects[i]);
		}
	}
	
	//cout << "------------- merge -------------" << endl;
	//cout << core_rects.size() << endl;
	for (int i = 0; i < core_rects.size(); ++ i)
	{
		//cout << core_rects[i];
		FPMPattern pt;
		for (int j = 0; j < target_polys.size(); ++ j)
		{
		  mergeRectPolygon(core_rects[i],target_polys[j],pt);
		}
		
		for (int j = 0; j < target_rects.size(); ++ j)
		{
			mergeRectRect(core_rects[i],target_rects[j],pt);
		}
	        pt.bbox = core_rects[i];	
		m_patterns.push_back(pt);
		//cout << pt;
	}
	
}
void FPMLayout::createGoodPatterns() 
{
	const int targetLayer = 10;
	const int coreLayer1 = 23;
	
	
	vector<FPMPoly> target_polys;
	for (int i = 0; i < m_polys.size(); ++ i)
	{
		if (m_polys[i].layer == targetLayer)
		{
			m_polys[i].makeCounterClockwise();
			target_polys.push_back(m_polys[i]);
		}
	}
	
	vector<FPMRect> core_rects;
	vector<FPMRect> target_rects;
	for (int i = 0; i < m_rects.size(); ++ i)
	{
		if (m_rects[i].layer == coreLayer1)
		{
			core_rects.push_back(m_rects[i]);
		}
		else if (m_rects[i].layer == targetLayer)
		{
			target_rects.push_back(m_rects[i]);
		}
	}
	
	//cout << "------------- merge -------------" << endl;
	//cout << core_rects.size() << endl;
	for (int i = 0; i < core_rects.size(); ++ i)
	{
		//cout << core_rects[i];
		FPMPattern pt;
		for (int j = 0; j < target_polys.size(); ++ j)
		{
		  mergeRectPolygon(core_rects[i],target_polys[j],pt);
		}
		
		for (int j = 0; j < target_rects.size(); ++ j)
		{
			mergeRectRect(core_rects[i],target_rects[j],pt);
		}
	        pt.bbox = core_rects[i];	
		m_patterns.push_back(pt);
		//cout << pt;
	}
	
}
void FPMLayout::mergeRectPolygon(FPMRect& r,FPMPoly& p,FPMPattern& pt)
{
	assert(!p.isClockwise());
	
	FPMPoint lb;
	FPMPoint ra;
	lb = ra = p.ptlist[0];
	for (int i = 1;i < p.ptlist.size(); ++ i)
	{
		if (p.ptlist[i].x < lb.x) 
		{
			lb.x = p.ptlist[i].x;
		}
		else if (p.ptlist[i].x > ra.x)
		{
			ra.x = p.ptlist[i].x;
		}
		
		if (p.ptlist[i].y < lb.y)
		{
			lb.y = p.ptlist[i].y;
		}
		else if (p.ptlist[i].y > ra.y)
		{
			ra.y = p.ptlist[i].y;
		}
	}
	
	if (lb.x >= r.lb.x + r.width ||
  	  lb.y >= r.lb.y + r.height ||
  	  r.lb.x >= ra.x ||
  	  r.lb.y >= ra.y)
  {
  	return;
  }
  
  if (lb.x >= r.lb.x &&
  	  lb.y >= r.lb.y &&
  	  ra.x <= r.lb.x + r.width &&
  	  ra.y <= r.lb.y + r.height)
  {
  	FPMPoly_full pf;
  	pf.full = true;
  	pf.p = p;
  	pt.m_poly_fulls.push_back(pf);
  	return;
  }
  
	KBool::Kbool4Router kbr;
  KBool::Bool_Engine booleng;
	
	kbr.ArmBoolEng(&booleng);

  if (booleng.StartPolygonAdd(KBool::GROUP_A))
  {
  	//added points in the polygon should be in counter-clockwise
		booleng.AddPoint(r.lb.x,r.lb.y);
		booleng.AddPoint(r.lb.x + r.width,r.lb.y); 
		booleng.AddPoint(r.lb.x + r.width,r.lb.y + r.height);
		booleng.AddPoint(r.lb.x,r.lb.y + r.height);
  }
  booleng.EndPolygonAdd();

  if (booleng.StartPolygonAdd(KBool::GROUP_B))
	{
		for (int j = 0; j < p.ptlist.size(); ++ j)
		{
      //added points in the polygon should be in counter-clockwise
			booleng.AddPoint(p.ptlist[j].x,p.ptlist[j].y);
		}
	}
	booleng.EndPolygonAdd();
		
	booleng.Do_Operation(KBool::BOOL_AND);
    
  // extract result
  while (booleng.StartPolygonGet())
  {
  	//Points in the polygon are in clockwise order, and intermediate points are not removed
    FPMPoly polygon;
    while(booleng.PolygonHasMorePoints())
    {
    	FPMPoint point;
    	point.x = (int)booleng.GetPolygonXPoint();
    	point.y = (int)booleng.GetPolygonYPoint();
    	polygon.ptlist.push_back(point);
    }
    
    polygon.layer = p.layer;
    polygon.makeCounterClockwise();
    
    FPMPoly_full pf;
    pf.full = false;
    pf.p = polygon;
    pt.m_poly_fulls.push_back(pf);
    booleng.EndPolygonGet();
  }
  
  return;
}

void FPMLayout::mergeRectRectToRect(FPMRect& r1,FPMRect& r2,FPMPattern& pt,int& id) {
  	if (r1.lb.x >= r2.lb.x + r2.width ||
	      r1.lb.y >= r2.lb.y + r2.height ||
	      r2.lb.x >= r1.lb.x + r1.width ||
  	    r2.lb.y >= r1.lb.y + r1.height)
    {
      return;
    }

    FPMPoint lb,ra;
    lb.x = maxint(r1.lb.x,r2.lb.x);
    lb.y = maxint(r1.lb.y,r2.lb.y);
    ra.x = minint(r1.lb.x + r1.width,r2.lb.x + r2.width);
    ra.y = minint(r1.lb.y + r1.height,r2.lb.y + r2.height);

		FPMRect r;
		r.lb.x = lb.x;
		r.lb.y = lb.y;
		r.width = ra.x - lb.x;
		r.height = ra.y - lb.y;
		
		for (int i = 0;i < 4;++i) {
			r.touchArray[i] = 0;
		}
		if (lb.x == r1.lb.x) {
			r.touchArray[0] = 1;
		}
		if (lb.y = r1.lb.y) {
			r.touchArray[1] = 1;
		}
		if (ra.x = r1.lb.x + r1.width) {
			r.touchArray[2] = 1;
		}
		if (ra.y = r1.lb.y + r1.height) {
			r.touchArray[3] = 1;
		}
		r.rect_id = id;
		++id;
		pt.rect_set.push_back(r);
		
		return;
  }

bool FPMLayout::checkOverlap(FPMRect& r1,FPMRect& r2) {
    if (r1.lb.x >= r2.lb.x + r2.width ||
	r1.lb.y >= r2.lb.y + r2.height ||
	r2.lb.x >= r1.lb.x + r1.width ||
	r2.lb.y >= r1.lb.y + r1.height)
    {
      return false;
    }
    return true;
  }

void FPMLayout::mergeRectRect(FPMRect& r1,FPMRect& r2,FPMPattern& pt)
{
  if (r1.lb.x >= r2.lb.x + r2.width ||
  		r1.lb.y >= r2.lb.y + r2.height ||
  		r2.lb.x >= r1.lb.x + r1.width ||
  		r2.lb.y >= r1.lb.y + r1.height)
  {
  	return;
  }
  
  FPMPoint lb,rb,ra,la;
  la.x = lb.x = maxint(r1.lb.x,r2.lb.x);
  lb.y = rb.y = maxint(r1.lb.y,r2.lb.y);
  ra.x = rb.x = minint(r1.lb.x + r1.width,r2.lb.x + r2.width);
  la.y = ra.y = minint(r1.lb.y + r1.height,r2.lb.y + r2.height);
  
  FPMPoly_full pf;
  pf.p.layer = r2.layer;
  pf.p.ptlist.push_back(lb);
  pf.p.ptlist.push_back(rb);
  pf.p.ptlist.push_back(ra);
  pf.p.ptlist.push_back(la);
  
  if (ra.x - la.x == r2.width && la.y - lb.y == r2.height)
  {
    pf.full = true;
  }
  else
  {
  	pf.full = false;
  }
  pt.m_poly_fulls.push_back(pf);
  
  return;
}


void FPMLayout::patternRotation(FPMPattern &pt, vector<FPMPattern> &rotatePatterns)
{
   int sin,cos,x0=0,y0=0,x,y;
   for(int i=1;i<=3;i++)
   {
     FPMPattern pattern;
     for(int j=0;j<pt.m_poly_fulls.size();j++)
     {
         FPMPoly_full poly;
         for(int k=0;k<pt.m_poly_fulls[j].p.ptlist.size();k++)
         {
             if(i==1) { sin=1; cos=0; }
             else if (i==2) { sin=0; cos=-1; }
             else { sin=-1; cos=0; }
             x=(pt.m_poly_fulls[j].p.ptlist[k].x-x0)*cos+(pt.m_poly_fulls[j].p.ptlist[k].y-y0)*sin+x0;
             y=(pt.m_poly_fulls[j].p.ptlist[k].y-y0)*cos-(pt.m_poly_fulls[j].p.ptlist[k].x-x0)*sin+y0;
             FPMPoint point;
             point.x=x;
             point.y=y;
             //cout<<"rotation position("<<x<<","<<y<<")"<<endl;
             poly.p.ptlist.push_back(point);       
         }
         //cout<<"****poly in rotation****"<<endl;
         //cout<<poly.p;
         //cout<<"*************"<<endl;
         pattern.m_poly_fulls.push_back(poly);
     }
     pattern.bbox = pt.bbox;
     int lx = pattern.bbox.lb.x;
     int by = pattern.bbox.lb.y;
     int rx = pattern.bbox.lb.x+pattern.bbox.width;
     int ty = pattern.bbox.lb.y+pattern.bbox.height;
     FPMPoint pnt1, pnt2, pnt3, pnt4;
//     pnt1.x = lx;
//     pnt1.y = by;
//     pnt2.x = lx;
//     pnt2.y = ty;
//     pnt3.x = rx;
//     pnt3.y = by;
//     pnt4.x = rx;
//     pnt4.y = ty;
     pnt1.x=(lx-x0)*cos+(by-y0)*sin+x0;
     pnt1.y=(by-y0)*cos-(lx-x0)*sin+y0;
     pnt2.x=(lx-x0)*cos+(ty-y0)*sin+x0;
     pnt2.y=(ty-y0)*cos-(lx-x0)*sin+y0;
     pnt3.x=(rx-x0)*cos+(by-y0)*sin+x0;
     pnt3.y=(by-y0)*cos-(rx-x0)*sin+y0;
     pnt4.x=(rx-x0)*cos+(ty-y0)*sin+x0;
     pnt4.y=(ty-y0)*cos-(rx-x0)*sin+y0;
     //TODO: only good when width == height
     pattern.bbox.lb.x = std::min(pnt1.x, std::min(pnt2.x, std::min(pnt3.x, pnt4.x)));
     pattern.bbox.lb.y = std::min(pnt1.y, std::min(pnt2.y, std::min(pnt3.y, pnt4.y)));
     
//     cout << pattern << endl;     
     for (int i=0;i<pattern.m_poly_fulls.size();i++)
     {
        FPMPoly_full poly;
        for(int j=0;j<pattern.m_poly_fulls[i].p.ptlist.size()-1;j++)
        {
           FPMPoint point, point1;
           point.x = pattern.m_poly_fulls[i].p.ptlist[j].x;
           point.y = pattern.m_poly_fulls[i].p.ptlist[j].y;
           point1.x = pattern.m_poly_fulls[i].p.ptlist[j+1].x;
           point1.y = pattern.m_poly_fulls[i].p.ptlist[j+1].y;
           if (point.x != point1.x && point.y != point1.y)
           {
                   printf("%s:%d\n", __FILE__, __LINE__);
	           printf("[%d,%d]\n", point.x, point.y);
	           printf("[%d,%d]\n", point1.x, point1.y);
                   exit(-1);
           }
//           assert(point.x >= pattern.bbox.lb.x && point.x <= pattern.bbox.lb.x+pattern.bbox.width);
//           assert(point.y >= pattern.bbox.lb.y && point.y <= pattern.bbox.lb.y+pattern.bbox.height);
        }
     }

     rotatePatterns.push_back(pattern);
   }
}

void FPMLayout::patternSymmetry(FPMPattern &pt, vector<FPMPattern> &symPatterns)
{
   //x=0 is the symmetry line
   int x,y;
   FPMPattern pattern;
   for(int i=0;i<pt.m_poly_fulls.size();i++)
   {
      FPMPoly_full poly;
      for(int j=0;j<pt.m_poly_fulls[i].p.ptlist.size();j++)
      {
         FPMPoint point;
         x = -(pt.m_poly_fulls[i].p.ptlist[j].x);
         point.x = x;
         point.y = pt.m_poly_fulls[i].p.ptlist[j].y;
         poly.p.ptlist.push_back(point);
      }
      pattern.m_poly_fulls.push_back(poly);
   }
   pattern.bbox = pt.bbox;
   int rx = pattern.bbox.lb.x+pattern.bbox.width;
   pattern.bbox.lb.x = -rx;
   
     for (int i=0;i<pattern.m_poly_fulls.size();i++)
     {
        FPMPoly_full poly;
        for(int j=0;j<pattern.m_poly_fulls[i].p.ptlist.size()-1;j++)
        {
           FPMPoint point, point1;
           point.x = pattern.m_poly_fulls[i].p.ptlist[j].x;
           point.y = pattern.m_poly_fulls[i].p.ptlist[j].y;
           point1.x = pattern.m_poly_fulls[i].p.ptlist[j+1].x;
           point1.y = pattern.m_poly_fulls[i].p.ptlist[j+1].y;
           if (point.x != point1.x && point.y != point1.y)
           {
                   printf("%s:%d\n", __FILE__, __LINE__);
	           printf("[%d,%d]\n", point.x, point.y);
	           printf("[%d,%d]\n", point1.x, point1.y);
                   exit(-1);
           }
//           assert(point.x >= pattern.bbox.lb.x && point.x <= pattern.bbox.lb.x+pattern.bbox.width);
//           assert(point.y >= pattern.bbox.lb.y && point.y <= pattern.bbox.lb.y+pattern.bbox.height);
        }
     }

//   cout << pattern << endl;
//   for(int i=0;i<pattern.m_poly_fulls.size();i++)
//   {
//      FPMPoly_full poly;
//      for(int j=0;j<pattern.m_poly_fulls[i].p.ptlist.size();j++)
//      {
//         FPMPoint point;
//         point.x = pattern.m_poly_fulls[i].p.ptlist[j].x;
//         point.y = pattern.m_poly_fulls[i].p.ptlist[j].y;
//         assert(point.x >= pattern.bbox.lb.x && point.x <= pattern.bbox.lb.x+pattern.bbox.width);
//         assert(point.y >= pattern.bbox.lb.y && point.y <= pattern.bbox.lb.y+pattern.bbox.height);
//      }
//   }
   
   symPatterns.push_back(pattern);
}

void FPMLayout::rotateAllPatterns(vector<FPMPattern> &allPatterns)
{
  vector<FPMPattern> rotatePatterns;
  for(int i = 0; i < allPatterns.size(); i++)
  {
    patternRotation(allPatterns[i], rotatePatterns);
  }
  vector<FPMPattern> symPatterns;
  for(int i = 0; i < rotatePatterns.size(); i++)
  {
    patternSymmetry(rotatePatterns[i], symPatterns);
  }
  for(int i = 0; i < allPatterns.size(); i++)
  {
    patternSymmetry(allPatterns[i], symPatterns);
  }
  allPatterns.insert(allPatterns.end(), rotatePatterns.begin(), rotatePatterns.end());
  allPatterns.insert(allPatterns.end(), symPatterns.begin(), symPatterns.end());
}

void FPMLayout::drawPattern(FPMPattern pattern,char* file)
{
     Plot plot;
     GNUPolygon *poly=NULL;
     GNUPointVector ptlist;
     char* filename=file;
     string colorStr="#0000FF";
     for(int i=0;i<pattern.m_poly_fulls.size();i++)
     {
        poly=new GNUPolygon(1);
        for(int j=0;j<pattern.m_poly_fulls[i].p.ptlist.size();j++)
        {
            GNUPoint* gp=new GNUPoint(j,pattern.m_poly_fulls[i].p.ptlist[j].x,pattern.m_poly_fulls[i].p.ptlist[j].y);
            //ptlist.push_back(gp);
            poly->addPoint(gp);
        }
        plot.addPolygon(poly);
     }
     plot.draw(filename);
}

void  FPMLayout::drawArea(const FPMPattern &pattern,int min_x,int min_y,int max_x,int max_y)
{
    Plot plot;
    GNULine *line=NULL;
    string colorBox = "#00FF00";
    int lx1=pattern.bbox.lb.x;
    int ly1=pattern.bbox.lb.y;
    int lx2=lx1+pattern.bbox.width;
    int ly2=ly1+pattern.bbox.height;
    GNURectangle *gnu_rect=new GNURectangle(pattern.rect_set.size(),lx1,ly1,lx2,ly2,colorBox);
    plot.addRect(gnu_rect);
    for(int i=0;i<pattern.m_edge.size();i++)
    {
      if(pattern.m_edge[i].type==0)//vertical
      {
                line = new GNULine(pattern.m_edge[i].edge_id,pattern.m_edge[i].point.x,pattern.m_edge[i].point.y, pattern.m_edge[i].point.x,pattern.m_edge[i].point.y+pattern.m_edge[i].length, 2);
		plot.addLine(line);
      }
      else//horizontal
      {
                line = new GNULine(pattern.m_edge[i].edge_id,pattern.m_edge[i].point.x,pattern.m_edge[i].point.y, pattern.m_edge[i].point.x+pattern.m_edge[i].length,pattern.m_edge[i].point.y, 2);
		plot.addLine(line);
      }
    }
    line =new GNULine(1,min_x,min_y,min_x,max_y,2);
    plot.addLine(line);
    line =new GNULine(2,min_x,min_y,max_x,min_y,2);
    plot.addLine(line);
    line =new GNULine(3,max_x,min_y,max_x,max_y,2);
    plot.addLine(line);
    line =new GNULine(4,min_x,max_y,max_x,max_y,2);
    plot.addLine(line);
    plot.draw("pattern");
}

void FPMLayout::drawEdge(FPMEdgeVector edge_vector,FPMRect bbox)
{
   Plot plot;
   GNULine *line=NULL;
   string colorBox = "#00FF00";	
	 int lx1=bbox.lb.x;
   int ly1=bbox.lb.y;
   int lx2=lx1+bbox.width;
   int ly2=ly1+bbox.height;
   GNURectangle *gnu_rect=new GNURectangle(edge_vector.size(),lx1,ly1,lx2,ly2,colorBox);
   plot.addRect(gnu_rect);
   for(int i=0;i<edge_vector.size();i++)
   {
      if(edge_vector[i].type==0)line = new GNULine(edge_vector[i].edge_id,edge_vector[i].point.x,edge_vector[i].point.y,edge_vector[i].point.x,edge_vector[i].point.y+edge_vector[i].length, 2);
		  else line = new GNULine(edge_vector[i].edge_id,edge_vector[i].point.x,edge_vector[i].point.y,edge_vector[i].point.x+edge_vector[i].length,edge_vector[i].point.y, 2);
      plot.addLine(line);
   }
   plot.draw("matchBlock");
}
void FPMLayout::drawBox(FPMPattern pattern,int x,int y)
{
    Plot plot;
    GNULine *line=NULL;
    string colorBox = "#00FF00";
    int lx1=pattern.bbox.lb.x;
    int ly1=pattern.bbox.lb.y;
    int lx2=lx1+pattern.bbox.width;
    int ly2=ly1+pattern.bbox.height;
    GNURectangle *gnu_rect=new GNURectangle(pattern.rect_set.size(),lx1,ly1,lx2,ly2,colorBox);
    plot.addRect(gnu_rect);
    //cout<<"pattern.m_edge.size(): "<<pattern.m_edge.size()<<endl;
    //getchar();
    for(int i=0;i<pattern.m_edge.size();i++)
    {
      //cout<<"i------------>"<<i<<endl;
      if(pattern.m_edge[i].type==0)//vertical
      {
                //cout<<"draw vertical line"<<endl;
                //cout<<pattern.m_edge[i].point.x<<endl;
                //cout<<pattern.m_edge[i].point.y<<endl;
                //cout<<pattern.m_edge[i].point.x<<endl;
                //cout<<pattern.m_edge[i].point.y+pattern.m_edge[i].length<<endl;
                line = new GNULine(pattern.m_edge[i].edge_id,pattern.m_edge[i].point.x,pattern.m_edge[i].point.y, pattern.m_edge[i].point.x,pattern.m_edge[i].point.y+pattern.m_edge[i].length, 2);
		plot.addLine(line);
      }
      else//horizontal
      {
                //cout<<"draw horizontal line"<<endl;
                //cout<<pattern.m_edge[i].point.x<<endl;
                //cout<<pattern.m_edge[i].point.y<<endl;
                //cout<<pattern.m_edge[i].point.x+pattern.m_edge[i].length<<endl;
                //cout<<pattern.m_edge[i].point.y<<endl;
                line = new GNULine(pattern.m_edge[i].edge_id,pattern.m_edge[i].point.x,pattern.m_edge[i].point.y, pattern.m_edge[i].point.x+pattern.m_edge[i].length,pattern.m_edge[i].point.y, 2);
		plot.addLine(line);
      }
    }
    //plot.draw(filename);
    //drawLines(pattern,"subLayout");
    //Plot plot;
    //GNULine *line=NULL;
    //string lineColor="#0000FF";
    int x1,y1,x2,y2,x3,y3,x4,y4;
    x1=x-600;
    y1=y-600;
    x2=x+600;
    y2=y1;
    x3=x2;
    y3=y+600;
    x4=x1;
    y4=y3;
    line =new GNULine(1,x1,y1,x2,y2,2);
    plot.addLine(line);
    line =new GNULine(2,x2,y2,x3,y3,2);
    plot.addLine(line);
    line =new GNULine(3,x3,y3,x4,y4,2);
    plot.addLine(line);
    line =new GNULine(4,x4,y4,x1,y1,2);
    plot.addLine(line);
    plot.draw("subLayout");
}

void FPMLayout::drawBlock(const FPMEdgeVector &edge_vector,const FPMPattern &pattern)
{
    Plot plot;
    GNULine *line=NULL;
    string colorBox = "#00FF00";
    int lx1=pattern.bbox.lb.x;
    int ly1=pattern.bbox.lb.y;
    int lx2=lx1+pattern.bbox.width;
    int ly2=ly1+pattern.bbox.height;
    GNURectangle *gnu_rect=new GNURectangle(pattern.rect_set.size(),lx1,ly1,lx2,ly2,colorBox);
    plot.addRect(gnu_rect);
    for(int i=0;i<edge_vector.size();i++)
    {
         if(edge_vector[i].type==0)
         {
            line = new GNULine(edge_vector[i].edge_id,edge_vector[i].point.x,edge_vector[i].point.y, edge_vector[i].point.x,edge_vector[i].point.y+edge_vector[i].length, 2);
		plot.addLine(line);
         }
         else
         {
            line = new GNULine(edge_vector[i].edge_id,edge_vector[i].point.x,edge_vector[i].point.y, edge_vector[i].point.x+edge_vector[i].length,edge_vector[i].point.y, 2);
		plot.addLine(line);
         }

    }
    plot.draw("block");
}

void FPMLayout::drawLines(FPMPattern pattern,char* filename)
{
    Plot plot;
    GNULine *line=NULL;
    string colorBox = "#00FF00";
    int lx1=pattern.bbox.lb.x;
    int ly1=pattern.bbox.lb.y;
    int lx2=lx1+pattern.bbox.width;
    int ly2=ly1+pattern.bbox.height;
    GNURectangle *gnu_rect=new GNURectangle(pattern.rect_set.size(),lx1,ly1,lx2,ly2,colorBox);
    plot.addRect(gnu_rect);
    //cout<<"pattern.m_edge.size(): "<<pattern.m_edge.size()<<endl;
    //getchar();
    for(int i=0;i<pattern.m_edge.size();i++)
    {
      //cout<<"i------------>"<<i<<endl;
      if(pattern.m_edge[i].type==0)//vertical
      {
                //cout<<"draw vertical line"<<endl;
                //cout<<pattern.m_edge[i].point.x<<endl;
                //cout<<pattern.m_edge[i].point.y<<endl;
                //cout<<pattern.m_edge[i].point.x<<endl;
                //cout<<pattern.m_edge[i].point.y+pattern.m_edge[i].length<<endl;
                line = new GNULine(pattern.m_edge[i].edge_id,pattern.m_edge[i].point.x,pattern.m_edge[i].point.y, pattern.m_edge[i].point.x,pattern.m_edge[i].point.y+pattern.m_edge[i].length, 2);
		plot.addLine(line);
      }
      else//horizontal
      {
                //cout<<"draw horizontal line"<<endl;
                //cout<<pattern.m_edge[i].point.x<<endl;
                //cout<<pattern.m_edge[i].point.y<<endl;
                //cout<<pattern.m_edge[i].point.x+pattern.m_edge[i].length<<endl;
                //cout<<pattern.m_edge[i].point.y<<endl;
                line = new GNULine(pattern.m_edge[i].edge_id,pattern.m_edge[i].point.x,pattern.m_edge[i].point.y, pattern.m_edge[i].point.x+pattern.m_edge[i].length,pattern.m_edge[i].point.y, 2);
		plot.addLine(line);
      }
    }
    plot.draw(filename);
}
void FPMLayout::drawPatterns()
{
     Plot plot;
     GNUPolygon *poly=NULL;
     GNUPointVector ptlist;
     char* filename="plotresult.txt";
     string colorStr="#0000FF";
     cout<<"filename----->"<<filename<<endl;
     //getchar();   
     for(int n=0;n<m_patterns.size();n++)
     {
        for(int i=0;i<m_patterns[n].m_poly_fulls.size();i++)
        {
            cout<<m_patterns[n];
            for(int j=0;j<m_patterns[n].m_poly_fulls[i].p.ptlist.size();j++)
            {
               GNUPoint* gp=new GNUPoint(j,m_patterns[n].m_poly_fulls[i].p.ptlist[j].x,m_patterns[n].m_poly_fulls[i].p.ptlist[j].y);
               ptlist.push_back(gp);
            }
            //Plot plot;
            poly=new GNUPolygon(1);
            cout<<"The "<<i<<" poly's point number is "<<ptlist.size()<<endl;
            for(int k=0;k<ptlist.size();k++)
            {
               cout<<"add point to "<<i<<" polygon"<<" ("<<ptlist[k]->x()<<","<<ptlist[k]->y()<<")"<<endl;
               poly->addPoint(ptlist[k]);     
            }
            plot.addPolygon(poly);
            ptlist.clear();
        }
        plot.draw(filename);
        getchar();
      }
}

void FPMLayout:: drawRectVector(FPMRectVector rect_vector)
{
    Plot plot;
    string colorStr = "#00FFFF";
    char* fileName="rect_vector_result";
    GNURectangle *gnu_rect=NULL;
    int lx1,ly1,lx2,ly2;
    for(int i=0;i<rect_vector.size();i++)
    {
        lx1=rect_vector[i].lb.x;
        ly1=rect_vector[i].lb.y;
        lx2=lx1+rect_vector[i].width;
        ly2=ly1+rect_vector[i].height;
        gnu_rect=new GNURectangle(i,lx1,ly1,lx2,ly2,colorStr);
        plot.addRect(gnu_rect);
    }
    plot.draw(fileName);
}

void FPMLayout:: drawRects(FPMPattern pattern,char* filename)
{
    //cout<<pattern;
    Plot plot;
    string colorStr = "#0000FF";
    char* fileName=filename;
    GNURectangle *gnu_rect=NULL;
    int lx1,ly1,lx2,ly2;
    //draw bounding box
    string colorBox = "#00FF00";
    lx1=pattern.bbox.lb.x;
    ly1=pattern.bbox.lb.y;
    lx2=lx1+pattern.bbox.width;
    ly2=ly1+pattern.bbox.height;
    gnu_rect=new GNURectangle(pattern.rect_set.size(),lx1,ly1,lx2,ly2,colorBox);
    plot.addRect(gnu_rect);

    for(int i=0;i<pattern.rect_set.size();i++)
    {
        lx1=pattern.rect_set[i].lb.x;
        ly1=pattern.rect_set[i].lb.y;
        lx2=lx1+pattern.rect_set[i].width;
        ly2=ly1+pattern.rect_set[i].height;
        gnu_rect=new GNURectangle(i,lx1,ly1,lx2,ly2,colorStr);
        plot.addRect(gnu_rect);
    }
    plot.draw(fileName);
}

void FPMLayout::createSubLayouts() 
{
	const int targetLayer = 10;
//.....................
	const int coreLayer1 = 11;
	const int coreLayer2 = 12;
	
	vector<FPMPoly> target_polys;
	for (int i = 0; i < m_polys.size(); ++ i)
	{
		if (m_polys[i].layer == targetLayer)
		{
			m_polys[i].makeCounterClockwise();
			target_polys.push_back(m_polys[i]);
		}
	}
	
	vector<FPMRect> core_rects;
	vector<FPMRect> target_rects;
	for (int i = 0; i < m_rects.size(); ++ i)
	{
		if (m_rects[i].layer == coreLayer1 || m_rects[i].layer == coreLayer2)
		{
			core_rects.push_back(m_rects[i]);
		}
		else if (m_rects[i].layer == targetLayer)
		{
			target_rects.push_back(m_rects[i]);
		}
	}
	
	//cout << "------------- merge -------------" << endl;
	//cout << core_rects.size() << endl;
	for (int i = 0; i < core_rects.size(); ++ i)
	{
		//cout << core_rects[i];
		FPMPattern pt;
		for (int j = 0; j < target_polys.size(); ++ j)
		{
		  mergeRectPolygon(core_rects[i],target_polys[j],pt);
		}
		
		for (int j = 0; j < target_rects.size(); ++ j)
		{
			mergeRectRect(core_rects[i],target_rects[j],pt);
		}
		pt.bbox = core_rects[i];	
		m_subLayouts.push_back(pt);
		//cout << pt;
	}
	
}

void FPMLayout::recordPattern()
{
   cout<<"come in recordPattern"<<endl;
   ofstream f("badPattern.txt"); 
   for(int i=0;i<m_patterns.size();i++)
   {
       PTR(m_patterns[i],false);
       //record bounding box first
       f<<m_patterns[i].bbox.lb.x<<" "<<m_patterns[i].bbox.lb.y<<" ";       
       f<<m_patterns[i].bbox.width<<" "<<m_patterns[i].bbox.height<<endl;
       //record rect_set
       f<<m_patterns[i].rect_set.size()<<endl;
       for(int j=0;j<m_patterns[i].rect_set.size();j++)
       {
              f<<m_patterns[i].rect_set[j].rect_id<<" ";
              f<<m_patterns[i].rect_set[j].lb.x<<" "<<m_patterns[i].rect_set[j].lb.y<<" ";
              f<<m_patterns[i].rect_set[j].width<<" "<<m_patterns[i].rect_set[j].height<<" ";
              f<<m_patterns[i].rect_set[j].touchArray[0]<<" ";
              f<<m_patterns[i].rect_set[j].touchArray[1]<<" ";
              f<<m_patterns[i].rect_set[j].touchArray[2]<<" ";
              f<<m_patterns[i].rect_set[j].touchArray[3]<<" ";
              f<<endl;
        }
        if(i!=m_patterns.size()-1)f<<"#"<<endl;
        else f<<"%"<<endl;
        //getchar();
   }
   f.close();
   cout<<"record pattern finish"<<endl;
}

void FPMLayout::readPattern()
{
    cout<<"come in readPattern"<<endl;
    //int pattern_num=0;
    int rect_num=0;
    char flag='#';
    ifstream in("badPattern.txt"); 
    while(flag=='#')
    {
       FPMPattern pattern;
       in>>pattern.bbox.lb.x>>pattern.bbox.lb.y>>pattern.bbox.width>>pattern.bbox.height;
       in>>rect_num;
       vector<FPMRect> rect_set;
       for(int i=0;i<rect_num;i++)
       {
          FPMRect rect;
          in>>rect.rect_id>>rect.lb.x>>rect.lb.y>>rect.width>>rect.height;
          in>>rect.touchArray[0]>>rect.touchArray[1]>>rect.touchArray[2]>>rect.touchArray[3];
          rect_set.push_back(rect);
       }
       pattern.rect_set=rect_set;
       record_patterns.push_back(pattern);
       in>>flag;
       //cout<<record_patterns[pattern_num];
       //getchar();
       //pattern_num++;
    }
    //cout<<"record patterns size: "<<record_patterns.size()<<endl;
    //cout<<"subLayout size: "<<m_subLayouts.size()<<endl;
    //cout<<record_patterns[99];
    //getchar();
}

FPMTempEdgeVector FPMLayout::generateRing(FPMPattern &pattern)
{
    for(int i=0;i<pattern.vertical_edge.size();i++)
    {
       if(pattern.vertical_edge[i].point.x==pattern.bbox.lb.x||pattern.vertical_edge[i].point.x==pattern.bbox.lb.x+pattern.bbox.width)
           {
                //cout<<"come in bounding vertical---->"<<pattern.vertical_edge[i].edge_id<<endl;
                pattern.vertical_edge[i].bound=1;
                pattern.m_edge[pattern.vertical_edge[i].edge_id].bound=1;
           }
           else 
           {
                pattern.vertical_edge[i].bound=0;
                pattern.m_edge[pattern.vertical_edge[i].edge_id].bound=0;
           }
    }

    for(int j=0;j<pattern.horizontal_edge.size();j++)
    {
           if(pattern.horizontal_edge[j].point.y==pattern.bbox.lb.y||pattern.horizontal_edge[j].point.y==pattern.bbox.lb.y+pattern.bbox.height)
              {
                  //cout<<"come in bounding horizontal---->"<<pattern.horizontal_edge[j].edge_id<<endl;
                  //getchar();
                  pattern.horizontal_edge[j].bound=1;
                  pattern.m_edge[pattern.horizontal_edge[j].edge_id].bound=1;
              }
              else 
              {
                  pattern.horizontal_edge[j].bound=0;
                  pattern.m_edge[pattern.horizontal_edge[j].edge_id].bound=0;
              }
    }
    FPMTempEdgeVector temp_edge_vector;
    FPMTempEdge temp_edge;
    for(int m=0;m<pattern.m_edge_vector.size();m++)
    {
      for(int n=0;n<pattern.m_edge_vector[m].size();n++)
      {
        //cout<<n<<"www"<<endl;
        if(n==pattern.m_edge_vector[m].size()-1)
        {
           temp_edge.tempNode=make_pair(pattern.m_edge_vector[m][n].edge_id,pattern.m_edge_vector[m][0].edge_id);
           temp_edge.weight=0;
           temp_edge_vector.push_back(temp_edge);
        }
        else
        {
           temp_edge.tempNode=make_pair(pattern.m_edge_vector[m][n].edge_id,pattern.m_edge_vector[m][n+1].edge_id);
           temp_edge.weight=0;
           temp_edge_vector.push_back(temp_edge);
        }
      }
    }
    //cout<<"gouzao huan jieshu"<<endl;
    return temp_edge_vector;
}

void FPMLayout::generateEdge(FPMPattern &pattern)//use polygon generate edge
{
     //cout<<"come in generateEdge"<<endl;
     int count=0;
     FPMEdge temp_edge;
     temp_edge.bound = 0;
     vector<FPMEdge> p_edge;
     for(int i=0;i<pattern.m_poly_fulls.size();i++)
     {
       //cout<<"come in polygon"<<endl;
       for(int j=0;j<pattern.m_poly_fulls[i].p.ptlist.size();j++)
       {
         if(j!=pattern.m_poly_fulls[i].p.ptlist.size()-1)
         {
         //cout<<"normal"<<endl;
         //cout<<pattern.m_poly_fulls[i].p.ptlist[j].x<<","<<pattern.m_poly_fulls[i].p.ptlist[j].y<<endl;
         //cout<<pattern.m_poly_fulls[i].p.ptlist[j+1].x<<","<<pattern.m_poly_fulls[i].p.ptlist[j+1].y<<endl;
           //getchar();
           if(pattern.m_poly_fulls[i].p.ptlist[j].x==pattern.m_poly_fulls[i].p.ptlist[j+1].x)
           {
             //cout<<"vertical edge"<<endl;
             temp_edge.type=0;
             if(pattern.m_poly_fulls[i].p.ptlist[j].y<pattern.m_poly_fulls[i].p.ptlist[j+1].y)
             {
                temp_edge.belong_id=i;
                temp_edge.point.x=pattern.m_poly_fulls[i].p.ptlist[j].x;
                temp_edge.point.y=pattern.m_poly_fulls[i].p.ptlist[j].y;
                temp_edge.length=pattern.m_poly_fulls[i].p.ptlist[j+1].y-pattern.m_poly_fulls[i].p.ptlist[j].y;
                temp_edge.edge_id=count;
                count++;
                pattern.vertical_edge.push_back(temp_edge);
                pattern.m_edge.push_back(temp_edge);
                p_edge.push_back(temp_edge);
             }
             else
             {
                temp_edge.belong_id=i;
                temp_edge.point.x=pattern.m_poly_fulls[i].p.ptlist[j+1].x;
                temp_edge.point.y=pattern.m_poly_fulls[i].p.ptlist[j+1].y;
                temp_edge.length=pattern.m_poly_fulls[i].p.ptlist[j].y-pattern.m_poly_fulls[i].p.ptlist[j+1].y;
                temp_edge.edge_id=count;
                count++;
                pattern.vertical_edge.push_back(temp_edge);
                pattern.m_edge.push_back(temp_edge);
                p_edge.push_back(temp_edge);
             } 
           }
           if(pattern.m_poly_fulls[i].p.ptlist[j].y==pattern.m_poly_fulls[i].p.ptlist[j+1].y)
           {
             //cout<<"horizontal edge"<<endl;
             temp_edge.type=1;
             if(pattern.m_poly_fulls[i].p.ptlist[j].x<pattern.m_poly_fulls[i].p.ptlist[j+1].x)
             {
              temp_edge.belong_id=i;
              temp_edge.point.x=pattern.m_poly_fulls[i].p.ptlist[j].x;
              temp_edge.point.y=pattern.m_poly_fulls[i].p.ptlist[j].y;
              temp_edge.length=pattern.m_poly_fulls[i].p.ptlist[j+1].x-pattern.m_poly_fulls[i].p.ptlist[j].x;
              temp_edge.edge_id=count;
              count++;
              pattern.horizontal_edge.push_back(temp_edge);
              pattern.m_edge.push_back(temp_edge);
              p_edge.push_back(temp_edge);
             }
             else
             {
              temp_edge.belong_id=i;
              temp_edge.point.x=pattern.m_poly_fulls[i].p.ptlist[j+1].x;
              temp_edge.point.y=pattern.m_poly_fulls[i].p.ptlist[j+1].y;
              temp_edge.length=pattern.m_poly_fulls[i].p.ptlist[j].x-pattern.m_poly_fulls[i].p.ptlist[j+1].x;
              temp_edge.edge_id=count;
              count++;
              pattern.horizontal_edge.push_back(temp_edge);
              pattern.m_edge.push_back(temp_edge);
              p_edge.push_back(temp_edge);
             } 
           }
         }
         else//the last point of polygon
         {
             //cout<<"last point"<<endl;
             //cout<<pattern.m_poly_fulls[i].p.ptlist[j].x<<","<<pattern.m_poly_fulls[i].p.ptlist[j].y<<endl;
         //cout<<pattern.m_poly_fulls[i].p.ptlist[0].x<<","<<pattern.m_poly_fulls[i].p.ptlist[0].y<<endl;
             //cout<<"last endl"<<endl;
           //getchar();
           if(pattern.m_poly_fulls[i].p.ptlist[j].x==pattern.m_poly_fulls[i].p.ptlist[0].x)
           {
             //cout<<"vertical edge"<<endl;
             temp_edge.type=0;
             if(pattern.m_poly_fulls[i].p.ptlist[j].y<pattern.m_poly_fulls[i].p.ptlist[0].y)
             {
              temp_edge.belong_id=i;
              temp_edge.point.x=pattern.m_poly_fulls[i].p.ptlist[j].x;
              temp_edge.point.y=pattern.m_poly_fulls[i].p.ptlist[j].y;
              temp_edge.length=pattern.m_poly_fulls[i].p.ptlist[0].y-pattern.m_poly_fulls[i].p.ptlist[j].y;
              temp_edge.edge_id=count;
              count++;
              pattern.vertical_edge.push_back(temp_edge);
              pattern.m_edge.push_back(temp_edge);
              p_edge.push_back(temp_edge);
             }
             else
             {
              temp_edge.belong_id=i;
              temp_edge.point.x=pattern.m_poly_fulls[i].p.ptlist[0].x;
              temp_edge.point.y=pattern.m_poly_fulls[i].p.ptlist[0].y;
              temp_edge.length=pattern.m_poly_fulls[i].p.ptlist[j].y-pattern.m_poly_fulls[i].p.ptlist[0].y;
              temp_edge.edge_id=count;
              count++;
              pattern.vertical_edge.push_back(temp_edge);
              pattern.m_edge.push_back(temp_edge);
              p_edge.push_back(temp_edge);
             } 
           }
           if(pattern.m_poly_fulls[i].p.ptlist[j].y==pattern.m_poly_fulls[i].p.ptlist[0].y)
           {
             //cout<<"horizontal edge"<<endl;
             temp_edge.type=1;
             if(pattern.m_poly_fulls[i].p.ptlist[j].x<pattern.m_poly_fulls[i].p.ptlist[0].x)
             {
              temp_edge.belong_id=i;
              temp_edge.point.x=pattern.m_poly_fulls[i].p.ptlist[j].x;
              temp_edge.point.y=pattern.m_poly_fulls[i].p.ptlist[j].y;
              temp_edge.length=pattern.m_poly_fulls[i].p.ptlist[0].x-pattern.m_poly_fulls[i].p.ptlist[j].x;
              temp_edge.edge_id=count;
              count++;
              pattern.horizontal_edge.push_back(temp_edge);
              pattern.m_edge.push_back(temp_edge);
              p_edge.push_back(temp_edge);
             }
             else
             {
              temp_edge.belong_id=i;
              temp_edge.point.x=pattern.m_poly_fulls[i].p.ptlist[0].x;
              temp_edge.point.y=pattern.m_poly_fulls[i].p.ptlist[0].y;
              temp_edge.length=pattern.m_poly_fulls[i].p.ptlist[j].x-pattern.m_poly_fulls[i].p.ptlist[0].x;
              temp_edge.edge_id=count;
              count++;
              pattern.horizontal_edge.push_back(temp_edge);
              pattern.m_edge.push_back(temp_edge);
              p_edge.push_back(temp_edge);
             } 
           }

          }
       }
       pattern.m_edge_vector.push_back(p_edge);
       p_edge.clear();
     }
}


FPMEdgeVector FPMLayout::Sort(FPMEdgeVector xvector)
{
    bool exchange;
    FPMEdge temp_rect;
    for(int i=0;i<xvector.size();i++)
    {
        exchange=false;
        for(int j=xvector.size()-1;j>0;j--)
        {
            if(xvector[j].edge_id<xvector[j-1].edge_id)
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

FPMEdgeVector FPMLayout::Sort(FPMPattern &pattern)
{
    bool exchange;
    FPMEdgeVector xvector;
    FPMEdge temp_rect;
    xvector=pattern.m_edge;
    for(int i=0;i<xvector.size();i++)
    {
        exchange=false;
        for(int j=xvector.size()-1;j>0;j--)
        {
            if(xvector[j].edge_id<xvector[j-1].edge_id)
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

int* FPMLayout::calcF(const FPMEdgeVector &edge_vector)
{
   int* result=new int[edge_vector.size()];
   FPMEdgeVector result_vector=Sort(edge_vector);
   for(int i=0;i<result_vector.size();i++)
   {
      result[i]=result_vector[i].edge_id;
   }
   
   return result;
}

int* FPMLayout::deleteEdge(FPMPattern &pattern)
{
   FPMEdgeVectorIter iter;
   FPMEdgeVector result_vector;
   iter=pattern.m_edge.begin();
   while(iter!=pattern.m_edge.end())
   {
      if((*iter).bound==1)
      {
         pattern.m_edge.erase(iter);
         continue;
      }
      iter++;
   }
   
   iter=pattern.horizontal_edge.begin();
   while(iter!=pattern.horizontal_edge.end())
   {
      if((*iter).bound==1)
      {
         pattern.horizontal_edge.erase(iter);
         continue;
      }
      iter++;
   }
   
   iter=pattern.vertical_edge.begin();
   while(iter!=pattern.vertical_edge.end())
   {
      if((*iter).bound==1)
      {
         pattern.vertical_edge.erase(iter);
         continue;
      }
      iter++;
   }
   int* result=new int[pattern.m_edge.size()];
   result_vector=Sort(pattern);
   for(int i=0;i<result_vector.size();i++)
   {
      result[i]=result_vector[i].edge_id;
   }
   return result;
}

void FPMLayout::deleteBound(FPMTempEdgeVector &temp_vector, FPMPattern &pattern)
{
    FPMTempEdgeVectorIter iter;
    iter=temp_vector.begin();
    while (iter!=temp_vector.end())
    {
      if (pattern.m_edge[(*iter).tempNode.first].bound == 1 
            || pattern.m_edge[(*iter).tempNode.second].bound == 1)
      {
        temp_vector.erase(iter);
        continue;
      }
      iter++;
    }
    //delete bounding edge in m_edge_vector
    FPMPolyEdgeVectorIter piter;
    FPMEdgeVectorIter eiter;
    piter = pattern.m_edge_vector.begin();
    while(piter != pattern.m_edge_vector.end())
    {
        eiter=(*piter).begin();
        while(eiter != (*piter).end())
        {
           if ((*eiter).bound ==1 )
           {
             (*piter).erase(eiter);
             continue;
           }
           eiter++;
        }
        piter++;
    }
}

bool FPMLayout::FindEdge(const FPMEdgeVector &edge_vector,int edge_id)
{
    for(int i=0;i<edge_vector.size();i++)
    {
        if(edge_vector[i].edge_id==edge_id)return true;
    }
    return false;
}

FPMEdgeVector FPMLayout::GetNodeSize(const FPMTempEdgeVector &temp_vector,const FPMPattern &pattern)
{
    //cout<<"come in GetNodeSize"<<endl;
    FPMEdgeVector edge_vector;
    for(int i=0;i<temp_vector.size();i++)
    {
        if(!FindEdge(edge_vector,temp_vector[i].tempNode.first))edge_vector.push_back(pattern.m_edge[temp_vector[i].tempNode.first]);
        if(!FindEdge(edge_vector,temp_vector[i].tempNode.second))edge_vector.push_back(pattern.m_edge[temp_vector[i].tempNode.second]);
    }
    //cout<<"get size--->"<<edge_vector.size()<<endl;
    return edge_vector;
    //cout<<"come out GetNodeSize"<<endl;
}

static void drawResult(FPMPattern &pat, FPMPattern &layout, vector<GNULine*> &lv, string file)
{
  Plot plot;
  GNUPolygon *poly=NULL;
  GNUPointVector ptlist;
  
  int rectNum = layout.rect_set.size();
  int LX = layout.bbox.lb.x;
  int RX = layout.bbox.width+50;
  int BY = layout.bbox.lb.y;
  
	for (int i = 0; i < rectNum; ++ i)
	{
    int lx = layout.rect_set[i].lb.x - LX;
    int by = layout.rect_set[i].lb.y - BY;
    int height = layout.rect_set[i].height;
    int width = layout.rect_set[i].width;
    GNURectangle *r = new GNURectangle(lx, by, lx+width-1, by+height-1);
    plot.addRect(r);    
	}
	
  rectNum = pat.rect_set.size();
  int LX1 = pat.bbox.lb.x;
  int BY1 = pat.bbox.lb.y;
	for (int i = 0; i < rectNum; ++ i)
	{
    int lx = pat.rect_set[i].lb.x - LX1 + RX;
    int by = pat.rect_set[i].lb.y - BY1;
    int height = pat.rect_set[i].height;
    int width = pat.rect_set[i].width;
    GNURectangle *r = new GNURectangle(lx, by, lx+width-1, by+height-1);
    plot.addRect(r);
	}
  
  for(int i=0; i < (int)lv.size(); i++)
  {
    //first point is in pattern, second point is in layout
    lv[i]->x1() += RX;
//    printf("lv %d: [%d,%d]-[%d,%d]\n", lv[i]->x1(), lv[i]->y1(), lv[i]->x2(), lv[i]->y2());
    plot.addLine(lv[i]);
  }  
  
  plot.draw(file.c_str());
}

void FPMLayout::test(std::vector<FPMPattern> record_patterns,bool isBad)
{
  printf("Number of sublayouts: %d\n", m_subLayouts.size());
  //m_subLayouts.erase(m_subLayouts.begin()+3, m_subLayouts.end());
  
  vector<FPMPoint> mset;
  
  /*
  for(int i = 0; i < record_patterns.size(); ++ i)
  {
    for(int j = i+1; j < record_patterns.size(); ++ j)
    {
      if (checkCoverArea(record_patterns[i], record_patterns[j]))
      {
        record_patterns[j].clearMem();
        record_patterns.erase(record_patterns.begin()+j);
        -- j;
      }
    }
  }
  */
  
  //small block FPMTempEdgeVector and F array
  FPMTempEdgeVector blockEdge;
  vector<FPMTempEdgeVector> bl_vector;
  FPMEdgeVector medge_vector;
  vector<int*> bF_vector;
  int wcount=0,match_count=0;
  vector<FPMEdgeVector> medgeVector;
  vector<FPMRect> bbox_vector;
  
  int add_count=0;
  int boundArray[4]={0,0,0,0};
  FPMTempEdgeVector patternEdge,layoutEdge, patternRing,patternEdge_vertical,patternEdge_horizontal,layoutRing,layoutEdge_horizontal,layoutEdge_vertical;
  vector<FPMTempEdgeVector> pe_vector;
  
  vector<ARGraph<void,int> *> pattern_graph_vector;
  ARGraph<void,int> * sublayout_graph = NULL;
  vector<int*> weight_sub;
  int* weight_target = NULL;
  for(int j=0;j<record_patterns.size();j++)
  {
    //cout<<"pattern num "<<j<<endl;
    if(patternEdge.size()!=0)patternEdge.clear();
    //cout<<"aaaa "<<endl;
    if(patternRing.size()!=0)patternRing.clear();
    //cout<<"bbbb "<<endl;
    if(patternEdge_horizontal.size()!=0)patternEdge_horizontal.clear();
    //cout<<"cccc "<<endl;
    if(patternEdge_vertical.size()!=0)patternEdge_vertical.clear();
    //cout<<"dddd "<<endl;
    generateEdge(record_patterns[j]);
    //cout<<"generate edge finished"<<endl;
    patternRing=generateRing(record_patterns[j]);
    //cout<<"generate ring finished"<<endl;
    SweepEdgeHorizontal(record_patterns[j], patternEdge_horizontal);
    //cout<<"construct horizontal finished"<<endl;
    SweepEdgeVertical(record_patterns[j], patternEdge_vertical);
    //cout<<"construct vertical finished"<<endl;
    //get all the edge together
    patternEdge.insert(patternEdge.begin(),patternRing.begin(),patternRing.end());
    patternEdge.insert(patternEdge.begin(),patternEdge_horizontal.begin(),patternEdge_horizontal.end());
    patternEdge.insert(patternEdge.begin(),patternEdge_vertical.begin(),patternEdge_vertical.end());             
    //cout<<"construct patternEdge finished"<<endl;           
    deleteBound(patternEdge, record_patterns[j]);
          
          /*****************construct small block******************/
          int min_x,min_y,max_x,max_y,amin_x,amin_y,amax_x,amax_y;
          for(int m=0;m<record_patterns[j].m_poly_fulls.size();m++)
          {
              //cout<<"come in block"<<endl;
              //get bounding box of poly then decide the near area
              add_count=0;
              min_x=record_patterns[j].m_poly_fulls[m].p.ptlist[0].x;
              min_y=record_patterns[j].m_poly_fulls[m].p.ptlist[0].y;
              max_x=record_patterns[j].m_poly_fulls[m].p.ptlist[0].x;
              max_y=record_patterns[j].m_poly_fulls[m].p.ptlist[0].y;
              for(int p=0;p<record_patterns[j].m_poly_fulls[m].p.ptlist.size();p++)
              {
                  if(min_x>record_patterns[j].m_poly_fulls[m].p.ptlist[p].x)
                    min_x=record_patterns[j].m_poly_fulls[m].p.ptlist[p].x;
                  if(min_y>record_patterns[j].m_poly_fulls[m].p.ptlist[p].y)
                    min_y=record_patterns[j].m_poly_fulls[m].p.ptlist[p].y;
                  if(max_x<record_patterns[j].m_poly_fulls[m].p.ptlist[p].x)
                    max_x=record_patterns[j].m_poly_fulls[m].p.ptlist[p].x;
                  if(max_y<record_patterns[j].m_poly_fulls[m].p.ptlist[p].y)
                    max_y=record_patterns[j].m_poly_fulls[m].p.ptlist[p].y;
              }
              //near area of poly m
              amin_x=min_x-S1_DISTANCE;
              amax_x=max_x+S1_DISTANCE;
              amin_y=min_y-S1_DISTANCE;
              amax_y=max_y+S1_DISTANCE;
              //drawArea(record_patterns[j],amin_x,amin_y,amax_x,amax_y);
              blockEdge.clear();
              medge_vector.clear();
              
              //cout<<"after clear vector"<<endl;
              for(int q=0;q<patternEdge.size();q++)
              {
                 //cout<<"come in patternEdge "<<q<<endl;
                 if(record_patterns[j].m_edge[patternEdge[q].tempNode.first].belong_id==m)
                 {
                     
                     //vertical edge
                     if(record_patterns[j].m_edge[patternEdge[q].tempNode.second].type==0)
                     {
                         //vertical edge judge
                         if((record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.x>amin_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.x<amax_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.y>amin_y&&record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.y<amax_y)||(record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.x>amin_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.x<amax_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.y+record_patterns[j].m_edge[patternEdge[q].tempNode.second].length>amin_y&&record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.y+record_patterns[j].m_edge[patternEdge[q].tempNode.second].length<amax_y))
                          {
                             //in the naer area
                             blockEdge.push_back(patternEdge[q]);
                             if(record_patterns[j].m_edge[patternEdge[q].tempNode.second].belong_id!=m)add_count++;
                          }
                     }
                     //horizontal edge
                     else
                     {
                          //horizontal edge judge
                          if((record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.x>amin_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.x<amax_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.y>amin_y&&record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.y<amax_y)||(record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.x+record_patterns[j].m_edge[patternEdge[q].tempNode.second].length>amin_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.x+record_patterns[j].m_edge[patternEdge[q].tempNode.second].length<amax_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.y>amin_y&&record_patterns[j].m_edge[patternEdge[q].tempNode.second].point.y<amax_y))
                          {
                             //in the naer area
                             blockEdge.push_back(patternEdge[q]);
                             if(record_patterns[j].m_edge[patternEdge[q].tempNode.second].belong_id!=m)add_count++;

                          }

                     }
                 }
                 else if(record_patterns[j].m_edge[patternEdge[q].tempNode.second].belong_id==m)
                 {
                    //vertical
                    if(record_patterns[j].m_edge[patternEdge[q].tempNode.first].type==0)
                    {
                         if((record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.x>amin_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.x<amax_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.y>amin_y&&record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.y<amax_y)||(record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.x>amin_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.x<amax_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.y+record_patterns[j].m_edge[patternEdge[q].tempNode.first].length>amin_y&&record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.y+record_patterns[j].m_edge[patternEdge[q].tempNode.first].length<amax_y))
                         {
                           //in the naer area
                           blockEdge.push_back(patternEdge[q]);
                           if(record_patterns[j].m_edge[patternEdge[q].tempNode.first].belong_id!=m)add_count++;

                         }
                    }
                    //horizontal
                    else
                    {
                         if((record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.x>amin_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.x<amax_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.y>amin_y&&record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.y<amax_y)||(record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.x+record_patterns[j].m_edge[patternEdge[q].tempNode.first].length>amin_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.x+record_patterns[j].m_edge[patternEdge[q].tempNode.first].length<amax_x&&record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.y>amin_y&&record_patterns[j].m_edge[patternEdge[q].tempNode.first].point.y<amax_y))
                         {
                           //in the naer area
                           blockEdge.push_back(patternEdge[q]);
                           if(record_patterns[j].m_edge[patternEdge[q].tempNode.first].belong_id!=m)add_count++;
                         }
                    }
                 }
              }
              //cout<<"come in construct sub graph "<<blockEdge.size()<<endl;
              
              if(blockEdge.size()!=0&&add_count>=ADD_COUNT)
              {
                 //cout<<blockEdge<<endl;
                 medge_vector=GetNodeSize(blockEdge,record_patterns[j]);
                
                 if(medge_vector.size()>=MEDGE_SIZE)
                 {
                    bbox_vector.push_back(record_patterns[j].bbox);
                    bl_vector.push_back(blockEdge);
                    medgeVector.push_back(medge_vector);
                     int* nF=calcF(medge_vector);
                     int* ws=new int[blockEdge.size()];
                     bF_vector.push_back(nF);
                     weight_sub.push_back(ws);
                     pattern_graph_vector.push_back(toGetSubG(medge_vector.size(),blockEdge,nF,weight_sub[wcount]));
                     wcount++;
                 }
              }
          }
   }
    
   bool flag;
   int miss_match=0;
  // FPMPatternVectorIter piter=m_subLayouts.begin();
   
   for(int i=0;i<m_subLayouts.size();i++)
   {  
      cout<<"layout num "<<i<<endl;
      match_count=0;
      flag=false;
      layoutEdge.clear();
      
      generateEdge(m_subLayouts[i]);
      layoutRing=generateRing(m_subLayouts[i]);
      SweepEdgeHorizontal(m_subLayouts[i], layoutEdge_horizontal);
      SweepEdgeVertical(m_subLayouts[i], layoutEdge_vertical);
      layoutEdge.insert(layoutEdge.begin(),layoutRing.begin(),layoutRing.end());
      layoutEdge.insert(layoutEdge.begin(),layoutEdge_horizontal.begin(),layoutEdge_horizontal.end());
      layoutEdge.insert(layoutEdge.begin(),layoutEdge_vertical.begin(),layoutEdge_vertical.end());
     
      //draw subLayouts
      //drawLines(m_subLayouts[i],"subLayout");
      //getchar();
      if (weight_target)
      {
        delete []weight_target;
        weight_target = NULL;
      }
      //point_target[i]=new int[m_subLayouts[i].m_edge.size()];
     // for(int yu=0;yu<m_subLayouts[i].m_edge.size();yu++)
      //  point_target[i][yu]=m_subLayouts[i].m_edge[yu].length;
      if (sublayout_graph)
      {
        delete sublayout_graph;
        sublayout_graph = NULL;
      }
      
      weight_target=new int[layoutEdge.size()];
      sublayout_graph = toGetTargetG(m_subLayouts[i].m_edge.size(),layoutEdge,weight_target);
       
       for(int j=0;j<bl_vector.size();j++)
       { 
         //cout<<"bl_vector num: "<<j<<endl;
         vector<FPMResultPair> result;
         toMatchEdge(pattern_graph_vector[j],sublayout_graph,medgeVector[j].size(),bF_vector[j], result);
          if(result.size()!=0)
          {
             match_count++;
             if(match_count==MATCH_COUNT)
             {
                 reOutput(m_subLayouts[i],result[0],mset,pattern_graph_vector[j]->NodeCount());
             	  // drawEdge(medgeVector[j],bbox_vector[j]);
                 break;
             }
             for (int x = 0; x < result.size(); ++ x) 
             {
               delete[] result[x].sub_result;
               delete[] result[x].target_result;
             }
      	     result.clear();
          }
       }
       m_subLayouts[i].m_edge_vector.clear();
       m_subLayouts[i].m_edge.clear();
       m_subLayouts[i].vertical_edge.clear();
       m_subLayouts[i].horizontal_edge.clear();
       m_subLayouts[i].m_poly_fulls.clear();
   }
   
   /*///////////////JUST Comment it -- HY   
   for(int i=0;i<weight_sub.size();i++)
   { 
   	 delete [] weight_sub[i];
   }
   weight_sub.clear();
   */
   
  //remove redundancy
  for (int i = 0; i < mset.size(); ++ i)
  {
    for (int j = i+1; j < mset.size(); ++ j)
    {
      if (mset[i].x == mset[j].x && mset[i].y == mset[j].y)
      {
        mset.erase(mset.begin()+j);
//        printf("Removed one redundant match!\n");
        --j;
      }
    }
  }
  if(isBad)
    bad_point.insert(bad_point.begin(),mset.begin(),mset.end());
  else
    good_point.insert(good_point.begin(),mset.begin(),mset.end());
  
  for(int j = 0; j < bF_vector.size(); ++j)
  {
    delete []bF_vector[j];
  }
  bF_vector.clear();		
}

static bool within_rect( FPMPoint &pt, FPMRect &rect )
{
  if( pt.x < rect.lb.x - 600  ||  pt.y < rect.lb.y - 600 )
    return false;
  if( pt.x > rect.lb.x+rect.width+600  ||  pt.y > rect.lb.y+rect.height+600 )
    return false;

  return true;
}

void FPMLayout::compScore(vector<FPMPoint> &mset)
{
  if (mset.size() == 0)
  {
    printf("Total zero matches ...\n");
    return;
  }
  if (m_coreRects.size() == 0)
  {
    printf("No core rects for computing the matching score ...\n");
    return;
  }
  vector<bool> hits(m_coreRects.size(), false);
  int correctNum = 0;
  for (int i = 0; i < mset.size(); ++ i)
  {
    for (int j = 0; j < m_coreRects.size(); ++ j)
    {
      if (within_rect(mset[i], m_coreRects[j]))
      {
        hits[j] = true;
        ++ correctNum;
        break;
      }
    }
  }
  int hitNum = 0;
  for (int i = 0; i < hits.size(); ++ i)
  {
    if (hits[i])
      ++ hitNum;
  }
  float hitRatio = (float)hitNum/hits.size();
  float extraRatio = (float)(mset.size()-correctNum)/mset.size();
  printf("Total core rects: %d, hits %d, ratio: %.2f\n", m_coreRects.size(), hitNum, hitRatio);
  printf("Wrong matches: %d, total matches %d, extra ratio: %.2f\n", mset.size()-correctNum, mset.size(), extraRatio);
}

}
