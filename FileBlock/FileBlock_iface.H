
/** \file FileBlock_iface.H
 * \brief Define FileBlock object and FileBlockInterface pure-virtual
 * interface class.
 * \author Phillip F Knaack
 */

#ifndef __FILE_BLOCK_H__
#define __FILE_BLOCK_H__

#include "BlockCache.H"
#include "types.H"

/** \mainpage FileBlock Interface

The purpose of the FileBlock interface is to manage allocation of file
space within a file.  An excellent analogy to the FileBlockInterface 
object are the standard unix functions \c malloc and \c free, except that
it is for file space rather than memory space.

 \section FileBlockObjects FileBlock component objects

The FileBlock interface is composed of a number of lower-level object
types.  Click on the following links to read about each of them.

<ul>
<li> \ref PageIO (see classes PageIO and PageIOFileDescriptor)
<li> \ref PageCache (see classes PageCachePage and PageCache)
<li> \ref BlockCache (see classes BlockCacheBlock and BlockCache)
<li> \ref ExtentMap (see classes Extent and Extents)
<li> \ref FileBlock (see classes FileBlock, FileBlockInterface, FileBlockLocal)
<li> \ref FileBlockLocalFileFormat
</ul>

*/

/** A data unit in a FileBlockInterface file.
 *
 * When the user of the FileBlockInterface is attempting to access
 * a portion of a file, this is the object which is passed back and
 * forth to the user.  The user is not allowed to construct or delete
 * this object; the only allowed access is via the get and release
 * methods of FileBlockInterface.
 */
class FileBlock {
    /** FileBlocks are identified using a unique 32-bit identifier.
     * 
     * The \em identifier is randomly-generated when a new block is allocated.
     * only the values \b 0 and \b 0xFFFFFFFF are invalid.  If a block is moved
     * around within a file, its identifier remains the same.
     */
    UINT32 id;
    BlockCacheBlock * bcb;
    /** FileBlock constructor does nothing. 
     * \note The constructor is private to prevent users from creating
     *   this object; this object should come from
     *    FileBlockInterface::get_block invocations only. */
     FileBlock(void) { }
    /** FileBlock destructor does nothing. 
     * \note The destructor is private to prevent users from creating
     *    this object; this object should be freed by passing to
     *    FileBlockInterface::unlock_block only. */
    virtual ~FileBlock(void) { }
    friend class FileBlockInterface;
public:
    /** return the identifier of this block.
     * \return the identifier */
    UINT32  get_id    (void) { return id; }
    /** return return the offset in the file where this block starts.
     * \return the offset in the file */
    off_t   get_offset(void) { return bcb->get_offset(); }
    /** return return the size of this block.
     * \return the size of this block */
    int     get_size  (void) { return bcb->get_size  (); }
    /** return a pointer to memory which holds this block's data.
     * \return a pointer to this block */
    UCHAR * get_ptr   (void) { return bcb->get_ptr   (); }
    /** call this if you have modified this block's data */
    void    mark_dirty(void) { bcb->mark_dirty(); }
};

/** A generic interface to read and write FileBlocks.
 *
 * This is a pure-virtual class whose purpose is to abstract the
 * access of files.  
 */
/** \todo We need to implement the information block stuff
 *        in FileBlockInterface */

class FileBlockInterface {
public:
    /** Destructor.
     *
     * This destructor is a placeholder; so that objects derived from
     * this class can implement their own virtual destructors. */
    virtual ~FileBlockInterface(void) { }

    /** allocate a block of space in a file.
     *
     * This function allocates space in the file.
     * \return a unique identifier that can be passed to get_block,
     *   or returns 0 if an error occurred. */
    virtual UINT32 alloc( int size ) = 0;

    /** free a block in a file.
     *
     * frees the part of the file used by a block, so this part of the
     * file can be allocated later.
     * \param id the identifier of the block to free.
     */
    virtual void   free( UINT32 id ) = 0;

    // retrieve an information block from the file
    // identified by some unique name.
//    virtual UINT32 get_data_info_block( char * info_name ) = 0;
//    virtual void   set_data_info_block( UINT32 id, char *info_name ) = 0;
//    virtual void   del_data_info_block( char * info_name ) = 0;

    /** retrieve a block from the file.
     *
     * This function retrieves data from the file and returns a
     * FileBlock to the caller.  If the caller is not interested in
     * reading this block and intends instead to write to it, the 
     * caller may pass for_write=true, which may have cache optimizations
     * to make the access faster.
     * \param id the identifier of the block (returned by alloc())
     * \param for_write set to true, if the caller intends to write to the
     *  buffer, not read it from the file.
     * \return a FileBlock pointer that the caller can use to access
     *  the data in the file. */
    virtual FileBlock * get_block( UINT32 id, bool for_write = false ) = 0;
    /** return a FileBlock back to the file.
     *
     * This function is called to indicate the user is done with a FileBlock
     * that was previously returned by get_block().  If the user has modified
     * the data, the user should call FileBlock::mark_dirty() before calling
     * this function.
     * \param blk pointer to FileBlock previously returned by get_block(). */
    virtual void unlock_block( FileBlock * blk ) = 0;

    /** flush the cache to the disk.
     *
     * This function sychronizes the in-memory copy of any cached blocks or
     * currently outstanding FileBlocks back to the actual file on disk. */
    virtual void flush(void) = 0;

    /** defragment and compact the file.
     *
     * \param time_limit the maximum number of seconds to spend performing
     * defragmentation and compaction.  If this much time passes and the file
     * isn't completely defragmented, this function will give up and return.
     * 0 means continue until the file is completely defragmented.
     */
    virtual void compact(int time_limit) = 0;
};

#endif /* __FILE_BLOCK_H__ */
