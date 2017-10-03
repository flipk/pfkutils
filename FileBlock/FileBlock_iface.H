
/** \file FileBlock_iface.H
 * \brief Define FileBlock object and FileBlockInterface pure-virtual
 * interface class.
 * \author Phillip F Knaack
 */

#ifndef __FILE_BLOCK_H__
#define __FILE_BLOCK_H__

#include "BlockCache.H"
#include "types.H"

/** \brief A data unit in a FileBlockInterface file.
 *
 * When the user of the FileBlockInterface is attempting to access
 * a portion of a file, this is the object which is passed back and
 * forth to the user.  The user is not allowed to construct or delete
 * this object; the only allowed access is via the get and release
 * methods of FileBlockInterface.
 */
class FileBlock {
    /** \brief FileBlocks are identified using a unique 32-bit identifier.
     * 
     * The \em identifier is randomly-generated when a new block is allocated.
     * only the values \b 0 and \b 0xFFFFFFFF are invalid.  If a block is moved
     * around within a file, its identifier remains the same.
     */
    UINT32 id;
    BlockCacheBlock * bcb;
    /** \brief FileBlock constructor does nothing. 
     * \note The constructor is private to prevent users from creating
     *   this object; this object should come from
     *    FileBlockInterface::get_block invocations only. */
     FileBlock(void) { }
    /** \brief FileBlock destructor does nothing. 
     * \note The destructor is private to prevent users from creating
     *    this object; this object should be freed by passing to
     *    FileBlockInterface::unlock_block only. */
    virtual ~FileBlock(void) { }
    friend class FileBlockInterface;
public:
    /** \brief return the identifier of this block.
     * \return the identifier */
    UINT32  get_id    (void) { return id; }
    /** \brief return return the offset in the file where this block starts.
     * \return the offset in the file */
    off_t   get_offset(void) { return bcb->get_offset(); }
    /** \brief return return the size of this block 
     * \return the size of this block */
    int     get_size  (void) { return bcb->get_size  (); }
    /** \brief return a pointer to memory which holds this block's data 
     * \return a pointer to this block */
    UCHAR * get_ptr   (void) { return bcb->get_ptr   (); }
    /** \brief call this if you have modified this block's data */
    void    mark_dirty(void) { bcb->mark_dirty(); }
};

/** \brief A generic interface to read and write FileBlocks.
 *
 * This is a pure-virtual class whose purpose is to abstract the
 * access of files.  
 */
/** \todo We need to implement the information block stuff
 *        in FileBlockInterface */

class FileBlockInterface {
public:
    /** \brief Destructor
     *
     * This destructor is a placeholder; so that objects derived from
     * this class can implement their own virtual destructors. */
    virtual ~FileBlockInterface(void) { }

    /** \brief allocate a block of space in a file.
     *
     * This function allocates space in the file.
     * \return a unique identifier that can be passed to get_block,
     *   or returns 0 if an error occurred. */
    virtual UINT32 alloc( int size ) = 0;

    /** \brief free a block in a file
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

    /** \brief retrieve a block from the file
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
    /** \brief return a FileBlock back to the file
     *
     * This function is called to indicate the user is done with a FileBlock
     * that was previously returned by get_block().  If the user has modified
     * the data, the user should call FileBlock::mark_dirty() before calling
     * this function.
     * \param blk pointer to FileBlock previously returned by get_block(). */
    virtual void unlock_block( FileBlock * blk ) = 0;

    /** \brief flush the cache to the disk
     *
     * This function sychronizes the in-memory copy of any cached blocks or
     * currently outstanding FileBlocks back to the actual file on disk. */
    virtual void flush(void) = 0;

    /** \brief defragment and compact the file
     *
     * \param time_limit the maximum number of seconds to spend performing
     * defragmentation and compaction.  If this much time passes and the file
     * isn't completely defragmented, this function will give up and return.
     * 0 means continue until the file is completely defragmented.
     */
    virtual void compact(int time_limit) = 0;
};

#endif /* __FILE_BLOCK_H__ */
