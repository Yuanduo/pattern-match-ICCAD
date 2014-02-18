// gdsii/builder.cc -- interface between GdsParser and client code
//
// last modified:   02-Jan-2010  Sat  16:22
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include "builder.h"

namespace Gdsii {


//----------------------------------------------------------------------
//                              GdsDate
//----------------------------------------------------------------------

// setTime -- set GdsDate members using a Unix time value.
// Returns true if members were successfully set, false otherwise.

bool
GdsDate::setTime (time_t time)
{
    // The GDSII spec is vague about the contents of the fields.  It does
    // not say whether the fields give local time or UTC.  We assume local.

    struct tm*  tms = localtime(&time);
    if (tms == Null)
        return false;

    // GDSII applications seem to write year-1900 in the year field, so
    // we copy the value from localtime() as is.  They seem use the
    // range 1..12 for the month; localtime() uses 0..11.

    year   = tms->tm_year;
    month  = tms->tm_mon + 1;
    day    = tms->tm_mday;
    hour   = tms->tm_hour;
    minute = tms->tm_min;
    second = tms->tm_sec;
    return true;
}



// getTime -- get the Unix time value represented by the GdsDate fields
//   ptime      out: on success the time is stored here
// Returns true if the result can be represented, false otherwise.

bool
GdsDate::getTime (/*out*/ time_t* ptime) const
{
    struct tm  tms;

    tms.tm_year  = year;
    tms.tm_mon   = month - 1;
    tms.tm_mday  = day;
    tms.tm_hour  = hour;
    tms.tm_min   = minute;
    tms.tm_sec   = second;
    tms.tm_isdst = -1;

    time_t  time = mktime(&tms);
    if (time == -1)
        return false;
    *ptime = time;
    return true;
}


//----------------------------------------------------------------------
//                              GdsBuilder
//----------------------------------------------------------------------

// Provide default builder methods that do nothing so that derived
// classes need supply only the methods they need.


/*virtual*/ void
GdsBuilder::setLocator (const GdsLocator*)  { }

/*virtual*/ void
GdsBuilder::gdsVersion (int)  { }

/*virtual*/ void
GdsBuilder::beginLibrary (const char*,
                          const GdsDate&,
                          const GdsDate&,
                          const GdsUnits&,
                          const GdsLibraryOptions&)  { }

/*virtual*/ void
GdsBuilder::endLibrary()  { }

/*virtual*/ void
GdsBuilder::beginStructure (const char*,
                            const GdsDate&,
                            const GdsDate&,
                            const GdsStructureOptions&) { }

/*virtual*/ void
GdsBuilder::endStructure()  { }


/*virtual*/ void
GdsBuilder::beginBoundary (int, int,
                           const GdsPointList&,
                           const GdsElementOptions&)  { }

/*virtual*/ void
GdsBuilder::beginPath (int, int,
                       const GdsPointList&,
                       const GdsPathOptions&)  { }

/*virtual*/ void
GdsBuilder::beginSref (const char*, int, int,
                       const GdsTransform&,
                       const GdsElementOptions&)  { }

/*virtual*/ void
GdsBuilder::beginAref (const char*, Uint, Uint,
                       const GdsPointList&,
                       const GdsTransform&,
                       const GdsElementOptions&)  { }

/*virtual*/ void
GdsBuilder::beginNode (int, int,
                       const GdsPointList&,
                       const GdsElementOptions&)  { }

/*virtual*/ void
GdsBuilder::beginBox (int, int,
                      const GdsPointList&,
                      const GdsElementOptions&)  { }

/*virtual*/ void
GdsBuilder::beginText (int, int, int, int,
                       const char*,
                       const GdsTransform&,
                       const GdsTextOptions&)  { }

/*virtual*/ void
GdsBuilder::addProperty (int, const char*)  { }

/*virtual*/ void
GdsBuilder::endElement()  { }


}  // namespace Gdsii
