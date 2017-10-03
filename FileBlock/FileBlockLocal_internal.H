
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

/** \file FileBlockLocal_internal.H
 * \brief private implementation details of FileBlockLocal
 * \author Phillip F Knaack
 *
 * This file implements private details for the
 *  FileBlockLocal implementation. */

#include "dll2.H"
#include "types.H"

/** A unique signature stored in the file, so the constructor
 *  can verify a file contains the required structures. */
#define FILE_BLOCK_SIGNATURE  "FILE BLOCK LOCAL"
#define FILE_BLOCK_SIGNATURE_LEN  16

#define MAX_FILE_INFO_BLOCKS 64

#define FILE_BLOCK_HEADER_POSITION ((off_t)0)

/** an internal representation for a FileBlock.
 * this object is used only internally by FileBlockLocal implementation,
 * and adds only linked-list pointers to a FileBlock. */
class FileBlockLocalInt : public FileBlock {
public:
    LListLinks <FileBlockLocalInt> links[1];
    /** constructor.
     * \param _id the unique identifier of this block
     * \param _bcb the BlockCacheBlock in which this block resides. */
    FileBlockLocalInt( UINT32 _id, BlockCacheBlock * _bcb ) {
        id = _id;
        bcb = _bcb;
    }
    ~FileBlockLocalInt(void) { }
    BlockCacheBlock * get_bcb( void ) { return bcb; }
};

/** information about one particular type.
 * the caller provides a unique string to identify it, and we
 * return the unique block id they should fetch. */
struct FileInfoBlockId {
    static const int max_string_len = 128;
    UINT32_t  id;
    char      string[ max_string_len ];
};

/** This struct appears at the beginning of a FileBlockLocal file. 
 * \see \ref FileBlockLocalFileFormat */
struct FileBlockLocalHeader {
    /** This is FILE_BLOCK_SIGNATURE. */
    char signature[ FILE_BLOCK_SIGNATURE_LEN ];
    /** file offset where the extent map starts in the file. */
    UINT64_t  piece_map_start;
    /** unique identifier of the piece map */
    UINT32_t  piece_map_id;
    /** length of the first piece of the extent map */
    UINT32_t  piece_map_len;
    /* file info blocks have not yet been designed */
    UINT32_t  file_info_block_ids[ MAX_FILE_INFO_BLOCKS ];
};

/** this struct appears at FileBlockHeader::piece_map_start position. */
struct PieceMapEntry {
    UINT64_t  offset;
    UINT32_t  id;
    UINT32_t  len;
};

/** the size of each block allocated to 
 * store pieces of the extent map. */
#define PIECE_SIZE 32768
