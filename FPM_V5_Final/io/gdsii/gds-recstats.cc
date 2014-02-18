// gdsii/gds-recstats.cc -- show frequencies of record types in GDSII files
//
// last modified:   30-Dec-2009  Wed  20:00
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// usage:  gds-recstats gdsii-file ...

#include <cassert>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <exception>

#include "misc/timer.h"
#include "misc/utils.h"
#include "rectypes.h"
#include "scanner.h"


using namespace std;
using namespace SoftJin;
using namespace Gdsii;


const char  UsageMessage[] =
"usage:  %s gdsii-file ...\n"
"Each gdsii-file is assumed to be gzipped if its name ends with '.gz'.\n"
"\n";


static void
UsageError() {
    fprintf(stderr, UsageMessage, GetProgramName());
    exit(1);
}


// RecordStats -- statistics for each record type

struct RecordStats {
    Ullong      numRecs;        // how many records of this type seen
    Ullong      totalBytes;     // total bytes in all records of this type
    int         minLength;      // size of smallest record of this type
    int         maxLength;      // size of largest record of this type
};


// FileStats -- statistics obtained after scanning a GDSII Stream file

struct FileStats {
    Ullong      fileSize;
    double      userTime;       // user time to scan the file
    double      sysTime;        // system time
    double      realTime;       // elapsed time
    RecordStats recStats [GRT_MaxType + 1];
};


//----------------------------------------------------------------------


// AnalyzeFile -- scan GDSII Stream file and report statistics on record types.
//   fname      pathname of GDSII file

static void
AnalyzeFile (const char* fname)
{
    FileStats   fs;

    // Get the file size

    struct stat st;
    if (stat(fname, &st) == 0)
        fs.fileSize = st.st_size;
    else {
        Error("cannot stat %s: %s", fname, strerror(errno));
        return;
    }

    // Initialize the statistics counters

    for (int j = 0;  j <= GRT_MaxType;  ++j) {
        RecordStats* rs = &fs.recStats[j];
        rs->numRecs    = 0;
        rs->totalBytes = 0;
        rs->minLength  = INT_MAX;
        rs->maxLength  = 0;
    }

    // Scan the file and collect statistics on the record types

    CodeTimer   timer;
    GdsRecord   rec;
    GdsScanner  scanner(fname, FileHandle::FileTypeAuto);

    do {
        scanner.getNextRecord(&rec);
        RecordStats* rs = &fs.recStats[rec.getRecordType()];
        ++rs->numRecs;
        rs->totalBytes += rec.getLength();
        rs->minLength = std::min(rs->minLength, (int)rec.getLength());
        rs->maxLength = std::max(rs->maxLength, (int)rec.getLength());
    } while (rec.getRecordType() != GRT_ENDLIB);
    timer.lapTime(&fs.userTime, &fs.sysTime, &fs.realTime);

    // Print the statistics

    Ullong  totalRecs = 0;
    for (int j = 0;  j <= GRT_MaxType;  ++j)
        totalRecs += fs.recStats[j].numRecs;

    printf("File: %s\n", fname);
    printf("Size: %llu bytes\n", fs.fileSize);
    printf("Number of records: %llu\n", totalRecs);
    printf("Processing time: user: %.3f, sys: %.3f, real: %.3f seconds\n",
           fs.userTime, fs.sysTime, fs.realTime);

    // Give some idea of the processing speed.  The times may be zero for
    // small files because of clock granularity.

    if (fs.userTime+fs.sysTime > 0  &&  fs.realTime > 0) {
        double  size = double(fs.fileSize)/1048576.0;   // size in MB
        double  recs = double(totalRecs)/1000000.0;     // millions of recs

        printf("Processing speed: %.0f MB/sec cpu, %.0f MB/sec real\n"
               "                  %.2f MRec/sec cpu, %.2f MRec/sec real\n",
               size/(fs.userTime+fs.sysTime), size/fs.realTime,
               recs/(fs.userTime+fs.sysTime), recs/fs.realTime);
    }

    printf("\nNum  Name             NumRecs"
           " MinSize MaxSize AvgSize   TotalSize\n\n");

    int line = 0;
    for (int j = 0;  j <= GRT_MaxType;  ++j) {
        RecordStats* rs = &fs.recStats[j];
        if (rs->numRecs == 0)
            continue;

        const GdsRecordTypeInfo*  rti = GdsRecordTypeInfo::get(j);
        assert (rti != Null);

        printf("%3d  %-14s %9llu %7ld %7ld %7llu %11llu\n",
               j, rti->name, rs->numRecs,
               long(rs->minLength), long(rs->maxLength),
               (rs->totalBytes + rs->numRecs/2) / rs->numRecs,
               rs->totalBytes);
        if (++line % 5 == 0)
            printf("\n");
    }
    printf("\n\n");
}



int
main (int argc, char* argv[])
{
    SetProgramName(argv[0]);
    if (argc == 1)
        UsageError();

    try {
        for (int j = 1;  j < argc;  ++j)
            AnalyzeFile(argv[j]);
    }
    catch (const std::exception& exc) {
        FatalError("%s", exc.what());
    }
    return 0;
}
