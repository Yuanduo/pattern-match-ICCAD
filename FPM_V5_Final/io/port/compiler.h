// port/compiler.h -- compiler-dependent definitions
//
// last modified:   01-Jan-2010  Fri  12:08
//
// Copyright 2009 SoftJin Technologies Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef PORT_COMPILER_H_INCLUDED
#define PORT_COMPILER_H_INCLUDED


#ifdef __GNUC__

// Many error functions take a printf-style format string and args.  We
// use a gcc extension to verify that the args match the format string.
// 'str' is the index of the format string and 'start' is the index of
// the first arg to be checked agains the format.  The index starts at 1
// for global functions, and at 2 for non-static class member functions
// (because of the implicit 'this' arg).

#define  SJ_PRINTF_ARGS(str, start)  \
                            __attribute__((format(printf, str, start)))
#define  SJ_SCANF_ARGS(str, start)  \
                            __attribute__((format(scanf, str, start)))


// This is for functions that do not return to their caller normally,
// usually because they always throw an exception.

#define  SJ_NORETURN        __attribute__((noreturn))


// This is for deprecated functions.

#define  SJ_DEPRECATED      __attribute__((deprecated))


//----------------------------------------------------------------------
#else  // not gcc


#define  SJ_PRINTF_ARGS(str, start)
#define  SJ_SCANF_ARGS(str, start)
#define  SJ_NORETURN
#define  SJ_DEPRECATED


#endif  // __GNUC__


#endif  // PORT_COMPILER_H_INCLUDED
