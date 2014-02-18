#include <iostream>
#include "FPMRect.h"

namespace FPM {
using namespace std;

ostream & operator << (ostream &out, FPMRect &r)
{
  out << "[" << r.lb.x << "," << r.lb.y << "]-[" << r.lb.x+r.width << "," << r.lb.y+r.height << "]" << endl;
  out<< "id of FPMrect is: "<<r.rect_id<<endl;
  out << "width is: "<<r.width<<"height is: "<<r.height<<endl;
  return out;
}

}
