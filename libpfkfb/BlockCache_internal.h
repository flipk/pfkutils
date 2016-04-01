/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/** \file BlockCache_internal.h
 * \author Phillip F Knaack
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
