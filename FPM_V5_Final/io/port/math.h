// port/math.h -- portable interface to <cmath>
//
// last modified:   25-Mar-2006  Sat  10:15
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// Files that use isfinite() should #include this instead of <cmath>.
// The isfinite() here is defined in the global namespace, not in std.

#ifndef PORT_MATH_H_INCLUDED
#define PORT_MATH_H_INCLUDED

#include <cmath>


#ifdef __sun
    #include <ieeefp.h>
    inline bool  isfinite(double d)  { return finite(d); }
#endif


// XXX:  Recent versions of VC++ might already define isfinite().
#if defined(_WIN32) || defined(__WIN32__)
    #include <float.h>
    inline bool  isfinite(double d)  { return finite(d); }
#endif


#endif  // PORT_MATH_H_INCLUDED
