// gdsii/double.h -- convert between doubles and GDSII Stream format reals
//
// last modified:   07-May-2004  Fri  22:52
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef GDSII_DOUBLE_H_INCLUDED
#define GDSII_DOUBLE_H_INCLUDED

#include "misc/globals.h"


namespace Gdsii {

using SoftJin::Uchar;


double  GdsRealToDouble (const Uchar* str);
bool    DoubleToGdsReal (double val, /*out*/ Uchar* buf);


}       // namespace Gdsii

#endif  // GDSII_DOUBLE_H_INCLUDED
