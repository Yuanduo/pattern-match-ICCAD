// timer.cc -- show time taken to execute code segments
//
// last modified:   26-Mar-2006  Sun  07:47
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <unistd.h>
#include "port/timing.h"
#include "timer.h"

namespace SoftJin {


CodeTimer::CodeTimer() {
    reset();
}


void
CodeTimer::reset()
{
    GetProcessCpuTimes(&tvUser, &tvSys);
    GetRealTime(&tvReal);
}



static inline double
timevalDiff(timeval t1, timeval t2)
{
    const double  Million = 1000*1000;

    return (t1.tv_sec - t2.tv_sec
            + double(t1.tv_usec - t2.tv_usec)/Million);
}



void
CodeTimer::lapTime(/*out*/ double* userTime, /*out*/ double* sysTime,
                  /*out*/ double* realTime)
{
    timeval  tvEndUser, tvEndSys, tvEndReal;

    GetProcessCpuTimes(&tvEndUser, &tvEndSys);
    GetRealTime(&tvEndReal);

    if (userTime != Null)  *userTime = timevalDiff(tvEndUser, tvUser);
    if (sysTime  != Null)  *sysTime  = timevalDiff(tvEndSys,  tvSys);
    if (realTime != Null)  *realTime = timevalDiff(tvEndReal, tvReal);
}


} // namespace SoftJin
