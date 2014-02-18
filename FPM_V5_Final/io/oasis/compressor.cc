// oasis/compressor.cc -- compress and decompress contents of cblock
//
// last modified:   01-Feb-2006  Wed  14:52
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cassert>
#include <zlib.h>
#include "misc/utils.h"
#include "compressor.h"


namespace Oasis {

using namespace std;
using namespace SoftJin;


/*virtual*/ Compressor::~Compressor() { }
/*virtual*/ Decompressor::~Decompressor() { }



//======================================================================
//                              ZlibDeflater
//======================================================================


ZlibDeflater::ZlibDeflater()
{
    endOfInput = false;
    endOfOutput = false;

    zinfo = new z_stream;
    zinfo->zalloc = Null;
    zinfo->zfree = Null;
    zinfo->next_in = Null;

    // The negative value -15 for windowBits is an undocumented way of
    // telling zlib not to put the RFC-1950 wrapper.

    int  zstat = deflateInit2(zinfo, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                              -15, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (zstat != Z_OK) {
        delete zinfo;
        ThrowRuntimeError("error code %d while initializing zlib compressor",
                          zstat);
    }
}



/*virtual*/
ZlibDeflater::~ZlibDeflater()
{
    deflateEnd(zinfo);
    delete zinfo;
}


// For the specifications of the virtual functions below, see the
// comments for the Compressor class in compressor.h.


/*virtual*/ void
ZlibDeflater::beginBlock (const char* ctxt)
{
    context = ctxt;
    endOfInput = false;
    endOfOutput = false;

    zinfo->next_in = Null;
    zinfo->avail_in = 0;
    int  zstat = deflateReset(zinfo);
    if (zstat != Z_OK)
        signalZlibError(zstat, zinfo->msg);
}



/*virtual*/ bool
ZlibDeflater::needInput()
{
    // We cannot provide more input until the previous input has been
    // used up.

    return (zinfo->avail_in == 0);
}



/*virtual*/ void
ZlibDeflater::giveInput (char* inbuf, Uint inbytes, bool endOfInput)
{
    assert (endOfInput  ||  inbytes > 0);
    assert (needInput());

    // reinterpret_cast is needed because Bytef is unsigned char.
    zinfo->next_in = reinterpret_cast<Bytef*>(inbuf);
    zinfo->avail_in = inbytes;
    this->endOfInput = endOfInput;
}



/*virtual*/ Uint
ZlibDeflater::getOutput (/*out*/ char* outbuf, Uint outbufSize)
{
    assert (outbufSize > 0);

    // Check that there is something to compress.

    if (zinfo->avail_in == 0  &&  !endOfInput)
        return 0;

    // If the last call to deflate() returned Z_STREAM_END, don't call it
    // again; the output is finished.

    if (endOfOutput)
        return 0;

    // Call deflate() to compress as much of the input as possible into
    // outbuf.  One of the return values of deflate() is Z_BUF_ERROR,
    // which means that deflate() can neither consume input nor write
    // output.  Although that is not an error, we do not test for it
    // because it should not happen.  Output should always be possible
    // because outbufSize > 0.  Input should be available unless
    // endOfInput is true.  And if it is, Z_FINISH will be passed to
    // deflate(), causing it to flush its output and write any trailing
    // bytes.

    zinfo->next_out = reinterpret_cast<Bytef*>(outbuf);
    zinfo->avail_out = outbufSize;
    int  zstat = deflate(zinfo, endOfInput ? Z_FINISH : Z_NO_FLUSH);
    if (zstat != Z_OK  &&  zstat != Z_STREAM_END)
        signalZlibError(zstat, zinfo->msg);
    endOfOutput = (zstat == Z_STREAM_END);

    // Return the number of bytes written.  deflate() updates next_out
    // to point to where the next byte would have been written.

    Uint  outbytes = zinfo->next_out - reinterpret_cast<Bytef*>(outbuf);
    assert (outbytes != 0  ||  zinfo->avail_in == 0);
    return outbytes;
}



// signalZlibError -- throw runtime_error because a zlib function failed.
//   zstat      return value of zlib function
//   msg        either Null or the msg field from the zinfo struct,
//              which describes the error

void
ZlibDeflater::signalZlibError (int zstat, const char* msg)
{
    if (msg == Null)
        ThrowRuntimeError("%szlib error %d while compressing CBLOCK",
                          context.c_str(), zstat);
    else
        ThrowRuntimeError("%szlib error %d while compressing CBLOCK: %s",
                          context.c_str(), zstat, msg);
}



//======================================================================
//                              ZlibInflater
//======================================================================

// The ZlibInflater code is similar to the ZlibDeflater code above.  See
// the comments in the ZlibDeflater code for explanations of the
// implementation.  For the specification of the virtual functions, see
// the comments for the Decompressor class in compressor.h


ZlibInflater::ZlibInflater()
{
    zinfo = new z_stream;
    zinfo->zalloc = Null;
    zinfo->zfree = Null;
    zinfo->next_in = Null;
    zinfo->avail_in = 0;

    // The negative value -15 for windowBits is an undocumented way of
    // telling zlib not to expect the RFC-1950 wrapper.

    int  zstat = inflateInit2(zinfo, -15);
    if (zstat != Z_OK) {
        delete zinfo;
        ThrowRuntimeError("error code %d while initializing zlib decompressor",
                          zstat);
    }
}



/*virtual*/
ZlibInflater::~ZlibInflater()
{
    inflateEnd(zinfo);
    delete zinfo;
}



/*virtual*/ void
ZlibInflater::beginBlock (const char* ctxt)
{
    context = ctxt;
    zinfo->next_in = Null;
    zinfo->avail_in = 0;
    int  zstat = inflateReset(zinfo);
    if (zstat != Z_OK)
        signalZlibError(zstat, zinfo->msg);
}



/*virtual*/ bool
ZlibInflater::needInput()
{
    return (zinfo->avail_in == 0);
}



/*virtual*/ void
ZlibInflater::giveInput (char* inbuf, Uint inbytes)
{
    assert (inbytes > 0);
    assert (needInput());

    zinfo->next_in = reinterpret_cast<Bytef*>(inbuf);
    zinfo->avail_in = inbytes;
}



/*virtual*/ Uint
ZlibInflater::getOutput (/*out*/ char* outbuf, Uint outbufSize)
{
    // Note that !needInput() is NOT a precondition.  If the previous
    // call to getOutput() filled the output buffer, this call may
    // produce more output even without being given input.  The code
    // assumes that getOutput() will not be called after the block ends.

    assert (outbufSize > 0);

    zinfo->next_out = reinterpret_cast<Bytef*>(outbuf);
    zinfo->avail_out = outbufSize;
    int  zstat = inflate(zinfo, Z_NO_FLUSH);
    if (zstat != Z_OK  &&  zstat != Z_STREAM_END)
        signalZlibError(zstat, zinfo->msg);

    return (zinfo->next_out - reinterpret_cast<Bytef*>(outbuf));
}



void
ZlibInflater::signalZlibError (int zstat, const char* msg)
{
    if (msg == Null)
        ThrowRuntimeError("%szlib error %d while decompressing CBLOCK",
                          context.c_str(), zstat);
    else
        ThrowRuntimeError("%szlib error %d while decompressing CBLOCK: %s",
                          context.c_str(), zstat, msg);
}


}  // namespace Oasis
