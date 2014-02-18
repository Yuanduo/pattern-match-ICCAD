// gdsii/asc-conv.cc -- convert between GDSII Stream format and ASCII
//
// last modified:   30-Dec-2009  Wed  08:19
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "misc/utils.h"
#include "asc-conv.h"
#include "asc-scanner.h"
#include "asc-writer.h"
#include "glimits.h"
#include "scanner.h"
#include "rectypes.h"
#include "writer.h"


namespace Gdsii {

using namespace std;
using namespace SoftJin;
using namespace GLimits;


//======================================================================
//                           GDSII -> ASCII
//======================================================================


class GdsToAsciiConverter {
    GdsScanner          scanner;        // read records from GDSII file
    AsciiWriter         writer;         // write to ASCII file
    GdsToAsciiOptions   options;        // conversion options
    double              unitMultiplier; // multiply XY coordinates by this
    string              infilename;     // pathname of input file

public:
                GdsToAsciiConverter (const char* infname, const char* outfname,
                                     const GdsToAsciiOptions& options);
    void        convertFile();
    void        convertStructure (const char* sname, off_t soffset);

private:
    void        calculateUnitMultiplier (GdsRecord& rec);
    void        skipToStructure (const char* sname);
    void        convertRecord (GdsRecord& rec);
    void        convertNormalRecord (GdsRecord& rec);
    void        convertLibsecurRecord (GdsRecord& rec);
    void        convertReflibsRecord (GdsRecord& rec);
    void        convertXYRecord (GdsRecord& rec);
    void        convertUnitsRecord (GdsRecord& rec);
    void        abortConverter (const char* fmt, ...)
                                            SJ_PRINTF_ARGS(2,3)  SJ_NORETURN;

                GdsToAsciiConverter (const GdsToAsciiConverter&);  // forbidden
    void        operator= (const GdsToAsciiConverter&);            // forbidden
};



// constructor
//   infname    pathname of input file
//   outfname   pathname of output file
//   options    specify conversion options.
// This just sets things up for the conversion.  Call convertFile()
// or convertStructure() to perform the conversion.

GdsToAsciiConverter::GdsToAsciiConverter (const char* infname,
                                          const char* outfname,
                                          const GdsToAsciiOptions& opt)
  : scanner(infname, opt.gdsFileType),
    writer(outfname, opt.showOffsets),
    options(opt),
    infilename(infname)
{
}



// convertFile -- convert entire input file to ASCII

void
GdsToAsciiConverter::convertFile()
{
    GdsRecord   rec;

    writer.beginFile();
    do {
        scanner.getNextRecord(&rec);
        convertRecord(rec);
    } while (rec.getRecordType() != GRT_ENDLIB);
    writer.endFile();
}



// convertStructure -- convert single structure to ASCII
//   sname      name of structure.  A runtime_error exception is thrown
//              if the file contains no structure with this name
//   soffset    offset of structure in file, if known.  If this is > 0,
//              convertStructure() uses it to jump directly to the structure.
//              Otherwise (soffset <= 0) it reads and discards records
//              until it gets to the structure.

void
GdsToAsciiConverter::convertStructure (const char* sname, off_t soffset)
{
    GdsRecord   rec;

    // If the XY coordinates are to be printed in user units instead of
    // database units, get the conversion factor from the UNITS record.

    if (options.convertUnits) {
        do {
            scanner.getNextRecord(&rec);
        } while (rec.getRecordType() != GRT_UNITS);
        calculateUnitMultiplier(rec);
    }

    // Go to the BGNSTR record that begins the structure and convert
    // everything until ENDSTR.

    if (soffset > 0)            // offset of structure is known
        scanner.seekTo(soffset);
    else
        skipToStructure(sname);

    writer.beginFile();
    do {
        scanner.getNextRecord(&rec);
        convertRecord(rec);
    } while (rec.getRecordType() != GRT_ENDSTR);
    writer.endFile();
}



// calculateUnitMultiplier -- calc factor to convert DB units to user units.
//   rec        UNITS record from which we get the data
// Sets the member variable unitMultiplier which convertXYRecord() uses
// for conversion.  unitMultiplier is used, and this function is called,
// only when options.convertUnits is true.

void
GdsToAsciiConverter::calculateUnitMultiplier (/*in*/ GdsRecord& rec)
{
    assert (rec.getRecordType() == GRT_UNITS);
    double  dbToUser = rec.nextDouble();
    // double  dbToMeter = rec.nextDouble();    // XXX: junk this?
    // XXX: what should the output unit be?
    unitMultiplier = dbToUser;
}



// skipToStructure -- skip records until beginning of given structure.
//   sname      name of structure to whose definition we should go to.
//
// This is invoked when we need to jump to the beginning of a structure
// whose offset the caller does not know.  We read and skip records until
// we get to the desired structure.
//
// Postcondition:
//   The next record to be read from the GDSII file is the BGNSTR that
//   begins the structure named sname.

void
GdsToAsciiConverter::skipToStructure (const char* sname)
{
    // Keep scanning records, and for each BGNSTR see if the following
    // STRNAME contains the name we want.

    GdsRecord  rec;
    for (;;) {
        // Eat records until the next BGNSTR.
        for (;;) {
            scanner.getNextRecord(&rec);
            int  recType = rec.getRecordType();
            if (recType == GRT_BGNSTR)
                break;
            if (recType == GRT_ENDLIB)
                abortConverter("no structure named '%s' in file", sname);
        }

        // Got a BGNSTR record.  Save the offset because we have to
        // return to it if this is the structure we want.  The next
        // record should be STRNAME.

        off_t  offset = rec.getOffset();
        scanner.getNextRecord(&rec);
        if (rec.getRecordType() != GRT_STRNAME)
            abortConverter("bgnstr record not followed by strname");

        // If the name in the STRNAME record is not sname, continue the
        // search.  Note that recName need not be NUL-terminated.  If
        // there are any NUL bytes for padding, recLen will be greater
        // than length of the string in the record.

        int  snameLen = strlen(sname);
        int  recLen = rec.getLength();
        const char*  recName = rec.nextString();

        if (recLen < snameLen
                ||  strncmp(recName, sname, snameLen) != 0
                ||  (recLen > snameLen  &&  recName[snameLen] != Cnul) )
            continue;

        // The name matches.  Seek back to the beginning of the
        // structure and return.

        scanner.seekTo(offset);
        return;
    }
}



// convertRecord -- convert a single GDSII record to ASCII.

void
GdsToAsciiConverter::convertRecord (/*in*/ GdsRecord& rec)
{
    const GdsRecordTypeInfo*  rti = rec.getTypeInfo();

    writer.setOffset(rec.getOffset());
    writer.beginRecord(rti);

    switch (rti->getRecordType()) {
        case GRT_LIBSECUR: convertLibsecurRecord(rec);  break;
        case GRT_REFLIBS:  convertReflibsRecord(rec);   break;
        case GRT_XY:       convertXYRecord(rec);        break;
        case GRT_UNITS:    convertUnitsRecord(rec);     break;
        default:           convertNormalRecord(rec);    break;
    }
    writer.endRecord();
}



// Methods for printing different kinds of records.
// convertNormalRecord() handles most records; the others handle special
// cases.  The GdsRecord& argument to these functions is not const because
// the functions modify the record by consuming the data in it.

void
GdsToAsciiConverter::convertNormalRecord (/*in*/ GdsRecord& rec)
{
    // Print the data items in the record, separated by a single space.

    switch (rec.getDataType()) {
        case GDATA_NONE:
            break;

        case GDATA_BITARRAY:
            // BitArray records contain only one element.
            writer.writeInt(" %#x", rec.nextBitArray());
            break;

        case GDATA_SHORT:
            for (int j = rec.numItems();  --j >= 0;  )
                writer.writeInt(" %d", rec.nextShort());
            break;

        case GDATA_INT:
            for (int j = rec.numItems();  --j >= 0;  )
                writer.writeInt(" %d", rec.nextInt());
            break;

        case GDATA_DOUBLE:
            for (int j = rec.numItems();  --j >= 0;  )
                writer.writeDouble(" %.15g", rec.nextDouble());
            break;

        // A GDATA_STRING record contains either a single variable-length
        // string (itemSize == 0 in the RecordTypeInfo struct), or some
        // number of fixed-length records.  Other functions handle the
        // LIBSECUR and REFLIBS records, which have varying numbers of
        // strings.  This function handles only the FONTS record (with
        // exactly 4 strings) and records that contain a single string.

        case GDATA_STRING: {
            Uint  itemSize = rec.getTypeInfo()->itemSize;
            int  numItems = (itemSize == 0) ? 1 : rec.numItems();
            if (itemSize == 0)                  // variable length string
                itemSize = rec.getLength();     // occupies whole record
            while (--numItems >= 0) {
                writer.writeChar(' ');
                writer.writeStringLiteral(rec.nextString(), itemSize);
            }
            break;
        }
        default:
            assert(false);
    }
}



void
GdsToAsciiConverter::convertLibsecurRecord (/*in*/ GdsRecord& rec)
{
    // A LIBSECUR record contains 1 to MaxAclEntries triplets of shorts.
    // The triplet is (group number, user number, access rights).
    // Print the number of triplets before printing the triplets.

     int  numTriplets = rec.numItems()/3;
     writer.writeInt("  %d", numTriplets);
     while (--numTriplets >= 0) {
         // Leave extra space between triplets
         writer.writeInt("   %d", rec.nextShort());
         writer.writeInt(" %d",   rec.nextShort());
         writer.writeInt(" %d",   rec.nextShort());
     }
}



void
GdsToAsciiConverter::convertReflibsRecord (/*in*/ GdsRecord& rec)
{
    // A REFLIBS record has the pathnames of between 2 and MaxRefLibs
    // libraries.  Each pathname is NUL-padded if needed to 44 bytes.
    // Print the number of pathnames before printing the pathnames.

    int  numLibs = rec.numItems();
    writer.writeInt("  %d", numLibs);
    while (--numLibs >= 0) {
        writer.writeString("  ");
        writer.writeStringLiteral(rec.nextString(), ReflibNameLength);
    }
}



void
GdsToAsciiConverter::convertXYRecord (/*in*/ GdsRecord& rec)
{
    // An XY record contains the (X,Y) coordinates of a sequence of
    // points.  Each coordinate is a 4-byte integer.  Print the number
    // of points followed by the x,y values.  Print four x,y pairs per
    // line and insert some padding in the continuation lines so that
    // the numbers line up with those in the first line.

    int  numPoints = rec.numItems()/2;
    writer.writeInt(" %3d", numPoints);
    for (int j = 0;  j < numPoints;  ++j) {
        if (j % 4 != 0  ||  j == 0)
            writer.writeString("   ");          // extra space between points
        else
            writer.continueRecord("         "); // start new line

        if (options.convertUnits) {
            writer.writeDouble("%.6g",  rec.nextInt()*unitMultiplier);
            writer.writeDouble(" %.6g", rec.nextInt()*unitMultiplier);
        } else {
            writer.writeInt("%d",  rec.nextInt());
            writer.writeInt(" %d", rec.nextInt());
        }
    }
}



void
GdsToAsciiConverter::convertUnitsRecord (/*in*/ GdsRecord& rec)
{
    // Grab the units and then reset GdsRecord's cursor for getting data
    // from record.  calculateUnitMultiplier() has read the contents,
    // but we need to reread it to print it.

    if (options.convertUnits) {
        calculateUnitMultiplier(rec);
        rec.reset();
    }
    convertNormalRecord(rec);
}



// abortConverter -- abort conversion by throwing runtime_error
//   fmt        printf() format string for error message
//   ...        args, if any, for the format string

void
GdsToAsciiConverter::abortConverter (const char* fmt, ...)
{
    char  msgbuf[256];

    va_list  ap;
    va_start(ap, fmt);
    Uint  n = SNprintf(msgbuf, sizeof msgbuf, "file '%s': ",
                       infilename.c_str());
    if (n < sizeof(msgbuf) - 1)
        VSNprintf(msgbuf+n, sizeof(msgbuf)-n, fmt, ap);
    va_end(ap);

    ThrowRuntimeError("%s", msgbuf);
}


//======================================================================
//                           ASCII -> GDSII
//======================================================================

// AsciiToGdsConverter -- convert ASCII version of GDSII back to GDSII.

class AsciiToGdsConverter {
    AsciiScanner        scanner;        // reads tokens from ASCII file
    GdsWriter           writer;         // writes binary data to GDSII file

public:
                AsciiToGdsConverter (const char* infilename,
                                     const char* outfilename,
                                     const AsciiToGdsOptions& opt);
    void        convertFile();

private:
    void        convertNormalRecord (const GdsRecordTypeInfo* rti);
    void        convertLibsecurRecord();
    void        convertReflibsRecord();
    void        convertXYRecord();
    void        convertRecord (const GdsRecordTypeInfo* rti);

                AsciiToGdsConverter (const AsciiToGdsConverter&);  // forbidden
    void        operator= (const AsciiToGdsConverter&);            // forbidden
};



// constructor
//   infilename   pathname of ASCII file
//   outfilename  pathname of output GDSII file
// This just sets things up for the conversion.  Call convertFile()
// to convert the file.

AsciiToGdsConverter::AsciiToGdsConverter (const char* infilename,
                                          const char* outfilename,
                                          const AsciiToGdsOptions& opt)
  : scanner(infilename),
    writer(outfilename, opt.gdsFileType)
{
}



// convertFile -- do the conversion set up by the constructor

void
AsciiToGdsConverter::convertFile()
{
    GdsRecordType       recType;

    writer.beginFile();
    while (scanner.readRecordType(&recType))
        convertRecord(GdsRecordTypeInfo::get(recType));
    writer.endFile();
}



// convertRecord -- read tokens from ASCII file and write one GDSII record.

void
AsciiToGdsConverter::convertRecord (const GdsRecordTypeInfo*  rti)
{

    writer.beginRecord(rti);
    switch (rti->getRecordType()) {
        case GRT_LIBSECUR: convertLibsecurRecord();  break;
        case GRT_REFLIBS:  convertReflibsRecord();   break;
        case GRT_XY:       convertXYRecord();        break;

        default:
            if (rti->datatype != GDATA_NONE)
                convertNormalRecord(rti);
            break;
    }
    writer.endRecord();
}



// convertNormalRecord -- handle records with fixed number of data items.
// Does not handle records with no data (GDATA_NONE records) or records
// with varying numbers of items (FONTS, LIBSECUR, and XY).  Other
// functions handle these records.

void
AsciiToGdsConverter::convertNormalRecord (const GdsRecordTypeInfo* rti)
{
    assert (rti->getDataType() != GDATA_NONE);

    // itemSize is the size of the data items in the GDSII record.  If this
    // is 0, the record contains a single variable-length string.
    // Otherwise the GDSII record size is fixed because it contains a fixed
    // number of fixed-length data items.  We can calculate from itemSize
    // and the length of the GDSII record the number of tokens to convert
    // for this record.

    assert (rti->itemSize == 0  ||  rti->minLength == rti->maxLength);
    Uint  itemSize = rti->itemSize;
    int   numTokens = (itemSize == 0) ? 1 : rti->minLength/itemSize;

    switch (rti->getDataType()) {
        case GDATA_BITARRAY:
            writer.writeBitArray(scanner.readBitArray());
            break;

        case GDATA_SHORT:
            while (--numTokens >= 0)
                writer.writeShort(scanner.readShort());
            break;

        case GDATA_INT:
            while (--numTokens >= 0)
                writer.writeInt(scanner.readInt());
            break;

        case GDATA_DOUBLE:
            while (--numTokens >= 0)
                writer.writeDouble(scanner.readDouble());
            break;

        case GDATA_STRING:
            while (--numTokens >= 0) {
                Uint  length;
                const char*  str = scanner.readString(itemSize, &length);
                writer.writeString(str, length);
            }
            break;

        default:
            assert(false);
    }
}



void
AsciiToGdsConverter::convertLibsecurRecord()
{
    // A LIBSECUR record contains 1 to 32 triplets of shorts.
    // The triplet is (group number, user number, access rights).
    // In the ASCII version the number of triplets follows the record name.

    int  numShorts = 3 * scanner.readBoundedInteger(1, MaxAclEntries);
    while (--numShorts >= 0)
        writer.writeShort(scanner.readShort());
}



void
AsciiToGdsConverter::convertReflibsRecord()
{
    // A REFLIBS record has the pathnames of between 2 and MaxRefLibs
    // libraries.  Each pathname is NUL-padded if needed to 44 bytes.
    // In the ASCII version the number of library names follows the
    // record name.

    int numLibs = scanner.readBoundedInteger(MinReflibs, MaxReflibs);
    while (--numLibs >= 0) {
        Uint  length;
        const char*  str = scanner.readString(ReflibNameLength, &length);
        writer.writeString(str, length);
    }
}



void
AsciiToGdsConverter::convertXYRecord()
{
    // An XY record contains the (X,Y) coordinates of 1 to 600 points.
    // Each coordinate is a 4-byte integer.  In the ASCII version the
    // number of points follows the record name.

    int  numCoords = 2 * scanner.readBoundedInteger(MinXYPoints, MaxXYPoints);
    while (--numCoords >= 0)
        writer.writeInt(scanner.readInt());
}



//======================================================================
//                           External interface
//======================================================================

// The rest of the program need not see the classes.  A function
// interface will do.  For the GDSII -> ASCII conversion we provide two
// overloaded methods -- one to convert the entire file and one to
// convert a single structure.


// ConvertGdsToAscii -- convert entire GDSII file to ASCII

/*GLOBAL*/ void
ConvertGdsToAscii (const char* infilename, const char* outfilename,
                   const GdsToAsciiOptions& options)
{
    GdsToAsciiConverter  cvt(infilename, outfilename, options);
    cvt.convertFile();
}



// ConvertGdsToAscii -- convert single structure in GDSII file to ASCII
//   infilename    pathname of GDSII file
//   outfilename   pathname of output file
//   options    conversion options
//   sname      structure name
//   soffset    optional: offset of structure in file, if known.
//              The offset may be found using the FileIndex created by
//              GdsParser.  If soffset is omitted or <= 0,
//              GdsToAsciiConverter scans the file from the beginning
//              to locate the structure.

/*GLOBAL*/ void
ConvertGdsToAscii (const char* infilename, const char* outfilename,
                   const GdsToAsciiOptions& options,
                   const char* sname, off_t soffset /* = -1 */)
{
    GdsToAsciiConverter  cvt(infilename, outfilename, options);
    cvt.convertStructure(sname, soffset);
}



/*GLOBAL*/ void
ConvertAsciiToGds (const char* infilename, const char* outfilename,
                   const AsciiToGdsOptions& options)
{
    AsciiToGdsConverter  cvt(infilename, outfilename, options);
    cvt.convertFile();
}


} // namespace Gdsii
