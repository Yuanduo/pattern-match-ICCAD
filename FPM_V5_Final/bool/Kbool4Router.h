#ifndef _KBOOL4ROUTER_HPP_
#define _KBOOL4ROUTER_HPP_

#include <vector>
#include "kbool/booleng.h"
#include "kbool/_lnk_itr.h"

using namespace std;

namespace KBool {

class tbSegment;
class tbObstacle;

class KboolPoint 
{     
public:
    KboolPoint()
	: x(0), y(0)
	{  }
    KboolPoint(int x1, int y1)
	: x(x1), y(y1)
	{  }
    KboolPoint(const KboolPoint &kp){
	if (this != &kp)
        *this = kp;
    }
    ~KboolPoint()
	{  }

    KboolPoint & operator = (const KboolPoint &kp){
	x = kp.getX();
	y = kp.getY();

	return *this;
    }
    bool operator == (const KboolPoint &kp){
	return (x == kp.getX()) && (y == kp.getY());
    }

    long getX() const { return x; }
    long getY() const { return y; }
    void setPoint(int x1, int y1){
	x = x1; y = y1;
    }
private:
    long x;
    long y;

};

class Kbool4Router 
{
public:
    Kbool4Router() {}
    void setMasterPoly( vector<tbSegment> &mpoly);
    void setMasterPoly( tbObstacle &mpoly); 
    void setSecondaryPoly( vector<tbSegment> &spoly);
    void setSecondaryPoly( tbObstacle &spoly); 

    // secondaryPoly - masterPoly
    vector< vector<tbSegment> > getSubtractResult();

    ~Kbool4Router() {}
    void ArmBoolEng (Bool_Engine* booleng);
private:
    void subtract();
    void makePointsOrderInResultCounterClockWise ();
    bool isClockwise (vector<KboolPoint>& poly);

    vector<KboolPoint> masterPoly;
    vector<KboolPoint> secondaryPoly;
    vector< vector<KboolPoint> > resultPolyGroup;

    int layer_z;
    // RDIR segmentDir;
    // should be dealt with outside

    vector<KboolPoint> tbObstacle2KboolPoints (tbObstacle& tbObs);
    vector<KboolPoint> tbSegment2KboolPoints (tbSegment& tbSeg);
    vector<KboolPoint> tbSegmentsGroup2KboolPoints ( vector<tbSegment>& tbSegGrp);
};

}

#endif // _KBOOL4ROUTER_HPP_

