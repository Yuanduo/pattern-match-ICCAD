// oasis/asc-scanner.h -- scanner for ASCII version of OASIS files.
//
// last modified:   01-Jan-2010  Fri  11:24
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef OASIS_ASC_SCANNER_H_INCLUDED
#define OASIS_ASC_SCANNER_H_INCLUDED

#include <inttypes.h>   // for uint32_t
#include <string>
#include "keywords.h"
#include "oasis.h"
#include "rectypes.h"


namespace Oasis {

using std::string;
using SoftJin::llong;
using SoftJin::Uint;
using SoftJin::Ullong;
using SoftJin::Ulong;


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
    enum TokenType {
        TokEOF             = 0,
        TokRecordID        = 1,         // value in recID is RID_* or RIDX_*
        TokKeyword         = 2,
        TokUnsignedInteger = 3,
        TokSignedInteger   = 4,
        TokReal            = 5,
        TokString          = 6,
        TokThreeDelta      = 7,         // 2-delta or 3-delta
        TokLpar            = 8,         // used only in g-deltas
        TokRpar            = 9,         // used only in g-deltas
        TokComma           = 10,        // used only in g-deltas
    };

private:
    bool        rereadToken;    // true => readToken() returns prev token again
    TokenType   tokType;        // type of last token read

    // The members below hold the value of the last token read; the
    // member actually used depends on the token type.  Conceptually
    // deltaVal and realVal are also part of the union.  They are
    // outside because their classes have constructors.

    union {
        Uint            recID;          // RID_* or RIDX_* value
        AsciiKeyword    keywordVal;
        Ullong          uintVal;
        llong           sintVal;
        struct {
            char*       data;           // points into flex's buffer
            Uint        len;
        } strVal;
    };
    Delta               deltaVal;
    Oreal               realVal;

public:
    explicit    AsciiScanner (const char* fname);
                ~AsciiScanner();

    TokenType   readToken();
    void        unreadToken();

    // Functions to get the type and value of the last token read.

    TokenType   getTokenType() const;
    Uint        getRecordID() const;
    AsciiKeyword  getKeywordValue() const;
    Ulong       getUnsignedIntegerValue() const;
    long        getSignedIntegerValue() const;
    void        getRealValue (/*out*/ Oreal* retvalp) const;
    void        getStringValue (/*out*/ string* retvalp) const;
    Delta       getDeltaValue() const;

    // Wrappers for readToken() that read a token, check that its type is
    // what is expected, and return the token value.

    Uint        readRecordID (/*out*/ Ullong* lineNum);
    AsciiKeyword  readKeyword();
    Uint        readInfoByte();

    Ulong       readUnsignedInteger();
    long        readSignedInteger();
    Ullong      readUnsignedInteger64();
    llong       readSignedInteger64();
    Ulong       readValidationScheme();
    uint32_t    readValidationSignature();
    void        readReal (/*out*/ Oreal* retvalp);
    void        readString (/*out*/ string* retvalp);

    long        readOneDelta();
    Delta       readTwoDelta();
    Delta       readThreeDelta();
    Delta       readGDelta();

private:
    void        scanToken();    // the flex-generated scanner function
    void        checkTokenType (TokenType expectedType);

    static const char*  TokenTypeName (TokenType tokType);
    static int  unescapeString (/*inout*/ char* str, int len);

private:
                AsciiScanner (const AsciiScanner&);     // forbidden
    void        operator= (const AsciiScanner&);        // forbidden
};



// readToken -- read next token from input, save it, and return its type.
// Returns the previous token again if it was unread.  To get the value
// of the token, use one of the getFooValue() methods.

inline AsciiScanner::TokenType
AsciiScanner::readToken()
{
    if (rereadToken)
        rereadToken = false;
    else
        scanToken();
    return tokType;
}


// unreadToken -- return the last token read to the input stream

inline void
AsciiScanner::unreadToken() {
    assert (! rereadToken);
    rereadToken = true;
}


// getTokenType -- return the type of the last token read

inline AsciiScanner::TokenType
AsciiScanner::getTokenType() const {
    return tokType;
}


// The getFoo() methods below return the value of the last token read.
// The precondition of each is that the token must have the appropriate
// type.


inline Uint
AsciiScanner::getRecordID() const {
    assert (tokType == TokRecordID);
    return recID;
}


inline AsciiKeyword
AsciiScanner::getKeywordValue() const {
    assert (tokType == TokKeyword);
    return keywordVal;
}


inline void
AsciiScanner::getStringValue (/*out*/ string* retvalp) const {
    assert (tokType == TokString);
    retvalp->assign(strVal.data, strVal.len);
}


inline Delta
AsciiScanner::getDeltaValue() const {
    assert (tokType == TokThreeDelta);
    return deltaVal;
}



// readOneDelta -- read token, verify that it is a 1-delta, return its value

inline long
AsciiScanner::readOneDelta() {
    // A 1-delta is just a signed-integer
    return (readSignedInteger());
}


}  // namespace Oasis

#endif  //  OASIS_ASC_SCANNER_H_INCLUDED
