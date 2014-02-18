#ifndef __FPMLAYOUT_H__
#define __FPMLAYOUT_H__
#include <ostream>
#include <string>
#include <vector>
#include <utility>
#include "FPMRect.h"
#include "FPMPoly.h"
#include "FPMPattern.h"
#include "FPMTempEdge.h"
#include "math.h"
#include "Plot.h"
using namespace std;
using namespace PLOT;
namespace FPM {

typedef struct _PMPoint
{
   int x;
   int y;
}PMPoint;

class FPMLayout
{
public:
  FPMLayout() {}
  ~FPMLayout() {}
  void storeCoreRects();
  void compScore(vector<FPMPoint> &mset);
  void setOutputFileName(std::string fileName) { m_outputFileName = fileName; }
/*****wangqin changed*******/
//  void patternRotation(FPMPattern& pt);
//  void patternSymmetry(FPMPattern& pt);
  void patternRotation(FPMPattern &pt, vector<FPMPattern> &rotatePatterns);
  void patternSymmetry(FPMPattern &pt, vector<FPMPattern> &symPatterns);
  void rotateAllPatterns(vector<FPMPattern> &allPatterns);
  
  //draw FPMEdge
  void drawEdge(FPMEdgeVector edge_vector,FPMRect bbox);
  //draw all the patterns in the layout
  void drawPatterns();
  //draw bounding box in layout
  void drawBox(FPMPattern pattern,int x,int y);
  //draw a specific pattern
  void drawPattern(FPMPattern pattern,char* filename);
  //rotate and symmetry all the patterns in layout
  //void rotatePatterns(int pnum);
  //draw the pattern after PTR
  void drawRects(FPMPattern pattern,char* filename);
  //draw FPMRectVector
  void drawRectVector(FPMRectVector rect_vector);
  //get subLayouts
  std::vector<FPMPattern> &getSubLayouts(){return m_subLayouts;}
  //record bad pattern to file
  void recordPattern();
  //read bad patterns from file
  void readPattern();
  //use polygon construct FPMEdge
  void generateEdge(FPMPattern &pattern);
  //generate a ring use FPMEdge
  FPMTempEdgeVector generateRing(FPMPattern &pattern);
  //delete edge which bounding is 1
  void deleteBound(FPMTempEdgeVector &temp_vector,FPMPattern &pattern); 
  //draw line
  void drawLines(FPMPattern pattern,char* filename);
  //getRing(FPMPattern &pattern,bool type);
  FPMTempEdgeVector getRing(FPMPattern &pattern,bool type);
  void test(std::vector<FPMPattern> record_patterns,bool isBad);
  
  int* deleteEdge(FPMPattern &pattern);
  int* calcF(const FPMEdgeVector &edge_vector);
  FPMEdgeVector Sort(FPMPattern &pattern);
  FPMEdgeVector Sort(FPMEdgeVector xvector);//can not use reference
  FPMEdgeVector GetNodeSize(const FPMTempEdgeVector &temp_vector,const FPMPattern &pattern);
  bool FindEdge(const FPMEdgeVector &temp_vector,int edge_id);
  void drawBlock(const FPMEdgeVector &edge_vector,const FPMPattern &pattern);
  void drawArea(const FPMPattern &pattern,int min_x,int min_y,int max_x,int max_y);
  
  bool within_box(int x,int y, FPMRect mwindow);
  void FinalResult(const std::vector<FPMPoint> &bad_point,const std::vector<FPMPoint> &good_point,ofstream &fout);
  void finalOutput(const std::vector<FPMRect> &windows,ofstream &fout);
/****************/
void clear() {
  	m_rects.clear();
  	m_polys.clear();
  	m_patterns.clear();
  	rotate_patterns.clear();
  }
  std::vector<FPMPoint> &getgoodPoint() { return good_point; }
  std::vector<FPMPoint> &getbadPoint() { return bad_point; }	
  void addRect(FPMRect &r) { m_rects.push_back(r); }
  std::vector<FPMRect> &getRects() { return m_rects; }
  int getRectNum() { return m_rects.size(); }
  
  void addPoly(FPMPoly &p);
  std::vector<FPMPoly> &getPolys() { return m_polys; }
	int getPolyNum() { return m_polys.size(); }
	
	void addPattern(FPMPattern &pt) {m_patterns.push_back(pt); }
  std::vector<FPMPattern> &getRotatePatterns() {return rotate_patterns; }
  std::vector<FPMPattern> &getPatterns() { return m_patterns; }
	int getPatternNum() { return m_patterns.size(); }
	void createPatterns();
        void createGoodPatterns();
        //yuanduo changed
        void createSubLayouts(bool flag);
        void createSubLayouts();
        bool checkOverlapOnFrame();
        bool checkOverlapOnWindow();
       	
  friend std::ostream & operator << (std::ostream &out, FPMLayout &l);

private:
  int subLayouts_inisize;
  std::string m_outputFileName;
  std::vector<FPMRect> m_coreRects;
  std::vector<FPMRect> m_rects;
  std::vector<FPMPoly> m_polys;
  std::vector<FPMPattern> rotate_patterns;
  std::vector<FPMPattern> m_patterns;
  std::vector<FPMPattern> m_rectLayouts;
  std::vector<FPMPattern> m_subLayouts;
  std::vector<FPMPattern> record_patterns;
  void mergeRectPolygon(FPMRect& r,FPMPoly& p,FPMPattern& pt);
  void mergeRectRect(FPMRect& r1,FPMRect& r2,FPMPattern& pt);
  void mergeRectRectToRect(FPMRect& r1,FPMRect& r2,FPMPattern& pt,int& id); 
  bool checkOverlap(FPMRect& r1,FPMRect& r2);
  void createSubLayoutsOnFrame();
  void createSubLayoutsOnWindow(int n);
  
  inline int minint(int a,int b) { if(a < b) return a; else return b;}
  inline int maxint(int a,int b) { if(a < b) return b; else return a;}

  std::vector<FPMPoint> good_point;
  std::vector<FPMPoint> bad_point;
  
};

}

#endif	//FPMLAYOUT_H
