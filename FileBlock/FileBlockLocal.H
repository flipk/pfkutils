
/** \file FileBlockLocal.H
 * \brief Defines the FileBlockLocal implementation of FileBlockInterface
 * \author Phillip F Knaack
 */

#ifndef __FILE_BLOCK_LOCAL_H__
#define __FILE_BLOCK_LOCAL_H__

#include "FileBlock_iface.H"
#include "dll2.H"
#include "ExtentMap.H"
/** \page FileBlock FileBlock object

 \section FileBlockExtents FileBlock Extents usage

The FileBlock controls allocation of the body of the file.  It uses an
Extents map to manage in-use and free zones in the file.  The Extents
is stored in a simple format on disk: the Extents is written an entire
page at a time.  Each entry is simply a pair of UINT32 values; the
first is an identifier key, while the second is the block size, with
the top bit reserved.  A value of 1 in the top bit indicates a block
which is in use, while 0 means a free region.  The remaining 31 bits
indicate the size of that particular block.  The file-offset of the
block is implied by summing all the previous extent-map descriptor
sizes.

Obviously, this implies that the on-disk format is not very useful for
actually managing allocations and frees.  When a file is first opened,
all of the extent-map pages are read in, and parsed.  A series of
lists are produced.  Every entry is on a linked list which is sorted
by the offset, in other words, in order through the file.  Every
entry, if a free region, is also on a bucket-list.  There are a
significant number of bucket lists, each corresponding to free regions
of different sizes.

The extent-map is stored on the disk in pieces.  The location of the
first piece is identified in the file-information block.

A flush-operation becomes rather expensive, because the entire linked
list of all extent-map entries must be serially written to the list of
extent-map file pages.  This is the only disadvantage of this
technique.

\section FileBlockBuckets FileBlock Buckets

The list of buckets is configurable by the user when the FileBlock
object is created.  The reason, is that the bucket-list which is
useful varies greatly by application.  If a given application uses
only 3 different block sizes, then a small but specific bucket-list
may be appropriate for speed.  On the other hand if bucket sizes vary
greatly by a large range, the application may define a short list of
ranges comprising the most common bucket sizes.  Or, if it is
important to optimize the size of the file as much as possible, it may
be desirable to define a much larger bucket-list with small increments
between them.  This will be slower to search for appropriate sizes,
but will dramatically improve file space utilization.

Another option which needs investigation, is to arrange the
bucket-list as an indexable array, where the index is derived from a
few bits of the piece size.

\section FileBlockHash FileBlock Hash

Any block which is currently in-use, is also on a hash list.  The hash
key is the unique identifier.

One benefit of the unique identifier key method of retrieval, is that
the file can be compacted by moving entries near the end of the file
to unused holes earlier in the file.  But the block itself is still
retrieved using the same identifier, so that the application storing
data in the file is unaware the blocks have moved to a different
position.

\section FileBlockAccess FileBlock Access Methods

\todo document the access methods.

\section Compacting

\todo document the compaction algorithm.

Next: \ref FileBlockLocalFileFormat

*/

/** \page FileBlockLocalFileFormat FileBlockLocal File Format

\note All 4-byte or 8-byte numeric fields are encoded in big-endian format.

\section FileBlockLocalFileFormatFileHeader File Header

The first few bytes of the first page contains a unique signature
which identifies the file as a FileBlock file.  This signature must be
added when the file is first created.  The contents of the signature
is the ASCII string \em "FILE BLOCK LOCAL" with no trailing NUL (this
consumes 16 bytes).

The size for blocks of extents maps is flexible.  The recommended
value is 8KB; however they must not be more than 64KB in size.

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

The extent map entries are encoded 8KB at a time.  First the size of
an extent is encoded as a 4-byte big-endian number.  If the extent is
free, the highest-order bit is cleared.  If the extent is used, the
highest-order bit is set.  Also, if the extent is used, the size is
followed by a 4-byte encoding of the block id.  If the extent is free,
the block id is not present.

The file blocks where the the extent map is stored are themselves
encoded in the extent map.  This must be taken into account when the
extent map is being parsed to produce the file blocks.

\see FileBlockHeader

 */

/** A "Local File" implementation of FileBlockInterface.
 * 
 * This class talks to a BlockCache to implement file storage.
 * internally it uses an ExtentMap to keep track of the condition
 * of the file.
 * \note The contents of the ExtentMap is stored within the file
 *       itself; the ExtentMap will also have entries marking the 
 *       portions of the file in use where the ExtentMap is stored. */
/** \todo the file info junk must be added here eventually */

class FileBlockLocal : public FileBlockInterface {
    /** the ExtentMap showing used and free portions of the file */
    Extents map;
    /** the interface to read and write the file itself */
    BlockCache * bc;
    /** render the map into on-disk format, alloc space for it
     * from the map, and then write it to bc.  This function is
     * static in the class, so that init_file can call it too.
     * \param m the Extents map to write to disk
     * \param bc the interface to the file to use.
     * \param pos pointer to the position in the file of the first
     *        piece of the old map.  if this parameter is 0, this
     *        means it is called from init_file, thus the file has
     *        no previous map.
     * \param len pointer to the length of the first piece of the
     *        map in the file.  ignored if first_piece_pos is 0. */
    static void store_map( Extents * m, BlockCache * bc,
                           UINT64_t * pos, UINT32_t * len );
public:
    /** Constructor.
     * \param _bc a BlockCache object which will be used to access
     *   the contents of the file */
    FileBlockLocal( BlockCache * _bc );
    /** Destructor.
     * \note This does \b not destroy the BlockCache which was passed in
     *   to the constructor!  The caller must destroy the BlockCache. */
    ~FileBlockLocal( void );

    /** checks for signature etc, returns true if file appears to
     * be valid.  this function is static so that it can be called
     * prior to the constructor.
     * \param bc the interface to the file to check
     * \return true if file has signature, false if it does not */
    static bool is_valid_file( BlockCache * bc );
    /** takes a new file, and writes a signature etc to initialize
     * the file.  this function is static, because it needs to be called
     * before the class can be constructed.
     * \param bc the interface to the file to check. */
    static void init_file( BlockCache * bc );

    /*virtual*/ UINT32 alloc( int size );
    /*virtual*/ void free( UINT32 id );
    /*virtual*/ FileBlock * get_block( UINT32 id, bool for_write = false );
    /*virtual*/ void unlock_block( FileBlock * blk );
    /*virtual*/ void flush(void);
    /*virtual*/ void compact(int time_limit);
};

#endif /* __FILE_BLOCK_LOCAL_H__ */
