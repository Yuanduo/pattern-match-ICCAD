// oasis/compressor.h -- compress and decompress contents of cblock
//
// last modified:   01-Feb-2006  Wed  14:52
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.
//
// OasisWriter and OasisScanner use the classes here to compress and
// decompress cblocks.  Although the OASIS specification defines only
// one compression method, DEFLATE, it says that other methods may be
// defined later.  Hence we define abstract base classes for compression
// and decompression and specify the protocol for invoking their virtual
// methods.


#ifndef OASIS_COMPRESSOR_H_INCLUDED
#define OASIS_COMPRESSOR_H_INCLUDED

#include <string>
#include "misc/globals.h"

struct z_stream_s;      // from <zlib.h>


namespace Oasis {

using std::string;
using SoftJin::Uint;
using SoftJin::Ulong;


// Compressor -- strategy class for creating (compressed) contents of cblock
//
// void beginBlock (const char* ctxt)
//
//      Begin a new compressed block.  The writer must call this
//      before any of the other methods for each cblock.
//          ctxt        context string that should appear as a prefix of
//                      the error message if any method throws an exception.
//                      The compressor copies the input string.
//
//
// bool needInput()
//
//      Returns true iff any input already supplied has been consumed.
//
//
// void giveInput (char* inbuf, Uint inbytes, bool endOfInput)
//
//      Supply input to be compressed or indicate end of input.
//          inbuf       beginning of input data
//          inbytes     bytes of input at inbuf.
//                      May be zero only if endOfInput is true.
//          endOfInput  true => the cblock ends at the end of this input
//
//      Precondition:   needInput()
//                      That is, don't give any new input until the previous
//                      input has been compressed.
//
//      The writer must keep the input unmodified until getOutput()
//      returns 0.
//
//
// Uint getOutput (char* outbuf, Uint outbufSize)
//
//      Compress input data into outbuf and return the number of bytes
//      written.
//          outbuf      output buffer
//          outbufSize  space available in outbuf.
//                      At most this many bytes will be written.
//                      Must be > 0.  Should be a reasonable value; the
//                      compressor may be unable to handle tiny values like 1.
//
//      A single call to getOutput() may not be enough to compress all
//      the input if the output buffer is too small.  The writer should
//      repeatedly call getOutput() until it returns 0.  If getOutput()
//      returns 0 and endOfInput was true in the last call to
//      giveInput(), the output is finished.  The writer must call
//      beginBlock() again to start another block.


class Compressor {
public:
    virtual             ~Compressor();
    virtual void        beginBlock (const char* ctxt) = 0;
    virtual bool        needInput() = 0;
    virtual void        giveInput (char* inbuf, Uint inbytes, bool endOfInput)
                                                                         = 0;
    virtual Uint        getOutput (char* outbuf, Uint outbufSize) = 0;
};



// Decompressor -- strategy class for decompressing contents of cblock
//
// void beginBlock (const char* ctxt)
//
//      Begin decompressing a new cblock.  The scanner must call this
//      before any of the other methods.
//          ctxt          context string that should appear as a prefix of
//                        the error message if any method throws an exception.
//                        The compressor copies the input string.
//
//
// bool needInput()
//
//      Returns true iff any input already supplied has been consumed.
//
//
// void giveInput (char* inbuf, Uint inbytes)
//
//      Supply input to be decompressed.
//          inbuf       beginning of input data
//          inbytes     bytes of input at inbuf.  Must be nonzero.
//
//      Precondition:   needInput() returns true.
//      Postcondition:  needInput() returns false.
//
//      The scanner must call giveInput() before getOutput() and
//      keep the input unmodified until getOutput() returns 0.
//
//
// Uint getOutput (char* outbuf, Uint outbufSize)
//
//      Decompress input data into outbuf and return the number of bytes
//      written.
//          outbuf      output buffer
//          outbufSize  space available in outbuf.  Must be nonzero.
//                      At most this many bytes will be written.
//
//      On exit, needInput() may be either true or false.  It will be
//      true if outbuf is big enough to contain the decompressed output.
//      If getOutput() returns 0 and needInput() is false, the output is
//      finished.  The writer must call beginBlock() again to start
//      another block.
//
//      Each call to getOutput() consumes input, produces output, or
//      both.
//
// For each cblock, the valid sequence of calls for beginBlock(),
// giveInput(), and getOutput() is:
//
//     beginBlock() { giveInput() { getOutput() }* }*
//
// needInput() may be called anytime after beginBlock() and before
// getOutput() returns 0.


class Decompressor {
public:
    virtual             ~Decompressor();
    virtual void        beginBlock (const char* ctxt) = 0;
    virtual bool        needInput() = 0;
    virtual void        giveInput (char* inbuf, Uint inbytes) = 0;
    virtual Uint        getOutput (char* outbuf, Uint outbufSize) = 0;
};


//----------------------------------------------------------------------


// ZlibDeflater -- use zlib to create a cblock with DEFLATE compression.
// To avoid #including zlib.h, we keep a pointer to a z_stream struct
// and not the struct itself.  It is better to localize the inclusion of
// zlib.h because it #defines all sorts of names.
//
// endOfInput   true means that there will be no more input for
//              the current block
// endOfOutput  true means that there will be no more output for
//              the current block (zlib's deflate() returned Z_STREAM_END)
//
// Invariant:  endOfOutput => endOfInput

class ZlibDeflater : public Compressor {
    z_stream_s* zinfo;
    string      context;        // prefix for error messages
    bool        endOfInput;     // no more input bytes
    bool        endOfOutput;    // no more output bytes

public:
                        ZlibDeflater();
    virtual             ~ZlibDeflater();
    virtual void        beginBlock (const char* ctxt);
    virtual bool        needInput();
    virtual void        giveInput (char* inbuf, Uint inbytes, bool endOfInput);
    virtual Uint        getOutput (char* outbuf, Uint outbufSize);

private:
    void                signalZlibError (int zstat, const char* msg);

                        ZlibDeflater (const ZlibDeflater&);     // forbidden
    void                operator= (const ZlibDeflater&);        // forbidden
};



// ZlibInflater -- use zlib to decompress cblock with DEFLATE compression.

class ZlibInflater : public Decompressor {
    z_stream_s* zinfo;
    string      context;        // prefix for error messages

public:
                        ZlibInflater();
    virtual             ~ZlibInflater();
    virtual void        beginBlock (const char* ctxt);
    virtual bool        needInput();
    virtual void        giveInput (char* inbuf, Uint inbytes);
    virtual Uint        getOutput (char* outbuf, Uint outbufSize);

private:
    void                signalZlibError (int zstat, const char* msg);

                        ZlibInflater (const ZlibInflater&);     // forbidden
    void                operator= (const ZlibInflater&);        // forbidden
};


}  // namespace Oasis

#endif  // OASIS_COMPRESSOR_H_INCLUDED
