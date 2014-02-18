// misc/stringpool.h -- lifetime-based allocator for small strings
//
// last modified:   22-Jul-2004  Thu  22:09
//
// Copyright (c) 2004 SoftJin Infotech Private Ltd.
// This software may be used only under the terms of the SoftJin
// Source License.  See the accompanying file LICENSE for details.

#ifndef MISC_STRINGPOOL_H_INCLUDED
#define MISC_STRINGPOOL_H_INCLUDED

#include <cstddef>              // for size_t
#include "globals.h"


namespace SoftJin {


// StringPool -- fast allocation for groups of strings
// This is the usual lifetime-based arena memory allocator for objects
// that are freed together, specialized for strings.  We don't have to
// worry about memory alignment for strings.
//
// Implementation
//
// StringPool allocates its memory in large blocks and doles out memory
// from these blocks.  The block size (the constructor arg) should be
// much larger than the typical allocation size; otherwise much space
// will be wasted.
//
// The blocks are arranged in a singly-linked list.  The first word of
// each block is a pointer to the next block, or Null for the last
// block.  The second word is a pointer to one past the end of the
// block.  The remaining space in the block is available for allocating
// strings.
//
// firstBlock points to the first block in the chain and currBlock
// points to the block from which we allocate strings.  All blocks from
// firstBlock to the one before currBlock are used.  Blocks from
// currBlock to the end of the chain are available for allocation.
//
// If the client asks for more space than is left in the current block,
// StringPool abandons the remaining free space in the block and moves
// to the next block.  If there is no next block, it allocates a new
// block.  If the client asks for more space than the normal block size
// (the size passed to the constructor), StringPool allocates a special
// block of exactly the requested size (plus the header).  StringPool
// always inserts each new block immediately after the current one.
//
// The size of each block in the list is at least blockSize.


class StringPool {
    struct Block {
        Block*  next;
        char*   end;
    };

private:
    Block*      firstBlock;     // first in chain of blocks
    Block*      currBlock;      // allocate from this block
    char*       nextc;          // first free byte in currBlock
    size_t      blockSize;      // size of normal blocks

public:
    explicit    StringPool (size_t normalBlockSize = 8192);
                ~StringPool();
    char*       alloc (size_t nbytes);
    char*       newString (const char* str);
    char*       newString (const char* str, size_t len);
    void        clear();

private:
    static size_t  getBlockSize (Block* block);
    void        allocNewBlock (size_t size);
    char*       extendAlloc (size_t nbytes);

private:
                StringPool (const StringPool&);         // forbidden
    void        operator= (const StringPool&);          // forbidden
};



// alloc -- allocate space from pool

inline char*
StringPool::alloc (size_t nbytes)
{
    // If there is space in the current block, use it.  Otherwise fall
    // back on the overflow handler, which moves to the next block or
    // adds a new block to the chain, and allocates from that.

    if (nbytes <= static_cast<size_t>(currBlock->end - nextc)) {
        char* retval = nextc;
        nextc += nbytes;
        return retval;
    } else {
        return extendAlloc(nbytes);
    }
}



// clear -- make all space in pool once again available for allocation

inline void
StringPool::clear()
{
    currBlock = firstBlock;
    nextc = reinterpret_cast<char*>(currBlock) + sizeof(Block);
}


}  // namespace SoftJin

#endif  // MISC_STRINGPOOL_H_INCLUDED
