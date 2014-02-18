#ifndef _TB_POINT_
#define _TB_POINT_

namespace KBool {

class tbPoint
{
  public:
    tbPoint(int x=0, int y=0);
    tbPoint(const tbPoint &p);

    int &x();
    int &y();
    int x() const;
    int y() const;

    void set(int x, int y);
    void setP(int x, int y);
    void setX(int x) { mx = x; }
    void setY(int y) { my = y; }

    tbPoint & operator = (const tbPoint &p);
    bool operator == (const tbPoint& p);

  private:
    int mx;
    int my;
};

}

#endif

