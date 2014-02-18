#ifndef _TB_SEGMENT_H_
#define _TB_SEGMENT_H_

#include <algorithm>
#include "tbPoint.h"
using namespace std;

namespace KBool {

class tbSegment
{
  public:
    tbSegment() {}
    tbSegment(tbPoint &p1, tbPoint &p2);
    tbSegment & operator = (const tbSegment &s);
    tbSegment(const tbSegment &s);

    void set(const tbPoint &p1, const tbPoint &p2);

    tbPoint &getP1() { return mp1; }
    tbPoint &getP2() { return mp2; }
    tbPoint p1() const { return mp1; }
    tbPoint p2() const { return mp2; }

  private:
    //	ORIENTATION  LayerOrientation;
    tbPoint mp1;
    tbPoint mp2;
};

}

#endif

