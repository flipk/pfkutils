
/*
    This file is part of the "pkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
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

/** \file BlockCache_internal.H
 * \author Phillip F Knaack
 * \brief internals of BlockCache implementation */

#include "dll2.H"

/** DLL2 list index identifiers for blocks on lists */
enum BLOCK_CACHE_LIST_INDICES { BLOCK_LIST, BLOCK_NUM_LISTS };

/** A BlockCacheBlock plus linked list information */
class BCB : public BlockCacheBlock {
public:
    BCB(off_t _offset, int _size) :
        BlockCacheBlock(_offset, _size) { }
    /** linked list pointers, see BLOCK_CACHE_LIST_INDICES
     * \see BLOCK_CACHE_LIST_INDICES */
    LListLinks <BCB> links[BLOCK_NUM_LISTS];
};

/** A list of blocks */
class BlockCacheList {
public:
    /** the linked list of blocks.
     * \see BLOCK_CACHE_LIST_INDICES */
    LList <BCB,BLOCK_LIST>  list;
};
