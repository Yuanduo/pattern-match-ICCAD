// port/timing.h -- get real time and process user/system times
//
// last modified:   26-Mar-2006  Sun  07:53
//
// Copyright 2006 SoftJin Technologies Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef PORT_TIMING_H_INCLUDED
#define PORT_TIMING_H_INCLUDED

#include <sys/time.h>
#if !defined(_WIN32) || defined(__CYGWIN__)
#   include <sys/resource.h>
#endif

namespace SoftJin {


// No idea how to get the times on Windows, so just set everything to 0
// there.


// GetProcessCpuTimes -- get CPU time (user and system) used by this process

inline void
GetProcessCpuTimes (/*out*/ timeval* pUserTime, /*out*/ timeval* pSysTime)
{
#if defined(_WIN32) && !defined(__CYGWIN__)

    pUserTime->tv_sec = 0;
    pUserTime->tv_usec = 0;
    pSysTime->tv_sec = 0;
    pSysTime->tv_usec = 0;

#else  // Unix or Cygwin

    rusage  ru;
    (void) getrusage(RUSAGE_SELF, &ru);
    *pUserTime = ru.ru_utime;
    *pSysTime  = ru.ru_stime;

#endif
}



// GetRealTime -- get current wall-clock time

inline void
GetRealTime (/*out*/ timeval* pRealTime)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
    pRealTime->tv_sec = 0;
    pRealTime->tv_usec = 0;
#else
    (void) gettimeofday(pRealTime, 0);
#endif
}


}  // namespace SoftJin

#endif	// PORT_TIMING_H_INCLUDED
