#ifndef LPOINT_H
#define LPOINT_H

#include "booleng.h"

namespace KBool {

class A2DKBOOLDLLEXP LPoint
{
public:
    LPoint();
    LPoint( B_INT const , B_INT const );
    LPoint( LPoint* const );

    void  Set( const B_INT, const B_INT );
    void  Set( const LPoint & );

    LPoint  GetPoint();
    B_INT  GetX();
    B_INT  GetY();
    void   SetX( B_INT );
    void  SetY( B_INT );
    bool  Equal( const LPoint a_point, B_INT Marge );
    bool  Equal( const B_INT, const B_INT , B_INT Marge );
    bool  ShorterThan( const LPoint a_point, B_INT marge );
    bool  ShorterThan( const B_INT X, const B_INT Y, B_INT );

    LPoint &operator=( const LPoint & );
    LPoint &operator+( const LPoint & );
    LPoint &operator-( const LPoint & );

    LPoint &operator*( int );
    LPoint &operator/( int );

    int  operator==( const LPoint & ) const;
    int  operator!=( const LPoint & ) const;

protected:
    B_INT _x;
    B_INT _y;

};

}

#endif
