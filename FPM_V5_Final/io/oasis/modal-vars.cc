// oasis/modal-vars.cc -- state of modal variables defined by OASIS
//
// last modified:   30-Jun-2004  Wed  15:38
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include "modal-vars.h"

namespace Oasis {


ModalVars::ModalVars()  {
    reset();
}


ModalVars::~ModalVars() { }


// reset -- reset all modal variables to default state.
// Invoked on construction and for each CELL and <name> record.
// Most of the modal variables become undefined.

void
ModalVars::reset()
{
    mvFlags = 0;
    placementX = placementY = 0;
    geometryX  = geometryY  = 0;
    textX      = textY      = 0;
    xyRelative = false;     // Spec 21.2
}


}  // namespace Oasis
