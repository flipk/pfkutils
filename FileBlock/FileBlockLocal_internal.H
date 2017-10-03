/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
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
