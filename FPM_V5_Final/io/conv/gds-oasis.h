// conv/gds-oasis.h -- translate GDSII files to OASIS
//
// last modified:   02-Mar-2006  Thu  11:31
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef CONV_GDS_OASIS_H_INCLUDED
#define CONV_GDS_OASIS_H_INCLUDED

#include "misc/gzfile.h"
#include "misc/utils.h"        // for WarningHandler
#include "oasis/oasis.h"


namespace GdsiiOasis {

using SoftJin::Uint;
using SoftJin::FileHandle;
using SoftJin::WarningHandler;
using Oasis::Validation;


// GdsToOasisOptions -- options for GDSII->OASIS conversion
//
// gdsFileType          FileHandle::FileType
//      Whether input file is GDSII or gzipped GDSII.
//      The most convenient value is FileHandle::FileTypeAuto, which
//      decides by looking at the filename suffix.
//
// valScheme          Validation::Scheme
//      Validation scheme to use for the OASIS file.
//      Validation::Checksum32 is preferred.
//
// compress             bool
//      true => cells and name tables should be compressed
//      true is preferred.
//
// immediateNames       bool
//      true  => Each name record in the OASIS file is written
//               before it is referenced.
//      false => All name records are collected in strict-mode name tables
//               at the end of the file.
//
//      false is preferred, but some other vendors' tools require
//      this to be true.
//
// deleteDuplicates     bool
//      If true, duplicate elements are deleted.  If false, they are are
//      retained.  True is preferred because duplicate elements are
//      redundant.  But sometimes it is more convenient to leave them in
//      the output, e.g. to verify that gds2oasis is writing out as many
//      elements as it gets.
//
// pathToRectangle      bool
//      true  => convert PATH elements with a single horizontal or vertical
//               segment into a RECTANGLE rather than a PATH
//      false => convert all PATHs only into PATHs.
//
//      Some GDSII tools use PATH elements for rectangles because they are
//      more compact than BOUNDARY.  For other tools PATH and BOUNDARY have
//      different semantics, so a GDSII PATH should be converted only to an
//      OASIS PATH.
//
// relativeMode         bool
//      true  => cells will be written with xy-mode relative
//      false => xy-mode is absolute
//
// transformTexts       bool
//      true  => a GDSII text element with a transform will be converted to
//               a PLACEMENT of a special cell with just the converted
//               text element.
//      false => transforms for GDSII text elements will be ignored
//
// verbose              bool
//      If true, GdsToOasisConverter writes a message to stderr each
//      time it starts a new structure or a new batch of elements.
//
// optLevel             Uint
//      Optimization level.  How much effort to expend to make the
//      OASIS file small.  Values currently distinguished: 0, 1, >1.
//      0 is only for testing.


struct GdsToOasisOptions {
    FileHandle::FileType gdsFileType;
    Validation::Scheme   valScheme;
    bool        compress;
    bool        immediateNames;
    bool        deleteDuplicates;
    bool        pathToRectangle;
    bool        relativeMode;
    bool        transformTexts;
    bool        verbose;
    Uint        optLevel;
};


void    ConvertGdsToOasis (const char* infilename,
                           const char* outfilename,
                           WarningHandler warner,
                           const GdsToOasisOptions& options);


}  // namespace GdsiiOasis

#endif  // CONV_GDS_OASIS_H_INCLUDED
