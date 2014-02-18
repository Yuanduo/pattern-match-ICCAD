#include <algorithm>
#include "Kbool4Router.h"
#include "tbSegment.h"
#include "tbObstacle.h"

using namespace std;

namespace KBool {

void Kbool4Router::setMasterPoly(vector<tbSegment> &mpoly) {
    resultPolyGroup.clear();
    masterPoly = tbSegmentsGroup2KboolPoints(mpoly);
}
// `isVia', `getLayer' hasn't been defined as const in `tbObstacle'
void Kbool4Router::setMasterPoly(tbObstacle &mpoly) {
    resultPolyGroup.clear();
    masterPoly = tbObstacle2KboolPoints(mpoly);
} 
void Kbool4Router::setSecondaryPoly(vector<tbSegment> &spoly) {
    resultPolyGroup.clear();
    secondaryPoly = tbSegmentsGroup2KboolPoints(spoly);
}

void Kbool4Router::setSecondaryPoly(tbObstacle &spoly) {
    resultPolyGroup.clear();
    secondaryPoly = tbObstacle2KboolPoints(spoly);
}

// secondaryPoly - masterPoly
vector< vector<tbSegment> >
Kbool4Router::getSubtractResult() {
    subtract();
    makePointsOrderInResultCounterClockWise();

  vector< vector<tbSegment> > resultSegPolyGroup;
  vector<tbSegment> resultSegPoly;
  tbPoint   tmpPoint1, tmpPoint2;
  tbSegment tmpSeg;
  // foreach polygon
  for ( size_t curPolyIndex = 0; curPolyIndex < resultPolyGroup.size(); curPolyIndex++){

    vector<KboolPoint> &curPoly = resultPolyGroup[curPolyIndex];
    // foreach points in polygon
    for (size_t curPointIndex = 0; curPointIndex < curPoly.size(); curPointIndex++){
      KboolPoint &curPoint = curPoly[curPointIndex];
      KboolPoint &nextPoint = curPoly[ ( curPointIndex + 1 ) % curPoly.size() ];
      tmpPoint1.setP(curPoint.getX(), curPoint.getY());
      tmpPoint2.setP(nextPoint.getX(), nextPoint.getY());
      tmpSeg.set(tmpPoint1, tmpPoint2);
      resultSegPoly.push_back(tmpSeg);
    }
    resultSegPolyGroup.push_back(resultSegPoly);
    resultSegPoly.clear();
  }

  return resultSegPolyGroup;
}


void Kbool4Router::subtract(){
  Bool_Engine* booleng = new Bool_Engine();
  ArmBoolEng( booleng );

  if ( booleng->StartPolygonAdd( GROUP_A ) ) {
    for (vector<KboolPoint>::iterator it = masterPoly.begin(); it != masterPoly.end(); it++) {
      booleng->AddPoint( it->getX(), it->getY() );
    }
  }
  booleng->EndPolygonAdd();

  if ( booleng->StartPolygonAdd( GROUP_B ) ) {
    for (vector<KboolPoint>::iterator it = secondaryPoly.begin(); it != secondaryPoly.end(); it++) {
      booleng->AddPoint( it->getX(), it->getY() );
    }
  }
  booleng->EndPolygonAdd();

  booleng->Do_Operation(BOOL_B_SUB_A);

  // extract result
  KboolPoint tmpPoint;
  vector<KboolPoint> resultPoly;
  while ( booleng->StartPolygonGet() ) {
    // foreach point in the polygon
    while ( booleng->PolygonHasMorePoints() ) {
      tmpPoint.setPoint((int)booleng->GetPolygonXPoint(), (int)booleng->GetPolygonYPoint());
      resultPoly.push_back(tmpPoint);
    }
    booleng->EndPolygonGet();

    resultPolyGroup.push_back(resultPoly);
    resultPoly.clear();
  }

  delete booleng;
}

vector<KboolPoint> Kbool4Router::tbSegmentsGroup2KboolPoints(vector<tbSegment> &tbSegGrp) {
    vector<KboolPoint> resultPointSet;
    vector<KboolPoint> tmpPoints;
    for (vector<tbSegment>::iterator it = tbSegGrp.begin(); it != tbSegGrp.end(); it++){
        vector<KboolPoint>::iterator headOfResult = resultPointSet.begin();
        vector<KboolPoint>::iterator tailOfResult = resultPointSet.end() - 1;
        tmpPoints = tbSegment2KboolPoints(*it);
        if ( resultPointSet.size() < 2) {
            for (size_t i = 0; i < tmpPoints.size(); i++) {
                resultPointSet.push_back(tmpPoints[i]);
            }
            continue;
        }
        if ( resultPointSet.size() == 2
            && ( (*headOfResult == tmpPoints[0])
                || (*headOfResult == tmpPoints[1]) ) ) {
            swap(*headOfResult, *tailOfResult);
        }
        
        tailOfResult = resultPointSet.end() - 1;
        if ( *tailOfResult == tmpPoints[0]) {
            resultPointSet.push_back( tmpPoints[1] );
        }
        else {
            resultPointSet.push_back( tmpPoints[0] );
        }
    }
    // remove the last one as all segments forming a circle
    resultPointSet.erase( resultPointSet.end() - 1);

    return resultPointSet;
}

vector<KboolPoint> Kbool4Router::tbObstacle2KboolPoints(tbObstacle &tbObs) {
    vector<KboolPoint> points;
    KboolPoint tmpPoint;
    long left = tbObs.left();
    long right = tbObs.right();
    long top = tbObs.top();
    long bottom = tbObs.bottom();
    
    tmpPoint.setPoint(right, top); points.push_back(tmpPoint);
    tmpPoint.setPoint(left, top); points.push_back(tmpPoint);
    tmpPoint.setPoint(left, bottom); points.push_back(tmpPoint);
    tmpPoint.setPoint(right, bottom); points.push_back(tmpPoint);
    
    return points;
}

vector<KboolPoint> Kbool4Router::tbSegment2KboolPoints(tbSegment &tbSeg) {
    vector<KboolPoint> points;
    KboolPoint tmpPoint;
    
    tmpPoint.setPoint(tbSeg.getP1().x(), tbSeg.getP1().y());
    points.push_back(tmpPoint);
    tmpPoint.setPoint(tbSeg.getP2().x(), tbSeg.getP2().y());
    points.push_back(tmpPoint);
    
    return points;
}

void Kbool4Router::ArmBoolEng( Bool_Engine* booleng ) {
  // set some global vals to arm the boolean engine
  double DGRID = 1000;  // round coordinate X or Y value in calculations to this
  double MARGE = 0.001;   // snap with in this range points to lines in the intersection routines
  // should always be > DGRID  a  MARGE >= 10*DGRID is oke
  // this is also used to remove small segments and to decide when
  // two segments are in line.
  double CORRECTIONFACTOR = 500.0;  // correct the polygons by this number
  double CORRECTIONABER   = 1.0;    // the accuracy for the rounded shapes used in correction
  double ROUNDFACTOR      = 1.5;    // when will we round the correction shape to a circle
  double SMOOTHABER       = 10.0;   // accuracy when smoothing a polygon
  double MAXLINEMERGE     = 1000.0; // leave as is, segments of this length in smoothen


  // DGRID is only meant to make fractional parts of input data which
  // are doubles, part of the integers used in vertexes within the boolean algorithm.
  // Within the algorithm all input data is multiplied with DGRID

  // space for extra intersection inside the boolean algorithms
  // only change this if there are problems
  int GRID = 10000;

  booleng->SetMarge( MARGE );
  booleng->SetGrid( GRID );
  booleng->SetDGrid( DGRID );
  booleng->SetCorrectionFactor( CORRECTIONFACTOR );
  booleng->SetCorrectionAber( CORRECTIONABER );
  booleng->SetSmoothAber( SMOOTHABER );
  booleng->SetMaxlinemerge( MAXLINEMERGE );
  booleng->SetRoundfactor( ROUNDFACTOR );
}

void Kbool4Router::makePointsOrderInResultCounterClockWise() {
    for (size_t i = 0; i < resultPolyGroup.size(); i++ ) {
        vector<KboolPoint> &curPoly = resultPolyGroup[i];
        // check the order of points
        vector<KboolPoint> newPoly;
        if ( isClockwise( curPoly ) ) {
            // a lousy algorithm (swap is better)
            for(vector<KboolPoint>::reverse_iterator rit = curPoly.rbegin(); rit != curPoly.rend(); rit++)
            {
                newPoly.push_back(*rit);
            }
            resultPolyGroup[i].clear();
            resultPolyGroup[i] = newPoly;
        }
    }
}

bool Kbool4Router::isClockwise( vector<KboolPoint> &poly ) {
    long aX = poly[1].getX() - poly[0].getX();
    long aY = poly[1].getY() - poly[0].getY();
    long bX = poly[2].getX() - poly[1].getX();
    long bY = poly[2].getY() - poly[1].getY();

    float product = ((float)aX * bY) - ((float)bX * aY);
    return (product < 0);
}

}
