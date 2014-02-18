#ifndef TB_OBSTACLE_H
#define TB_OBSTACLE_H

#include "tbPoint.h"

namespace KBool {

class tbObstacle
{
  public:
    tbObstacle();
    tbObstacle(long left, long right, long btm, long top);
    void setObs(long left, long right, long btm, long top);
    void getObs(long &left, long &right, long &btm, long &top);
    int left() { return leftX; }
    int right() { return rightX; }
    int top() { return topY; }
    int bottom() { return bottomY; }
  private:
    long		leftX;
    long		rightX;
    long		bottomY;
    long		topY;
};

}

#endif

