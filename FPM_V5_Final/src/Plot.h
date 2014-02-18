#ifndef __PLOT_H__
#define __PLOT_H__

#include <cassert>
#include <vector>
#include <fstream>
#include <string>

using namespace std;

namespace PLOT {

	class GNUPoint {
		public:
			//constructors
			GNUPoint(int x, int y): l_id(0), l_x(x), l_y(y)
  		{
  		}
			GNUPoint(int i, int x, int y): l_id(i), l_x(x), l_y(y)
  		{
  		}

			//accossers
			int getId() { return l_id; }
			int &x() { return l_x; }
			int &y() { return l_y; }
		private:
			int l_id;
			int l_x, l_y;
	};

	typedef vector<GNUPoint *> GNUPointVector;
	typedef GNUPointVector::iterator GNUPointVectorItr;

	class GNUPolygon {
		public:
			//constructors
			GNUPolygon(): l_id(0), l_width(2)
  		{
  		}
			GNUPolygon(int i): l_id(i), l_width(2)
  		{
  		}
			~GNUPolygon()
			{
				for (int i = 0; i < l_points.size(); i ++)
				{
					delete l_points[i];
				}
			}
			//accossers
			int getId() { return l_id; }
			void addPoint(GNUPoint *pnt) { l_points.push_back(pnt); }
			GNUPoint *getPoint(int index) { assert(0 <= index && index < l_points.size()); return l_points[index]; }
			int getPointNum() { return l_points.size(); }
			GNUPointVector &getPoints() { return l_points; }
			void setWidth(int w) { l_width = w; }
			int &getWidth() { return l_width; }
		private:
			int l_id;
			int l_width;
			GNUPointVector l_points;
	};

	class GNURectangle {
		public:
			//constructors
			GNURectangle(int lx1, int ly1, int lx2, int ly2): id(0), l_x1(lx1), l_y1(ly1), l_x2(lx2), l_y2(ly2), colorStr("#0000FF")
  		{
  		}
			GNURectangle(int i, int lx1, int ly1, int lx2, int ly2, string cStr): id(i), l_x1(lx1), l_y1(ly1), l_x2(lx2), l_y2(ly2), colorStr(cStr)
  		{
  		}

			//accossers
			int getId() { return id; }
			int &x1() { return l_x1; }
			int &x2() { return l_x2; }
			int &y1() { return l_y1; }
			int &y2() { return l_y2; }
			void setColorStr(string str) { colorStr = str; }
			string getColorStr() { return colorStr; }
		private:
			int id;
			int l_x1, l_x2, l_y1, l_y2;
			string colorStr;
	};
  
	class GNULine {
		public:
			//constructors
			GNULine(int lx1, int ly1, int lx2, int ly2, int r, int g, int b);
			GNULine(int i, int lx1, int ly1, int lx2, int ly2): id(i), l_x1(lx1), l_y1(ly1), l_x2(lx2), l_y2(ly2), linetype(3), l_width(2), colorStr("#FF0000")
  		{
  		}
			GNULine(int i, int lx1, int ly1, int lx2, int ly2, int w): id(i), l_x1(lx1), l_y1(ly1), l_x2(lx2), l_y2(ly2), linetype(3), l_width(w), colorStr("#FF0000")
  		{
  		}
      
			//accossers
			int getId() { return id; }
			int &x1() { return l_x1; }
			int &x2() { return l_x2; }
			int &y1() { return l_y1; }
			int &y2() { return l_y2; }
			int getLineType() { return linetype; }
			void setWidth(int w) { l_width = w; }
			int &getWidth() { return l_width; }
			void setColorStr(string str) { colorStr = str; }
			string getColorStr() { return colorStr; }
		private:
			int id;
			int l_x1, l_x2, l_y1, l_y2;
			int linetype;
			int l_width;
			string colorStr;
	};

	typedef vector<GNURectangle *> GNURectVector;
	typedef GNURectVector::iterator GNURectVectorItr;
	typedef vector<GNULine *> GNULineVector;
	typedef GNULineVector::iterator GNULineVectorItr;
	typedef vector<GNUPolygon *> GNUPolygonVector;
	typedef GNUPolygonVector::iterator GNUPolygonVectorItr;

	class Plot {
		public:
			//constructors
			Plot()
			{
				l_title.clear();
			}
			~Plot();
			//modifiers
			void addRect(GNURectangle *r) { l_rects.push_back(r); }
			void addLine(GNULine *l) { l_lines.push_back(l); }
			void addPolygon(GNUPolygon *p) { l_polygons.push_back(p); }
			void addPoint(GNUPoint *pnt) { l_points.push_back(pnt); }
			void drawRect(ofstream &fileout, int rId, int x1, int y1, int x2, int y2, string colorStr);
			void drawLine(ofstream &fileout, int linetype, int linewidth, int x1, int y1, int x2, int y2, string colorStr = "#FF0000");
    	void drawLine(ofstream &fileout, int id, int linetype, int linewidth, int x1, int y1, int x2, int y2, string colorStr = "#FF0000");
    	void drawPoint(ofstream &fileout, int pId, int x, int y, string colorStr);
			void setTitle(string titleStr) { l_title = titleStr; }

			void draw(string file, int minX = 0, int minY = 0, int maxX = 0, int maxY = 0);
			void cleanMem();
		private:
			GNURectVector l_rects;
			GNULineVector l_lines;
			GNUPolygonVector l_polygons;
			GNUPointVector l_points;
			string l_title;
	};

}

#endif //__PLOT_H__
