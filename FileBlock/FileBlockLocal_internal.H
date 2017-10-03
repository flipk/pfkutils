
/** \file FileBlockLocal_internal.H
 * \brief private implementation details of FileBlockLocal
 * \author Phillip F Knaack
 *
 * This file implements private details for the
 *  FileBlockLocal implementation. */


/** \page FileBlockLocalFileFormat FileBlockLocal File Format

\note All 4-byte or 8-byte numeric fields are encoded in big-endian format.

\section FileBlockLocalFileFormatFileHeader File Header

The first few bytes of the first page contains a unique signature
which identifies the file as a FileBlock file.  This signature must be
added when the file is first created.  The contents of the signature
is the ASCII string \em "FILE BLOCK LOCAL" with no trailing NUL (this
consumes 16 bytes).

The size for blocks of extents maps is flexible.  The recommended
value is 32KB; however they must not be more than 64KB in size.

The next 12 bytes contain an 8-byte offset and 4-byte size describing
a portion of the file containing a piece-map, described below.

Following this are 64 4-byte units identifying block ids where file-
information blocks exist.  The value 0 means no information block is
populated in this slot.  The file information block functionality is
not yet designed or implemented.

\todo design and document file-information block system.

\section FileBlockLocalFileFormatFileHeaderPieceMap Piece Map

A piece-map is a block in the file containing offset/size pairs.  Each
offset is an 8-byte value and each size is a 4-byte value.  Each
offset/size pair describes another block of the file containing a piece
of the extent map, described below.

\section FileBlockLocalFileFormatFileHeaderPiece Piece

The extent map entries are encoded 32KB at a time.  First the size of
an extent is encoded as a 4-byte big-endian number.  If the extent is
free, the highest-order bit is cleared.  If the extent is used, the
highest-order bit is set.  Also, if the extent is used, the size is
followed by a 4-byte encoding of the block id.  If the extent is free,
the block id is not present.

The file blocks where the the extent map is stored are themselves
encoded in the extent map.  This must be taken into account when the
extent map is being parsed to produce the file blocks.

\section FileBlockLocalFileFormatExample Example

A data file which has one million used extents and half a million free
extents will consume the following amount of disk space:

<pre>
1,000,000 x 8 bytes per extent = 8,000,000 bytes
  500,000 x 4 bytes per extent = 2,000,000 bytes
10,000,000 / 32768 bytes per piece = 306 pieces
</pre>

While running, it will also consume the following amount of RAM:

<pre>
1,500,000 extent map entries x
   (16 bytes per LListLinks * 4 LListLinks + 8 + 4 + 4) =
1,500,000 x 80 = 120,000,000 bytes
</pre>

(Note this does not count the overhead of the hash table.)

\see FileBlockHeader, PieceMapEntry

 */

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
    UINT64_t  piece_map_start;
    /** length of the first piece of the extent map */
    UINT32_t  piece_map_len;
    /* file info blocks have not yet been designed */
//    UINT32_t  file_info_block_ids[ MAX_FILE_INFO_BLOCKS ];
};

/** this struct appears at FileBlockHeader::piece_map_start position. */
struct PieceMapEntry {
    UINT64_t  offset;
    UINT32_t  len;
};

/** the size of each block allocated to 
 * store pieces of the extent map. */
#define PIECE_SIZE 32768
