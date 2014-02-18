// misc/timer.h -- show time taken to execute code segments
//
// last modified:   11-Mar-2004  Thu  18:55
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef MISC_TIMER_H_INCLUDED
#define MISC_TIMER_H_INCLUDED

#include <stdlib.h>
#include <sys/time.h>
#include "globals.h"

namespace SoftJin {


class CodeTimer {
    timeval     tvUser;         // process user time at interval start
    timeval     tvSys;          // process system time
    timeval     tvReal;         // wall clock time at interval start
public:
                CodeTimer();
    void        reset();
    void        lapTime(/*out*/ double* userTime, /*out*/ double* sysTime,
                        /*out*/ double* realTime);
};


} // namespace SoftJin

#endif  // MISC_TIMER_H_INCLUDED
