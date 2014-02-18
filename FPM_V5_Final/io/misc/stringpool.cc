// misc/stringpool.cc -- lifetime-based allocator for small strings
//
// last modified:   19-Jul-2004  Mon  23:53
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#include <cassert>
#include <cstring>
#include "stringpool.h"


namespace SoftJin {

using std::memcpy;


StringPool::StringPool (size_t normalBlockSize)
{
    assert (normalBlockSize >= 256);    // should be reasonably large
    blockSize = normalBlockSize;
    firstBlock = currBlock = Null;
    allocNewBlock(blockSize);
}



// allocNewBlock -- allocate new block and insert after current one (if any).
//   size       size of block to allocate
// Postconditions:
//   currBlock points to newly allocated block
//   next == currBlock + sizeof(Block) bytes

void
StringPool::allocNewBlock (size_t size)
{
    char*  buf = new char[size];
    nextc = buf + sizeof(Block);

    Block*  newBlock = reinterpret_cast<Block*>(buf);
    newBlock->end = buf + size;

    if (currBlock == Null) {                    // first allocation?
        assert (firstBlock == Null);
        newBlock->next = Null;
        firstBlock = newBlock;
    } else {
        newBlock->next = currBlock->next;
        currBlock->next = newBlock;
    }
    currBlock = newBlock;
}



StringPool::~StringPool()
{
    // Go down the chain of blocks, deleting each.

    Block*  b = firstBlock;
    while (b != Null) {
        Block*  next = b->next;
        delete[] reinterpret_cast<char*>(b);
        b = next;
    }
}



/*static*/ inline size_t
StringPool::getBlockSize (Block* block) {
    return (block->end - reinterpret_cast<char*>(block));
}



// extendAlloc -- allocate from next block in chain, adding one if needed.
//   nbytes     space wanted by application
// alloc() calls this when there is not enough space in the current
// block to satisfy the allocation request.

char*
StringPool::extendAlloc (size_t nbytes)
{
    // If there is a next block in the chain and it is big enough, move
    // to it.  Otherwise insert a new block after the current one.

    size_t  size = nbytes + sizeof(Block);
    if (currBlock->next != Null  &&  size <= getBlockSize(currBlock->next)) {
        currBlock = currBlock->next;
        nextc = reinterpret_cast<char*>(currBlock) + sizeof(Block);
    } else {
        // Create a normal-size block if that is enough.  Otherwise
        // create a special block of exactly the size needed.
        //
        if (size < blockSize)
            size = blockSize;
        allocNewBlock(size);
    }
    assert (nbytes <= static_cast<size_t>(currBlock->end - nextc));

    // Allocate from the now-current block, which is guaranteed to have
    // enough free space to satisfy the request.  This recursive call to
    // alloc() will therefore succeed immediately; it won't cause an
    // infinite loop.

    return (alloc(nbytes));
}



// newString -- make a copy of the argument string in the pool
// Returns a pointer to the copy.

char*
StringPool::newString (const char* str)
{
    size_t  len = strlen(str);
    char*  copy = alloc(len+1);
    memcpy(copy, str, len+1);
    return copy;
}



// newString -- make a copy of the argument string in the pool
//   str        string to copy; need not be NUL-terminated
//   len        length of string
// newString() NUL-terminates the copy and returns a pointer to it.

char*
StringPool::newString (const char* str, size_t len)
{
    char*  copy = alloc(len+1);
    memcpy(copy, str, len);
    copy[len] = Cnul;
    return copy;
}


}  // namespace SoftJin
