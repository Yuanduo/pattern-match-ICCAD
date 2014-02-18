#include "tbObstacle.h"

namespace KBool {

tbObstacle::tbObstacle(long left, long right, long btm, long top)
{
  leftX = left;
  rightX = right;
  bottomY = btm;
  topY = top;
}

tbObstacle::tbObstacle()
{
}

void tbObstacle::setObs(long left, long right, long btm, long top)
{
  leftX = left;
  rightX = right;
  bottomY = btm;
  topY = top;
}

void tbObstacle::getObs(long &left, long &right, long &btm, long &top)
{
  left = leftX;
  right = rightX;
  btm = bottomY;
  top = topY;
}

}

