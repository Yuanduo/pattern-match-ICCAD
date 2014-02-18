#ifndef STATUSB_H
#define STATUSB_H

namespace KBool {

// abstract base class for own statusbar inherite from it
class A2DKBOOLDLLEXP StatusBar
{
public:
    // constructor & destructor
    StatusBar(){};
    ~StatusBar(){};

    virtual void SetXY( double = 0.0, double = 0.0 ) = 0;
    virtual void ResetCoord() = 0;
    virtual void  SetFile( char* = 0 ) = 0;
    virtual void SetProcess( char* = 0 ) = 0;
    virtual void  SetTime( time_t seconds = 0 ) = 0;
    virtual void SetRecording( int status = 0 ) = 0;
    virtual void SetZoom( float factor = 1 ) = 0;
    virtual void Reset() = 0;
    void     StartDTimer();
    void     EndDTimer();
    int    GetDTimerOn();
    time_t   GetDTimer();

protected:
    int    timer;
    time_t   oldtime;
    time_t   curtime;
};

}

#endif
