
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

/** This struct appears at the beginning of a FileBlockLocal file. 
 * \see \ref FileBlockLocalFileFormat */
struct FileBlockHeader {
    /** This is FILE_BLOCK_SIGNATURE. */
    char signature[ FILE_BLOCK_SIGNATURE_LEN ];
    /** file offset where the extent map starts in the file. */
    UINT64_t  extent_map_start;
    /** length of the first piece of the extent map */
    UINT32_t  extent_map_part_len;
    /** file info blocks have not yet been designed */
    UINT32_t  file_info_block_ids[ MAX_FILE_INFO_BLOCKS ];
};

/** the size of each block allocated to 
 * store pieces of the extent map. */
#define PIECE_SIZE 32768
