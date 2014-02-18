// gdsii/locator.h -- identify position and context in GDSII file being parsed
//
// last modified:   01-Mar-2005  Tue  22:13

#ifndef GDSII_LOCATOR_H_INCLUDED
#define GDSII_LOCATOR_H_INCLUDED

#include <sys/types.h>
#include <string>
#include "misc/globals.h"


namespace Gdsii {


// GdsLocator -- identify position and context in GDSII file
//
// GdsParser contains an object of this class.  It passes a reference to
// the object to the builder each time it parses a file.  Before it
// invokes a builder method it updates the context stored here.  The
// builder can use the information, e.g., when it needs to display an
// error message.  See the comment before class GdsBuilder in builder.h
// for the value that each member has within a call to a GdsBuilder
// method.
//
// For gzipped files the offset stored here is the offset in the
// uncompressed stream.


class GdsLocator {
    std::string filename;
    std::string strname;
    off_t       offset;

public:
	        GdsLocator (const char* filename) : filename(filename) {
	            offset = -1;
	        }
                // The compiler-generated copy constructor, copy
                // assignment operator, and destructor are fine.

    void        setOffset (off_t offset) {
	            this->offset = offset;
	        }
    void        setStructureName (const char* strname) {
                    this->strname = (strname == Null ? "" : strname);
	        }

    const char* getFileName() const {
	            return filename.c_str();
	        }
    const char* getStructureName() const {
	            return (strname.empty() ? Null : strname.c_str());
	        }
    off_t       getOffset() const {
	            return offset;
	        }
};


}  // namespace Gdsii

#endif	// GDSII_LOCATOR_H_INCLUDED
