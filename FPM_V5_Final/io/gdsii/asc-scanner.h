// gdsii/asc-scanner.h -- scanner for ASCII version of GDSII files.
//
// last modified:   31-Aug-2004  Tue  21:44
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef GDSII_ASC_SCANNER_H_INCLUDED
#define GDSII_ASC_SCANNER_H_INCLUDED

#include "rectypes.h"


namespace Gdsii {

using SoftJin::Ushort;
using SoftJin::Uint;


// AsciiScanner -- class wrapper for flex-generated ASCII file scanner.
// The scanner that flex generates is non-reentrant.  Consequently at most
// one instance of AsciiScanner can exist at any time.  Flex can
// generate reentrant scanners, but those use virtual functions and
// iostreams, both of which we are better without.
//
// XXX: Recent versions of flex can generate reentrant C scanners, but
// using those might mean more work for somebody porting this code.

class AsciiScanner {
public:
    // The class of token returned by readToken().  TokKeyword and
    // its value field Token::keyword are currently unused.
    enum TokenType {
        TokEOF        = 0,
        TokRecordType = 1,      // value is one of the GRT_XXX enumerators
        TokKeyword    = 2,      // for keyword/value pairs in bitarray
        TokInteger    = 3,
        TokDouble     = 4,
        TokString     = 5
    };

    // The value of the token; the field used depends on the token type.

    union Token {
        GdsRecordType   recType;
        Uint            keyword;        // attribute name for bit array
        int             ival;
        double          dval;

        struct {
            char*       str;
            Uint        len;
        } sval;
    };

public:
    explicit    AsciiScanner (const char* fname);
                ~AsciiScanner();

    // readToken -- flex-generated scanner function.
    // This is the low-level interface; applications will typically use
    // one of the other methods below.

    TokenType   readToken (/*out*/ Token* tok);

    // Wrappers for readToken() that read a token of an expected type and
    // check for errors.

    bool        readRecordType (/*out*/ GdsRecordType* recType);
    int         readBoundedInteger (int minval, int maxval);
    Ushort      readBitArray();
    short       readShort();
    int         readInt();
    double      readDouble();
    const char* readString (Uint maxlen, /*out*/ Uint* length);


private:
    static const char*  TokenTypeName (TokenType tokType);
    static void checkTokenType (TokenType tokType, TokenType expectedType);
    static int  unescapeString (/*inout*/ char* str, int len);

                AsciiScanner (const AsciiScanner&);     // forbidden
    void        operator= (const AsciiScanner&);        // forbidden
};



// readBitArray -- read a bit-array value (an unsigned 2-byte integer).
// Accepts only positive numbers and returns an unsigned number because it
// does not make much sense to specify bit values using negative numbers.

inline Ushort
AsciiScanner::readBitArray() {
    const int  BitArrayMax  = 0xffff;
    return (readBoundedInteger(0, BitArrayMax));
}


// readShort -- read a GDSII 2-byte integer (GDATA_SHORT)

inline short
AsciiScanner::readShort() {
    const int  ShortMax = 32767;
    return (readBoundedInteger(-ShortMax-1, ShortMax));
}


// readInt -- read a GDSII 4-byte integer (GDATA_INT)
// Note that the bounds checks are not redundant because the valid range is
// defined by GDSII and not the C implementation.

inline int
AsciiScanner::readInt() {
    const int  IntMax = 2147483647;
    return (readBoundedInteger(-IntMax-1, IntMax));
}


}  // namespace Gdsii

#endif  //  GDSII_ASC_SCANNER_H_INCLUDED
