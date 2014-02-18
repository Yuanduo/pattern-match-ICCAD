#include "tbSegment.h"

using namespace std;

namespace KBool {

tbSegment::tbSegment(tbPoint &p1, tbPoint &p2) : mp1(p1), mp2(p2) 
{
}

tbSegment::tbSegment(const tbSegment &s)
{
  mp1 = s.p1();
  mp2 = s.p2();
}

void tbSegment::set(const tbPoint &p1, const tbPoint &p2)
{
  mp1 = p1;
  mp2 = p2;
}

tbSegment & tbSegment::operator = (const tbSegment &s)
{
  mp1 = s.p1();
  mp2 = s.p2();
}

}

