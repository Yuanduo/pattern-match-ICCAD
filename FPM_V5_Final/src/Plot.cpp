#include <iostream>
#include <cmath>
#include <cstdio>
#include <limits.h>

#include "Plot.h"

using namespace std;

//#define USE_JPEG
//#define NO_LINE
#define NO_TEXT

namespace PLOT {

	Plot::~Plot()
	{
		for (int i = 0; i < l_points.size(); i ++)
		{
			GNUPoint *p = l_points[i];
			delete p;
		}
		l_points.clear();
		for (int i = 0; i < l_rects.size(); i ++)
		{
			GNURectangle *r = l_rects[i];
			delete r;
		}
		l_rects.clear();
		for (int i = 0; i < l_lines.size(); i ++)
		{
			GNULine *l = l_lines[i];
			delete l;
		}
		l_lines.clear();
		for (int i = 0; i < l_polygons.size(); i ++)
		{
			GNUPolygon *p = l_polygons[i];
			delete p;
		}
		l_polygons.clear();
	}

	void Plot::cleanMem()
	{
		for (int i = 0; i < l_rects.size(); i ++)
		{
			GNURectangle *r = l_rects[i];
			delete r;
		}
		l_rects.clear();
		for (int i = 0; i < l_lines.size(); i ++)
		{
			GNULine *l = l_lines[i];
			delete l;
		}
		l_lines.clear();
		for (int i = 0; i < l_polygons.size(); i ++)
		{
			GNUPolygon *p = l_polygons[i];
			delete p;
		}
		l_polygons.clear();
	}
  
	GNULine::GNULine(int lx1, int ly1, int lx2, int ly2, int r, int g, int b): id(0), l_x1(lx1), l_y1(ly1), l_x2(lx2), l_y2(ly2), linetype(3), l_width(2)
	{
	  char str[10];
	  assert(r >=0 && r <= 255);
	  assert(g >=0 && g <= 255);
	  assert(b >=0 && b <= 255);
	  sprintf(str, "#%.2X%.2X%.2X", r, g, b);
	  colorStr = str;
//	  printf("r: %d, g: %d, b:%d, str: %s\n", r, g, b, colorStr.c_str());
//	  exit(-1);
	}
  
	void Plot::draw(string file, int minX, int minY, int maxX, int maxY)
	{
		//return;

		ofstream fileout;
		char fileName[250];
		sprintf(fileName, "%s.plt", file.c_str());
		fileout.open(fileName);
		if (!fileout)
		{
			cout<<"failed to open file "<<fileName<<endl;
			exit(-1);
		}

    bool bound = true;
    if (!minX && !minY && !maxX && !maxY)
    {
      bound = false;
      minX = minY = INT_MAX;
      maxX = maxY = INT_MIN;
    }
    
//#ifndef NO_LINE
		for (int i = 0; i < l_lines.size(); i ++)
		{
			GNULine *l = l_lines[i];
			//      cout<<"drawing Line "<<i<<": ["<<l->x1()<<","<<l->y1()<<"]->["<<l->x2()<<","<<l->y2()<<"]"<<endl;
			if (!bound)
			{
  			if (minX > l->x1())
  			{
  				minX = l->x1();
  			}
  			if (minY > l->y1())
  			{
  				minY = l->y1();
  			}
  			if (maxX < l->x2())
  			{
  				maxX = l->x2();
  			}
  			if (maxY < l->y2())
  			{
  				maxY = l->y2();
  			}
  		}
      
			drawLine(fileout, l->getId(), l->getLineType(), l->getWidth(), l->x1(), l->y1(), l->x2(), l->y2(), l->getColorStr());
		}
//#endif

		for (int i = 0; i < l_polygons.size(); i ++)
		{
			GNUPolygon *p = l_polygons[i];
			GNUPointVector &points = p->getPoints();

			for (int j = 0; j < points.size(); j ++)
			{
				GNUPoint *p1, *p2;
				if (j == 0)
				{
					p1 = points[j];
					p2 = points[(int)points.size()-1];
				}
				else
				{
					p1 = points[j];
					p2 = points[j-1];
				}

				cout<<"drawing line "<<j<<": ["<<p1->x()<<","<<p1->y()<<"]->["<<p2->x()<<","<<p2->y()<<"]"<<endl;

				assert (p1->x() == p2->x() || p1->y() == p2->y());
				if (!bound)
				{
  				if (minX > p1->x())
  				{
  					minX = p1->x();
  				}
  				if (minY > p1->y())
  				{
  					minY = p1->y();
  				}
  				if (maxX < p1->x())
  				{
  					maxX = p1->x();
  				}
  				if (maxY < p1->y())
  				{
  					maxY = p1->y();
  				}
  			}
        
				drawLine(fileout, 3, 2, p1->x(), p1->y(), p2->x(), p2->y());
			}
		}

		for (int i = 0; i < l_rects.size(); i ++)
		{
			GNURectangle *r = l_rects[i];

			//      cout<<"drawing GNURectangle "<<i<<": ["<<r->x1()<<","<<r->y1()<<"]->["<<r->x2()<<","<<r->y2()<<"]"<<endl;

			assert (r->x1() <= r->x2() && r->y1() <= r->y2());
      if (!bound)
      {
  			if (minX > r->x1())
  			{
  				minX = r->x1();
  			}
  			if (minY > r->y1())
  			{
  				minY = r->y1();
  			}
  			if (maxX < r->x2())
  			{
  				maxX = r->x2();
  			}
  			if (maxY < r->y2())
  			{
  				maxY = r->y2();
  			}
  		}
			int rId = r->getId();
			drawRect(fileout, rId, r->x1(), r->y1(), r->x2(), r->y2(), r->getColorStr());

			//			drawLine(fileout, i%10, 2, r->x1(), r->y1(), r->x1(), r->y2());
			//			drawLine(fileout, i%10, 2, r->x1(), r->y1(), r->x2(), r->y1());
			//			drawLine(fileout, i%10, 2, r->x1(), r->y2(), r->x2(), r->y2());
			//			drawLine(fileout, i%10, 2, r->x2(), r->y1(), r->x2(), r->y2());
		}

		for (int i = 0; i < l_points.size(); i ++)
		{
			GNUPoint *p = l_points[i];

			//cout<<"drawing GNUPoint "<<i<<": ["<<p->x()<<","<<p->y()<<"]<<endl;
      
      if (!bound)
      {
  			if (minX > p->x())
  			{
  				minX = p->x();
  			}
  			if (minY > p->y())
  			{
  				minY = p->y();
  			}
  			if (maxX < p->x())
  			{
  				maxX = p->x();
  			}
  			if (maxY < p->y())
  			{
  				maxY = p->y();
  			}
  		}
			int pId = p->getId();
			drawPoint(fileout, pId, p->x(), p->y(), "#0000FF");
		}

#ifdef USE_JPEG
		fileout<<"set term jpeg enhanced"<<endl;// size "<<maxX-minX+3<<","<<maxY-minY+3<<endl;
#else
		fileout<<"set term post eps enhanced color solid"<<endl;// size "<<maxX-minX+3<<","<<maxY-minY+3<<endl;
#endif
		fileout<<"set size 1.0,1.0"<<endl;
#ifdef USE_JPEG
		fileout<<"set output \""<<file<<".jpg\""<<endl;
#else
		fileout<<"set output \""<<file<<".eps\""<<endl;
#endif
		fileout<<"#set xlabel 'x'"<<endl;
		fileout<<"#set ylabel 'y'"<<endl;
		fileout<<"set xtics"<<endl;
		fileout<<"set mxtics"<<endl;
		fileout<<"set ytics"<<endl;
		fileout<<"set mytics"<<endl;
		fileout<<"set xtics nomirror"<<endl;
		fileout<<"set ytics nomirror"<<endl;
		fileout<<"set x2tics"<<endl;
		fileout<<"set mx2tics"<<endl;
		fileout<<"set y2tics"<<endl;
		fileout<<"set my2tics"<<endl;
		fileout<<"set nokey"<<endl;
		fileout<<"#set size square"<<endl;
		//		cout<<"dimension: xrange["<<minX<<":"<<maxX<<"] yrange["<<minY<<":"<<maxY<<"]"<<endl;
//		fileout<<"set xrange["<<minX-100<<":"<<maxX+100<<"]"<<endl;
//		fileout<<"set yrange["<<minY-100<<":"<<maxY+100<<"]"<<endl;
//		fileout<<"set x2range["<<minX-100<<":"<<maxX+100<<"]"<<endl;
//		fileout<<"set y2range["<<minY-100<<":"<<maxY+100<<"]"<<endl;
		fileout<<"set xrange["<<minX<<":"<<maxX<<"]"<<endl;
		fileout<<"set yrange["<<minY<<":"<<maxY<<"]"<<endl;
		fileout<<"set x2range["<<minX<<":"<<maxX<<"]"<<endl;
		fileout<<"set y2range["<<minY<<":"<<maxY<<"]"<<endl;
		//		fileout<<"#set title 'layout figure generated by Hailong Yao_{\\@UCSD}^{CSE}'"<<endl;
		if (!l_title.empty())
			fileout<<"set title '"<<l_title<<"'"<<endl;

		fileout<<"plot -0.1 lt rgb \"#ffffff\""<<endl;
		fileout.close();
    
		char Commands[250];
		sprintf(Commands, "/home/admin/yaohl/ProgramFiles/GNUPlot/bin/gnuplot %s", fileName);
		system(Commands);
		sprintf(Commands, "ps2pdf %s.eps", file.c_str());
		system(Commands);

		sprintf(Commands, "\\rm %s", fileName);
		system(Commands);

		//clean the memory here
		cleanMem();
	}
  
	void Plot::drawPoint(ofstream &fileout, int pId, int x, int y, string colorStr)
	{
//#ifndef NO_TEXT
//		fileout<<"set label "<<pId+1<<" at "<<x<<","<<y<<" \""<<pId<<"\" "<<"font \"Helvetica,6\" front center tc rgb \"#000000\""<<endl;
//#endif

		fileout<<"set object rect from "<<x<<","<<y<<" to "<<x<<","<<y<<" back lw 1 fc rgb \""<<colorStr<<"\" fillstyle solid 0.50 bord -1"<<endl;
	}

	void Plot::drawRect(ofstream &fileout, int rId, int x1, int y1, int x2, int y2, string colorStr)
	{
#ifndef NO_TEXT
		//  	fileout<<"set label "<<rId+1<<" at "<<(x1+x2)/2<<","<<(y1+y2)/2<<" \""<<rId<<"\" "<<"front center tc rgb \"#000000\""<<endl;
		fileout<<"set label "<<rId+1<<" at "<<(x1+x2)/2<<","<<(y1+y2)/2<<" \""<<rId<<"\" "<<"font \"Helvetica,6\" front center tc rgb \"#000000\""<<endl;
#endif

		fileout<<"set object rect from "<<x1<<","<<y1<<" to "<<x2<<","<<y2<<" back lw 1 fc rgb \""<<colorStr<<"\" fillstyle solid 0.50 bord -1"<<endl;
		//		fileout<<"set object rect from "<<(x1+x2)/2-2<<","<<(y1+y2)/2-2<<" to "<<(x1+x2)/2+2<<","<<(y1+y2)/2+2<<" back lw 1 fc rgb \"#FF0000\""<<endl;
	}
  
	void Plot::drawLine(ofstream &fileout, int id, int linetype, int linewidth, int x1, int y1, int x2, int y2, string colorStr)
	{
		fileout<<"set label "<<id+1<<" at "<<(x1+x2)/2<<","<<(y1+y2)/2<<" \""<<id<<"\" "<<"font \"Helvetica,6\" front center tc rgb \"#000000\""<<endl;
		fileout<<"set arrow from "<<x1<<","<<y1<<" to "<<x2<<","<<y2<<" nohead lt rgb \""<<colorStr.c_str()<<"\" lw "<<1<<endl;
	}
	
	void Plot::drawLine(ofstream &fileout, int linetype, int linewidth, int x1, int y1, int x2, int y2, string colorStr)
	{
		fileout<<"set arrow from "<<x1<<","<<y1<<" to "<<x2<<","<<y2<<" nohead lt rgb \""<<colorStr.c_str()<<"\" lw "<<1<<endl;
		//  	if (linetype == 3)
		//  	{
		//	  	fileout<<"set arrow from "<<x1<<","<<y1<<" to "<<x2<<","<<y2<<" nohead lt rgb \"#FF0000\" lw "<<linewidth<<endl;
		//	  }
		//	  else if (linetype == 1)
		//	  {
		//	  	fileout<<"set arrow from "<<x1<<","<<y1<<" to "<<x2<<","<<y2<<" nohead lt rgb \"#BEBEBE\" lw "<<linewidth<<endl;
		//	  }
		//	  else if (linetype == 4)
		//	  {
		//	  	fileout<<"set arrow from "<<x1<<","<<y1<<" to "<<x2<<","<<y2<<" nohead lt rgb \"#00FF00\" lw "<<linewidth<<endl;
		//	  }
		//	  else
		//	  	fileout<<"set arrow from "<<x1<<","<<y1<<" to "<<x2<<","<<y2<<" nohead lt "<<linetype<<" lw "<<linewidth<<endl;

		//  	fileout<<"set arrow from "<<x1<<","<<y1<<" to "<<x2<<","<<y2<<" lt "<<linetype<<" lw "<<linewidth<<endl;
	}

}
