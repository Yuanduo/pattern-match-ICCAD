// port/fcntl.h -- allow files to be opened in binary mode in Windows
//
// last modified:   25-Mar-2006  Sat  10:19
//
// Copyright 2006 SoftJin Technologies Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef PORT_FCNTL_H_INCLUDED
#define PORT_FCNTL_H_INCLUDED

#include <fcntl.h>

namespace SoftJin {


#if defined(_WIN32) || defined(__WIN32__)
    const int BinaryOpenFlag = _O_BINARY;
#else
    const int BinaryOpenFlag = 0;
#endif


}  // namespace SoftJin

#endif	// PORT_FCNTL_H_INCLUDED
