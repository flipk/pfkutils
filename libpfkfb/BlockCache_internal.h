/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

/** \file BlockCache_internal.h
 * \brief internals of BlockCache implementation */

#include "dll3.h"

class BCB;
typedef DLL3::List<BCB,1,false>  BlockCacheList_t;

/** DLL2 list index identifiers for blocks on lists */
enum BLOCK_CACHE_LIST_INDICES { BLOCK_LIST, BLOCK_NUM_LISTS };

/** A BlockCacheBlock plus linked list information */
class BCB : public BlockCacheBlock,
            public BlockCacheList_t::Links
{
public:
    BCB(off_t _offset, int _size) :
        BlockCacheBlock(_offset, _size) { }
};

/** A list of blocks */
class BlockCacheList {
public:
    /** the linked list of blocks.
     * \see BLOCK_CACHE_LIST_INDICES */
    BlockCacheList_t  list;
};