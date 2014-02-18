%{
// oasis/asc-scanner.l -- scanner for ASCII version of OASIS
//
// last modified:   07-Jan-2010  Thu  17:12
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// This scanner uses flex-specific features.

#include <cassert>
#include <cctype>
#include <cerrno>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <unistd.h>

#include "port/compiler.h"
#include "misc/utils.h"
#include "asc-scanner.h"

using namespace std;
using namespace SoftJin;


const Ulong     MaxLong = LONG_MAX;
    // MaxLong is a Ulong because we compare it with Ulongs.


// The variables and functions below are static, i.e., local to this
// file.  They are not kept in class AsciiScanner or in namespace
// Oasis because that causes too many problems interfacing with the
// flex-generated code.


static int      fdin = -1;
    // File descriptor opened for reading on ASCII file.
    // A negative value means that the scanner is not active, i.e.,
    // no instance of class AsciiScanner exists.

static string   filename;
static Ullong   currLine;
    // The filename corresponding to fdin and the number of the line
    // currently being processed, both used only for error messages.



// abortScanner -- abort the scanner by throwing a runtime_error exception.
// The arguments are as for printf().  The exception message includes
// the file name, current line number, and the formatted error message.

static void
abortScanner (const char* fmt, ...) SJ_PRINTF_ARGS(1,2) SJ_NORETURN;

static void
abortScanner (const char* fmt, ...)
{
    char        buf[256];
    va_list     ap;

    va_start(ap, fmt);
    Uint  n = SNprintf(buf, sizeof(buf), "file '%s', line %llu: ",
                       filename.c_str(), currLine);
    if (n < sizeof(buf) - 1)
        VSNprintf(buf+n, sizeof(buf)-n, fmt, ap);
    va_end(ap);
    ThrowRuntimeError("%s", buf);

    // Useless call to keep gcc from warning about unused function.
    yy_fatal_error(buf);
}



inline long
StringToLong (const char* str)
{
    // As usual we forbid the minimum value (-2^31 or -2^63) to avoid
    // having to check for overflow when negating numbers.

    errno = 0;
    long  val = strtol(str, Null, 0);
    if (errno != 0  ||  val == LONG_MIN)
        abortScanner("signed-integer %s is too large", str);
    return val;
}



inline Ulong
StringToUlong (const char* str)
{
    errno = 0;
    Ulong  val = strtoul(str, Null, 0);
    if (errno != 0)
        abortScanner("unsigned-integer %s is too large", str);
    return val;
}



inline llong
StringToLlong (const char* str)
{
    errno = 0;
    llong  val = strtoll(str, Null, 0);
    if (errno != 0  ||  val == numeric_limits<llong>::min())
        abortScanner("signed-integer %s is too large", str);
    return val;
}



inline Ullong
StringToUllong (const char* str)
{
    errno = 0;
    Ullong  val = strtoull(str, Null, 0);
    if (errno != 0)
        abortScanner("unsigned-integer %s is too large", str);
    return val;
}



inline double
StringToDouble (const char* str)
{
    errno = 0;
    double  dval = strtod(str, Null);
    if (errno != 0)
        abortScanner("floating-point number %s is out of range", str);
    return dval;
}



inline float
StringToFloat (const char* str)
{
    errno = 0;
    double  dval = strtod(str, Null);           // no strtof() in Solaris 8
    if (errno != 0  ||  fabs(dval) < FLT_MIN  ||  fabs(dval) > FLT_MAX)
        abortScanner("floating-point number %s is out of range", str);
    return dval;
}



// Convenience macros used when a record type or keyword is recognized.
// If you change any record name in the rules, update rectypes.cc to
// match.  If you change any keyword name in the rules, update
// keywords.cc to match.

#define  ReturnRecordID(rid)            \
                tokType = TokRecordID;  \
                recID = rid;            \
                return

#define  ReturnKeyword(kword)           \
                tokType = TokKeyword;   \
                keywordVal = kword;     \
                return


#define  YY_FATAL_ERROR(msg)    abortScanner("%s", msg)
    // The default flex definition of YY_FATAL_ERROR calls exit() on a
    // fatal error.  We don't want that if this is part of a large
    // program with a GUI.


#define  YY_DECL        void Oasis::AsciiScanner::scanToken()
    // Define the flex-generated scanner function to be a member
    // function of AsciiScanner.  There is no return value or argument
    // because the rules store the token type and value in the data
    // member 'tok'.  The flex scanner is private; the public interface
    // is readToken(), which handles the pushback.


#define  YY_INPUT(buf, result, maxSize)  \
                if ((result = read(fdin, buf, maxSize)) < 0)     \
                    abortScanner("cannot read input: %s", strerror(errno));
    // Bypass stdio.
%}


%option case-insensitive
    /* Case is irrelevant in record IDs and keywords.  If you remove
     * this, update the name definitions below.  For example, hex_digit
     * should use '[0-9a-fA-F]' instead of just '[0-9a-f]' because case
     * should be ignored here regardless of what is done for record IDs
     * and keywords.
     */
%option never-interactive
    /* Input will never be a tty */

%option nounput
    /* Don't generate unput() because we don't need it. */

%option noyywrap
    /* Don't use yywrap() because we process only one file at a time. */


%x DELTA


digit           [0-9]
hex_digit       [0-9a-f]
od_int          {digit}+
hex_int         0x{hex_digit}+
uint            {od_int}|{hex_int}
sint            [-+]{uint}
integer         [-+]?{uint}

rational        {integer}"/"{uint}
float_f         ({digit}+\.{digit}*|\.{digit}+)
float_e         ({od_int}|{float_f})e[-+]?{digit}+
float_double    [-+]?({float_f}|{float_e})
float_single    ({float_double}|{integer})f
    /* An F suffix indicates a single-precision number as in C
     * We must allow integers too for float_single because AsciiRecordWriter
     * uses the printf format "%.15gf" to print floats.  For a number like
     * 5.0 this results in "5f" because printf %g format drops the decimal
     * point if all following digits are 0.
     */

norm_string     \"[^"\\\n]*\"
esc_string      \"([^"\\\n]|\\.)+\"
nonterm_string  \"([^"\\\n]|\\.)+
    /* norm_string    is a string without any escape sequences.
     * esc_string     is a string with one or more escape sequences.
     * nonterm_string is a string that is not terminated (we don't
     *                allow strings to span lines).
     */


rec_offset      \[[^]\n]+\]
    /* The writer may put the record offset in square brackets at the
     * beginning of the line.  We ignore it.
     */

%%
                Delta::Direction  dirn = Delta::East;
                /* The initial value is just to avoid a compiler warning */


{norm_string}   { /*  String contains no escape sequences.
                   */
                  yytext[yyleng-1] = Cnul;      /* kill ending "   */
                  tokType = TokString;
                  strVal.data = yytext + 1;     /* skip starting " */
                  strVal.len = yyleng - 2;
                  return;
                }

{esc_string}    { /*  String contains one or more escape sequences.
                   */
                  yytext[yyleng-1] = Cnul;
                  tokType = TokString;
                  strVal.data = yytext+1;
                  strVal.len = unescapeString(yytext+1, yyleng-2);
                  return;
                }

{nonterm_string} { abortScanner("non-terminated string literal"); }


"n:"            { dirn = Delta::North;      BEGIN DELTA; }
"s:"            { dirn = Delta::South;      BEGIN DELTA; }
"e:"            { dirn = Delta::East;       BEGIN DELTA; }
"w:"            { dirn = Delta::West;       BEGIN DELTA; }
"ne:"           { dirn = Delta::NorthEast;  BEGIN DELTA; }
"nw:"           { dirn = Delta::NorthWest;  BEGIN DELTA; }
"se:"           { dirn = Delta::SouthEast;  BEGIN DELTA; }
"sw:"           { dirn = Delta::SouthWest;  BEGIN DELTA; }

<DELTA>{uint}   { tokType = TokThreeDelta;
                  deltaVal.assign(dirn, StringToUlong(yytext));
                  BEGIN INITIAL;
                  return;
                }


{uint}          { tokType = TokUnsignedInteger;
                  uintVal = StringToUllong(yytext);
                  return;
                }

{sint}          { tokType = TokSignedInteger;
                  sintVal = StringToLlong(yytext);
                  return;
                }

{rational}      { char*  cp = strchr(yytext, '/');
                  *cp = Cnul;
                  long  numer = StringToLong(yytext);
                  long  denom = StringToLong(cp+1);
                  tokType = TokReal;
                  realVal.assign(numer, denom);
                  return;
                }

{float_double}  { tokType = TokReal;
                  realVal = StringToDouble(yytext);
                  return;
                }

{float_single}  { yytext[yyleng-1] = Cnul;      /* kill the f suffix */
                  tokType = TokReal;
                  realVal = StringToFloat(yytext);
                  return;
                }


{rec_offset}    { /* Ignore record offsets at beginning of the line */ }
\#.*            { /* Everything from # to end of line is a comment */  }

[ \t\r\f\v]+    { }

\n              { ++currLine; }


pad                    { ReturnRecordID (RID_PAD);                 }
start                  { ReturnRecordID (RID_START);               }
end                    { ReturnRecordID (RID_END);                 }
cellname               { ReturnRecordID (RID_CELLNAME);            }
textstring             { ReturnRecordID (RID_TEXTSTRING);          }
propname               { ReturnRecordID (RID_PROPNAME);            }
propstring             { ReturnRecordID (RID_PROPSTRING);          }
layername_geometry     { ReturnRecordID (RID_LAYERNAME_GEOMETRY);  }
layername_text         { ReturnRecordID (RID_LAYERNAME_TEXT);      }
cell                   { ReturnRecordID (RID_CELL_NAMED);          }
xyabsolute             { ReturnRecordID (RID_XYABSOLUTE);          }
xyrelative             { ReturnRecordID (RID_XYRELATIVE);          }
placement              { ReturnRecordID (RID_PLACEMENT);           }
placement_x            { ReturnRecordID (RID_PLACEMENT_TRANSFORM); }
text                   { ReturnRecordID (RID_TEXT);                }
rectangle              { ReturnRecordID (RID_RECTANGLE);           }
polygon                { ReturnRecordID (RID_POLYGON);             }
path                   { ReturnRecordID (RID_PATH);                }
trapezoid              { ReturnRecordID (RID_TRAPEZOID);           }
trapezoid_a            { ReturnRecordID (RID_TRAPEZOID_A);         }
trapezoid_b            { ReturnRecordID (RID_TRAPEZOID_B);         }
ctrapezoid             { ReturnRecordID (RID_CTRAPEZOID);          }
circle                 { ReturnRecordID (RID_CIRCLE);              }
property               { ReturnRecordID (RID_PROPERTY);            }
property_repeat        { ReturnRecordID (RID_PROPERTY_REPEAT);     }
xname                  { ReturnRecordID (RID_XNAME);               }
xelement               { ReturnRecordID (RID_XELEMENT);            }
xgeometry              { ReturnRecordID (RID_XGEOMETRY);           }
cblock                 { ReturnRecordID (RID_CBLOCK);              }

end_cblock             { ReturnRecordID (RIDX_END_CBLOCK);         }
cellname_table         { ReturnRecordID (RIDX_CELLNAME_TABLE);     }
textstring_table       { ReturnRecordID (RIDX_TEXTSTRING_TABLE);   }
propname_table         { ReturnRecordID (RIDX_PROPNAME_TABLE);     }
propstring_table       { ReturnRecordID (RIDX_PROPSTRING_TABLE);   }
layername_table        { ReturnRecordID (RIDX_LAYERNAME_TABLE);    }
xname_table            { ReturnRecordID (RIDX_XNAME_TABLE);        }

angle                  { ReturnKeyword (KeyAngle);                 }
attribute              { ReturnKeyword (KeyAttribute);             }
checksum32             { ReturnKeyword (KeyChecksum32);            }
crc32                  { ReturnKeyword (KeyCRC32);                 }
datatype               { ReturnKeyword (KeyDatatype);              }
delta_a                { ReturnKeyword (KeyDelta_A);               }
delta_b                { ReturnKeyword (KeyDelta_B);               }
end_extn               { ReturnKeyword (KeyEndExtn);               }
flip                   { ReturnKeyword (KeyFlip);                  }
halfwidth              { ReturnKeyword (KeyHalfwidth);             }
height                 { ReturnKeyword (KeyHeight);                }
horizontal             { ReturnKeyword (KeyHorizontal);            }
layer                  { ReturnKeyword (KeyLayer);                 }
mag                    { ReturnKeyword (KeyMag);                   }
name                   { ReturnKeyword (KeyName);                  }
none                   { ReturnKeyword (KeyNone);                  }
ptlist                 { ReturnKeyword (KeyPointList);             }
radius                 { ReturnKeyword (KeyRadius);                }
refnum                 { ReturnKeyword (KeyRefnum);                }
rep                    { ReturnKeyword (KeyRepetition);            }
square                 { ReturnKeyword (KeySquare);                }
standard               { ReturnKeyword (KeyStandard);              }
start_extn             { ReturnKeyword (KeyStartExtn);             }
string                 { ReturnKeyword (KeyString);                }
textlayer              { ReturnKeyword (KeyTextlayer);             }
texttype               { ReturnKeyword (KeyTexttype);              }
ctraptype              { ReturnKeyword (KeyTrapType);              }
values                 { ReturnKeyword (KeyValues);                }
vertical               { ReturnKeyword (KeyVertical);              }
width                  { ReturnKeyword (KeyWidth);                 }
x                      { ReturnKeyword (KeyX);                     }
y                      { ReturnKeyword (KeyY);                     }


[a-z_]+         { abortScanner("illegal keyword '%s' in input", yytext); }

"("             { tokType = TokLpar;   return; }
")"             { tokType = TokRpar;   return; }
","             { tokType = TokComma;  return; }

.               { char  buf[5];
                  CharToString(buf, sizeof buf, static_cast<Uchar>(*yytext));
                  abortScanner("illegal character `%s' in input", buf);
                }

<<EOF>>         { tokType = TokEOF;  return; }


%%
//======================================================================


namespace Oasis {

using namespace std;
using namespace SoftJin;


// AsciiScanner constructor
//   fname      pathname of file being scanned

AsciiScanner::AsciiScanner (const char* fname)
{
    // class members
    // Set tokType to EOF so that the getFooValue() methods will abort
    // if called before readToken().

    tokType = TokEOF;
    rereadToken = false;

    // file static variables

    if (fdin >= 0)
        ThrowRuntimeError("OASIS ASCII scanner already in use");
    fdin = OpenFile(fname, O_RDONLY);

    filename = fname;
    currLine = 1;
    yyrestart(Null);
}



AsciiScanner::~AsciiScanner()
{
    close(fdin);
    fdin = -1;
}



// TokenTypeName -- get string representation of token type for error messages.

/*static*/ const char*
AsciiScanner::TokenTypeName (TokenType tokType)
{
    switch (tokType) {
        case TokEOF:             return "EOF";
        case TokRecordID:        return "record name";
        case TokKeyword:         return "keyword";
        case TokUnsignedInteger: return "unsigned-integer";
        case TokSignedInteger:   return "signed-integer";
        case TokReal:            return "real";
        case TokString:          return "string";
        case TokThreeDelta:      return "delta";
        case TokLpar:            return "'('";
        case TokRpar:            return "')'";
        case TokComma:           return "','";

        default:
            assert(false);
            return "UNKNOWN";
    }
}



// checkTokenType -- verify that the token just read has the expected type
//   expectedType       the type of token we expected to read
// Throws a runtime_error if the data member tokType is not expectedType.

inline void
AsciiScanner::checkTokenType (TokenType expectedType)
{
    if (tokType != expectedType)
        abortScanner("expected %s, got %s",
                     TokenTypeName(expectedType), TokenTypeName(tokType));
}



Ulong
AsciiScanner::getUnsignedIntegerValue() const
{
    assert (tokType == TokUnsignedInteger);
    if (uintVal > ULONG_MAX)
        ThrowRuntimeError("unsigned-integer value %llu too large", uintVal);
    return (static_cast<Ulong>(uintVal));
}



long
AsciiScanner::getSignedIntegerValue() const
{
    assert (tokType == TokUnsignedInteger  ||  tokType == TokSignedInteger);

    if (tokType == TokSignedInteger) {
        if (sintVal <= LONG_MIN  ||  sintVal > LONG_MAX)
            abortScanner("signed-integer too large: %lld", sintVal);
        return (static_cast<long>(sintVal));
    }
    if (uintVal > LONG_MAX)
        abortScanner("signed-integer too large: %llu", uintVal);
    return (static_cast<long>(uintVal));
}



void
AsciiScanner::getRealValue (/*out*/ Oreal* retvalp) const
{
    assert (tokType == TokUnsignedInteger
            ||  tokType == TokSignedInteger
            ||  tokType == TokReal);

    // If the current token is an integer, make a real of type 0 or 1.
    // If the integer does not fit in a long, abort.  OasisScanner
    // converts such numbers to type 7 (double) with a warning, but here
    // it is better to abort.  ASCII files being parsed are typically
    // created manually, probably for testing, and it is better to force
    // the user to explicitly specify a double.

    switch (tokType) {
        case TokUnsignedInteger:
            if (uintVal > LONG_MAX) {
                abortScanner("integer %llu too large for real; "
                             "use double instead", uintVal);
            }
            *retvalp = static_cast<long>(uintVal);
            break;

        case TokSignedInteger:
            if (sintVal <= LONG_MIN  ||  sintVal > LONG_MAX) {
                abortScanner("integer %lld too large for real; "
                             "use double instead", sintVal);
            }
            *retvalp = static_cast<long>(sintVal);
            break;

        default:
            *retvalp = realVal;
            break;
    }
}



// Each readFoo() method below reads a token, checks that its type is
// what is expected, and returns the token value.  It throws
// runtime_error if the token has the wrong type.


// readRecordID -- read a token that is a record name.
// Returns the record-ID and the line number on which it occurs.
// The return value is Uint and not RecordID because one of the RIDX_*
// enumerators may also be returned.

Uint
AsciiScanner::readRecordID (/*out*/ Ullong* lineNum)
{
    readToken();
    checkTokenType(TokRecordID);
    *lineNum = currLine;
    return recID;
}



AsciiKeyword
AsciiScanner::readKeyword()
{
    readToken();
    checkTokenType(TokKeyword);
    return keywordVal;
}



Ulong
AsciiScanner::readUnsignedInteger()
{
    readToken();
    checkTokenType(TokUnsignedInteger);
    return (getUnsignedIntegerValue());
}



long
AsciiScanner::readSignedInteger()
{
    readToken();
    if (tokType != TokUnsignedInteger)
        checkTokenType(TokSignedInteger);
    return (getSignedIntegerValue());
}



Ullong
AsciiScanner::readUnsignedInteger64()
{
    readToken();
    checkTokenType(TokUnsignedInteger);
    return uintVal;
}



llong
AsciiScanner::readSignedInteger64()
{
    llong  retval;

    readToken();
    if (tokType == TokUnsignedInteger) {
        const Ullong  MaxLongLong = numeric_limits<llong>::max();
        if (uintVal > MaxLongLong)
            abortScanner("signed-integer too large: %llu", uintVal);
        retval = uintVal;
    } else {
        checkTokenType(TokSignedInteger);
        retval = sintVal;
    }
    return retval;
}



void
AsciiScanner::readReal (/*out*/ Oreal* retvalp)
{
    readToken();
    if (tokType != TokUnsignedInteger  &&  tokType != TokSignedInteger)
        checkTokenType(TokReal);
    getRealValue(retvalp);
}



void
AsciiScanner::readString (/*out*/ string* retvalp)
{
    readToken();
    checkTokenType(TokString);
    retvalp->assign(strVal.data, strVal.len);
}



// readValidationScheme -- read validation scheme in END record
// We accept both the unsigned-integer values and their keyword
// equivalents.

Ulong
AsciiScanner::readValidationScheme()
{
    readToken();
    if (tokType == TokUnsignedInteger)
        return uintVal;

    if (tokType == TokKeyword) {
        switch (keywordVal) {
            case KeyNone:        return Validation::None;
            case KeyCRC32:       return Validation::CRC32;
            case KeyChecksum32:  return Validation::Checksum32;

            default:
                abortScanner("expected validation scheme, got '%s'",
                             GetKeywordName(keywordVal));
        }
    }

    abortScanner("expected validation scheme, got %s",
                 TokenTypeName(tokType));
    return 0;   // make gcc happy
}



uint32_t
AsciiScanner::readValidationSignature()
{
    Ulong  val = readUnsignedInteger();
    if (val > numeric_limits<uint32_t>::max())
        abortScanner("validation signature %#lx does not fit in 32 bits", val);
    return val;
}



Delta
AsciiScanner::readTwoDelta()
{
    readToken();
    checkTokenType(TokThreeDelta);
    if (! deltaVal.isManhattan())
        abortScanner("expected 2-delta, got 3-delta");
    return deltaVal;
}



Delta
AsciiScanner::readThreeDelta()
{
    readToken();
    checkTokenType(TokThreeDelta);
    return deltaVal;
}



// readGDelta -- read a g-delta
// Unlike the other readFoo() methods, this may read several tokens.
// A g-delta in the ASCII file is either a 3-delta (e.g., sw:40)
// or a pair of signed-integers, e.g., (-20, 60).  The parentheses and
// comma are separate tokens.
//
// XXX: Should we forbid whitespace within the parentheses and make a
// single token out of it?

Delta
AsciiScanner::readGDelta()
{
    Delta  retval;

    // A 2-delta or 3-delta is also a g-delta.

    readToken();
    if (tokType == TokThreeDelta)
        return deltaVal;

    // Otherwise it must have the form (x, y) where x and y are
    // signed-integers.

    checkTokenType(TokLpar);
    retval.x = readSignedInteger();

    readToken();
    checkTokenType(TokComma);
    retval.y = readSignedInteger();

    readToken();
    checkTokenType(TokRpar);
    return retval;
}



inline bool
IsOctalDigit (char ch) {
    return (isdigit(static_cast<Uchar>(ch))  &&  ch < '8');
}



// unescapeString -- replace \-escaped sequences in string
//   str        NUL-terminated string with \\ and \ooo escape sequences
//   len        strlen(str)
// Overwrites str, replacing \\ by \, \" by ", and \ooo (where ooo
// is an octal number in the range 0..0377) by the value of ooo.
// Returns the new length of the string.

/*static*/ int
AsciiScanner::unescapeString (/*inout*/ char* str, int len)
{
    char*  cpi;                 // read ptr in str
    char*  cpo;                 // write ptr in str
    char*  end = str + len;

    // At the bottom of the loop and at each continue statement, cpi
    // points to the last char read and cpo points to the last char
    // written.  Because this function is called only for the rare
    // string literals that contain escape sequences, there is no need
    // to optimize for the case of no escape sequences.

    for (cpi = cpo = str;  cpi != end;  ++cpi, ++cpo) {
        // Normal char
        if (*cpi != '\\') {
            *cpo = *cpi;
            continue;
        }

        // Start of escape sequence.  The next char must be another
        // backslash, a double-quote, or an octal digit.

        if (++cpi == end)
            abortScanner("string literal ends with \\");
        if (*cpi == '\\'  ||  *cpi == '"') {
            *cpo = *cpi;
            continue;
        }
        if (! IsOctalDigit(*cpi))
            abortScanner("invalid escape sequence in string literal");

        // The input has a \ followed by 1, 2, or 3 octal digits.
        // cpi now points to the first digit; at the end cpi will
        // point to the last digit.

        int  num = *cpi - '0';
        if (cpi+1 != end  &&  IsOctalDigit(cpi[1]))
            num = 8*num + (*++cpi - '0');
        if (cpi+1 != end  &&  IsOctalDigit(cpi[1]))
            num = 8*num + (*++cpi - '0');
        if (num > 255)
            abortScanner("invalid escape sequence in string literal");
        *cpo = num;
    }
    *cpo = Cnul;
    return (cpo - str);
}


}  // namespace Oasis
