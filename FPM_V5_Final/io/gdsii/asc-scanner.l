%{
// gdsii/asc-scanner.l -- scanner for ASCII version of GDSII Stream
//
// last modified:   30-Dec-2009  Wed  19:17
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// This scanner uses flex-specific features.

#include <cassert>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <limits>
#include <string>

#include "misc/utils.h"
#include "asc-scanner.h"

using namespace std;
using namespace SoftJin;


// The variables and functions below are static, i.e., local to this
// file.  They are not kept in class AsciiScanner or in namespace
// Gdsii because that causes too many problems interfacing with the
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
abortScanner (const char* fmt, ...)  SJ_PRINTF_ARGS(1,2)  SJ_NORETURN;

static void
abortScanner (const char* fmt, ...)
{
    char  buf[256];
    Uint  n = SNprintf(buf, sizeof(buf), "file '%s', line %llu: ",
                       filename.c_str(), currLine);
    va_list  ap;
    va_start(ap, fmt);
    if (n < sizeof(buf) - 1)
        VSNprintf(buf+n, sizeof(buf)-n, fmt, ap);
    va_end(ap);
    ThrowRuntimeError("%s", buf);

    // Useless call to keep gcc from warning about unused function.
    yy_fatal_error(buf);
}


// Convenience macros used when a record type or bit-array keyword
// is recognized.  Bit-array keywords are currently unimplemented.

#define  ReturnRecordType(rtype)  \
                tok->recType = rtype;  return TokRecordType

#define  ReturnKeyword(kword)  \
                tok->keyword = kword;  return TokKeyword


// The default flex definition of YY_FATAL_ERROR calls exit() on a fatal
// error.  We don't want that if this is part of a large program with a
// GUI.

#define  YY_FATAL_ERROR(msg)  \
                abortScanner("%s", msg)


#define  YY_DECL  \
                Gdsii::AsciiScanner::TokenType  \
                Gdsii::AsciiScanner::readToken (/*out*/ Token* tok)
    // Define the flex-generated scanner function to be a member of
    // AsciiScanner.


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


digit           [0-9]
hex_digit       [0-9a-f]
od_int          {digit}+
hex_int         0x{hex_digit}+
integer         [-+]?({od_int}|{hex_int})

float_f         ({digit}+\.{digit}*|\.{digit}+)
float_e         ({od_int}|{float_f})e[-+]?{digit}+
float           [-+]?({float_f}|{float_e})f?

norm_string     \"[^"\\\n]*\"
esc_string      \"([^"\\\n]|\\.)+\"
nonterm_string  \"([^"\\\n]|\\.)+

rec_offset      \[[^]\n]+\]
    /* The writer may put the record offset in square brackets at the
     * beginning of the line.  We ignore it.
     */

%%


{norm_string}   { /*  String contains no escape sequences.
                   */
                  yytext[yyleng-1] = Cnul;      /* kill ending "   */
                  tok->sval.str = yytext + 1;   /* skip starting " */
                  tok->sval.len = yyleng - 2;
                  return TokString;
                }

{esc_string}    { /*  String contains one or more escape sequences.
                   */
                  yytext[yyleng-1] = Cnul;
                  tok->sval.str = yytext+1;
                  tok->sval.len = unescapeString(yytext+1, yyleng-2);
                  return TokString;
                }

{nonterm_string} { abortScanner("non-terminated string literal"); }


{integer}       { errno = 0;
                  long  lval = strtol(yytext, Null, 0);
                  if (errno != 0  ||  lval < INT_MIN  || lval > INT_MAX)
                      abortScanner("integer too large: %s", yytext);
                  tok->ival = lval;
                  return TokInteger;
                }

{float}         { errno = 0;
                  double dval = strtod(yytext, Null);
                  if (errno != 0) {
                      abortScanner("floating-point number out of range: %s",
                                   yytext);
                  }
                  tok->dval = dval;
                  return TokDouble;
                }


{rec_offset}    { /* Ignore record offsets at beginning of the line */ }
\#.*            { /* Everything from # to end of line is a comment */  }

[ \t\r\f\v]+    { }

\n              { ++currLine; }


header          { ReturnRecordType (GRT_HEADER);         }
bgnlib          { ReturnRecordType (GRT_BGNLIB);         }
libname         { ReturnRecordType (GRT_LIBNAME);        }
units           { ReturnRecordType (GRT_UNITS);          }
endlib          { ReturnRecordType (GRT_ENDLIB);         }
bgnstr          { ReturnRecordType (GRT_BGNSTR);         }
strname         { ReturnRecordType (GRT_STRNAME);        }
endstr          { ReturnRecordType (GRT_ENDSTR);         }
boundary        { ReturnRecordType (GRT_BOUNDARY);       }
path            { ReturnRecordType (GRT_PATH);           }
sref            { ReturnRecordType (GRT_SREF);           }
aref            { ReturnRecordType (GRT_AREF);           }
text            { ReturnRecordType (GRT_TEXT);           }
layer           { ReturnRecordType (GRT_LAYER);          }
datatype        { ReturnRecordType (GRT_DATATYPE);       }
width           { ReturnRecordType (GRT_WIDTH);          }
xy              { ReturnRecordType (GRT_XY);             }
endel           { ReturnRecordType (GRT_ENDEL);          }
sname           { ReturnRecordType (GRT_SNAME);          }
colrow          { ReturnRecordType (GRT_COLROW);         }
textnode        { ReturnRecordType (GRT_TEXTNODE);       }
node            { ReturnRecordType (GRT_NODE);           }
texttype        { ReturnRecordType (GRT_TEXTTYPE);       }
presentation    { ReturnRecordType (GRT_PRESENTATION);   }
string          { ReturnRecordType (GRT_STRING);         }
strans          { ReturnRecordType (GRT_STRANS);         }
mag             { ReturnRecordType (GRT_MAG);            }
angle           { ReturnRecordType (GRT_ANGLE);          }
reflibs         { ReturnRecordType (GRT_REFLIBS);        }
fonts           { ReturnRecordType (GRT_FONTS);          }
pathtype        { ReturnRecordType (GRT_PATHTYPE);       }
generations     { ReturnRecordType (GRT_GENERATIONS);    }
attrtable       { ReturnRecordType (GRT_ATTRTABLE);      }
elflags         { ReturnRecordType (GRT_ELFLAGS);        }
nodetype        { ReturnRecordType (GRT_NODETYPE);       }
propattr        { ReturnRecordType (GRT_PROPATTR);       }
propvalue       { ReturnRecordType (GRT_PROPVALUE);      }
box             { ReturnRecordType (GRT_BOX);            }
boxtype         { ReturnRecordType (GRT_BOXTYPE);        }
plex            { ReturnRecordType (GRT_PLEX);           }
bgnextn         { ReturnRecordType (GRT_BGNEXTN);        }
endextn         { ReturnRecordType (GRT_ENDEXTN);        }
tapenum         { ReturnRecordType (GRT_TAPENUM);        }
tapecode        { ReturnRecordType (GRT_TAPECODE);       }
strclass        { ReturnRecordType (GRT_STRCLASS);       }
format          { ReturnRecordType (GRT_FORMAT);         }
mask            { ReturnRecordType (GRT_MASK);           }
endmasks        { ReturnRecordType (GRT_ENDMASKS);       }
libdirsize      { ReturnRecordType (GRT_LIBDIRSIZE);     }
srfname         { ReturnRecordType (GRT_SRFNAME);        }
libsecur        { ReturnRecordType (GRT_LIBSECUR);       }
border          { ReturnRecordType (GRT_BORDER);         }
softfence       { ReturnRecordType (GRT_SOFTFENCE);      }
hardfence       { ReturnRecordType (GRT_HARDFENCE);      }
softwire        { ReturnRecordType (GRT_SOFTWIRE);       }
hardwire        { ReturnRecordType (GRT_HARDWIRE);       }
pathport        { ReturnRecordType (GRT_PATHPORT);       }
nodeport        { ReturnRecordType (GRT_NODEPORT);       }
userconstraint  { ReturnRecordType (GRT_USERCONSTRAINT); }
spacer_error    { ReturnRecordType (GRT_SPACER_ERROR);   }
contact         { ReturnRecordType (GRT_CONTACT);        }

[a-z_]+         { abortScanner("illegal keyword in input: %s", yytext); }

.               { char  buf[5];
                  CharToString(buf, sizeof buf, static_cast<Uchar>(*yytext));
                  abortScanner("illegal character `%s' in input", buf);
                }

<<EOF>>         { return TokEOF; }


%%
//======================================================================

// The record types below are defined in the spec but are not used (the
// `valid' member is false in rectypes.cc).  So there is no point in
// recognizing them as record types.

//      spacing         { ReturnRecordType (GRT_SPACING);        }
//      uinteger        { ReturnRecordType (GRT_UINTEGER);       }
//      ustring         { ReturnRecordType (GRT_USTRING);        }
//      styptable       { ReturnRecordType (GRT_STYPTABLE);      }
//      strtype         { ReturnRecordType (GRT_STRTYPE);        }
//      elkey           { ReturnRecordType (GRT_ELKEY);          }
//      linktype        { ReturnRecordType (GRT_LINKTYPE);       }
//      linkkeys        { ReturnRecordType (GRT_LINKKEYS);       }
//      reserved        { ReturnRecordType (GRT_RESERVED);       }


namespace Gdsii {

using namespace std;
using namespace SoftJin;


// constructor
//   fname      pathname of file being scanned

AsciiScanner::AsciiScanner (const char* fname)
{
    if (fdin >= 0)
        ThrowRuntimeError("GDS ASCII scanner already in use");
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
        case TokEOF:         return "EOF";
        case TokRecordType:  return "record type";
        case TokKeyword:     return "bit-array keyword";
        case TokInteger:     return "integer";
        case TokDouble:      return "floating-point number";
        case TokString:      return "string literal";

        default:
            assert(false);
            return "UNKNOWN";
    }
}



// checkTokenType -- verify that the expected type of token has been read.
//   tokType            the token type just read
//   expectedType       the type we expected to read
// Throws a runtime_error if the types don't match.

/*static*/ inline void
AsciiScanner::checkTokenType (TokenType tokType, TokenType expectedType)
{
    if (tokType == expectedType)
        return;

    if (tokType == TokEOF)
        abortScanner("unexpected EOF");
    abortScanner("expected %s, got %s",
                 TokenTypeName(expectedType), TokenTypeName(tokType));
}



// readRecordType -- read a token and verify that it is a record type.
// Returns true if it reads a record type, false on EOF.
// Throws a runtime_error if the next token in the input is
// not a record type.

bool
AsciiScanner::readRecordType (/*out*/ GdsRecordType* recType)
{
    TokenType   tokType;
    Token       tok;

    if ((tokType = readToken(&tok)) == TokEOF)
        return false;
    checkTokenType(tokType, TokRecordType);
    *recType = tok.recType;
    return true;
}



// readBoundedInteger -- read integer and verify that it is in some range.
// Reads a token and verifies that it is an integer in the range
// minval..maxval.  Throws a runtime_error if the next token in
// the input is not an integer or if it's outside the range specified.

int
AsciiScanner::readBoundedInteger (int minval, int maxval)
{
    Token       tok;

    TokenType  tokType = readToken(&tok);
    checkTokenType(tokType, TokInteger);
    if (tok.ival < minval  ||  tok.ival > maxval) {
        abortScanner("integer %d is outside valid range %d..%d",
                     tok.ival, minval, maxval);
    }
    return tok.ival;
}



// readDouble -- read a double and verify it can be represented in GDSII.
// Reads a token and verifies that it is an integer or a double that can be
// represented as a GDSII 8-byte floating-point number.  If the next token
// is an integer, converts it into a double.  Throws a
// runtime_error if the next token in the input is not an integer
// or a double in the valid range.

double
AsciiScanner::readDouble()
{
    // The smallest positive value that can be represented in a GDSII
    // 8-byte real is 0.0625 * 16**(-64) == 0x0010 0000 0000 0000.  The
    // largest value is (1.0 - 2**(-56)) * 16**63 == 0x7fff ffff ffff ffff.

    const double  DoubleMin  = 5.3976053469340279e-79;
    const double  DoubleMax  = 7.2370055773322622e+75;

    Token       tok;

    TokenType  tokType = readToken(&tok);
    if (tokType == TokInteger)
        tok.dval = static_cast<double>(tok.ival);
    else
        checkTokenType(tokType, TokDouble);
    if ((fabs(tok.dval) < DoubleMin  ||  fabs(tok.dval) > DoubleMax)
            &&  tok.dval != 0.0)
        abortScanner("floating-point value %g is out of range", tok.dval);
    return tok.dval;
}



// readString -- read a string literal and verify that it is not too long.
//   maxlen     the maximum legal length for the string.  The value 0
//              means that the only limit is that of the record length.
//   length     out: the actual length of the string.
// Returns a pointer to the NUL-terminated string.  The string contents
// are owned by the scanner and remain valid only until the next call
// to one of the readXxx() methods.  Throws an exception if the next token
// in the input is not a string literal or if the string is too long.

const char*
AsciiScanner::readString (Uint maxlen, /*out*/ Uint* length)
{
    const Uint  MaxStringLength = 65530;
    Token       tok;

    TokenType  tokType = readToken(&tok);
    checkTokenType(tokType, TokString);
    if (maxlen == 0)
        maxlen = MaxStringLength;
    if (tok.sval.len > maxlen) {
        abortScanner("string \"%s\" is longer than %d characters, "
                     "the maximum allowed in this context",
                     tok.sval.str, maxlen);
    }
    *length = tok.sval.len;
    return tok.sval.str;
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


}  // namespace Gdsii
