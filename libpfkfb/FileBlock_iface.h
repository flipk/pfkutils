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

/** \file FileBlock_iface.h
 * \brief Define FileBlock object and FileBlockInterface pure-virtual
 * interface class.
 */

#ifndef __FILE_BLOCK_H__
#define __FILE_BLOCK_H__

#include <sys/types.h>
#include <inttypes.h>
#include "BlockCache.h"
#include "bst.h"

typedef uint32_t FB_AUID_T;
typedef UINT32_t FB_AUID_t;

/** A data unit in a FileBlockInterface file.
 *
 * When the user of the FileBlockInterface is attempting to access
 * a portion of a file, this is the object which is passed back and
 * forth to the user.  The user is not allowed to construct or delete
 * this object; the only allowed access is via the get and release
 * methods of FileBlockInterface.
 */
class FileBlock {
protected:
    /** FileBlocks are identified using a unique 32-bit identifier.
     * 
     * The \em identifier is randomly-generated when a new block is allocated.
     * only the values \b 0 and \b 0xFFFFFFFF are invalid.  If a block is moved
     * around within a file, its identifier remains the same.
     */
    FB_AUID_T auid;
    BlockCacheBlock * bcb;
    /** FileBlock constructor does nothing. 
     * \note The constructor is private to prevent users from creating
     *   this object; this object should come from
     *    FileBlockInterface::get_block invocations only. */
     FileBlock(void) { }
    /** FileBlock destructor does nothing. 
     * \note The destructor is private to prevent users from creating
     *    this object; this object should be freed by passing to
     *    FileBlockInterface::release only. */
    virtual ~FileBlock(void) { }
    friend class FileBlockInterface;
public:
    /** return the identifier of this block.
     * \return the identifier */
    FB_AUID_T  get_auid  (void) { return auid; }
    /** return return the offset in the file where this block starts.
     * \return the offset in the file */
    off_t   get_offset(void) { return bcb->get_offset(); }
    /** return return the size of this block.
     * \note this does not return the exact size passed to alloc() !
     *       the size value this returns is rounded up to the nearest
     *       internal block boundary, and thus may be up to 127 bytes
     *       larger than what was passed to alloc().
     * \return the size of this block */
    int     get_size  (void) { return bcb->get_size  (); }
    /** return a pointer to memory which holds this block's data.
     * \return a pointer to this block */
    uint8_t * get_ptr   (void) { return bcb->get_ptr   (); }
    /** call this if you have modified this block's data */
    void    mark_dirty(void) { bcb->mark_dirty(); }
};

/** A struct containing statistics for a FileBlock file. */
struct FileBlockStats {
    uint32_t  au_size;       /**< size of an allocation unit */
    uint32_t  used_aus;      /**< number of aus in use */
    uint32_t  free_aus;      /**< number of aus free */
    uint32_t  used_regions;  /**< number of unique regions in use */
    uint32_t  free_regions;  /**< number of free regions (holes) in use */
    uint32_t  num_aus;       /**< file size, in aus */
};

typedef bool (*FileBlockCompactionStatusFunc)(FileBlockStats *stats, void *arg);

/** A generic interface to read and write FileBlocks.
 *
 * This is a pure-virtual class whose purpose is to abstract the
 * access of files.  
 */
class FileBlockInterface {
    /** @name Constructor/Destructor */
    // @{
protected:
    /** the constructor for the base class.
     * this constructor is not public, because users should not
     * be able to 'new' them.  Proper way to get one is using
     * FileBlockInterface::open. */
    FileBlockInterface(void) { }
public:
    /** Destructor.
     * This destructor is a placeholder; so that objects derived from
     * this class can implement their own virtual destructors. */
    virtual ~FileBlockInterface(void) { }
    // @}

    /** @name File open/close methods */
    // @{
    /** checks for signature etc, returns true if file appears to
     * be valid.  this function is static so that it can be called
     * prior to the constructor.
     * \see FileBlockLocal::valid_file
     * \param bc the interface to the file to check
     * \return true if file has signature, false if it does not */
    static bool valid_file( BlockCache * bc );

    /** open a FileBlockLocal object and return it.
     * the purpose of this method is to hide the FileBlockLocal
     * object from the user.
     * \param bc the BlockCache object to access the file
     * \return the newly-created FileBlockLocal object downcast to
     *         the FileBlockInterface type. */
    static FileBlockInterface * open( BlockCache * bc );

    /** takes a new file, and writes a signature etc to initialize
     * the file.  this function is static, because it needs to be called
     * before the class can be constructed.
     * \param bc the interface to the file to check. */
    static void init_file( BlockCache * bc );

    /** backend of openFile/createFile.
     * \param filename full path to file to open
     * \param max_bytes size of the cache.
     * \param create if false, and file does not exist, returns error.
     *      if true, and file already exists or cannot be
     *      created, returns error.
     * \param mode the mode of the file to create, see open(2). ignored if
     *     create==false.
     * \return a pointer to an FBI to use or NULL if error.
     */
    static FileBlockInterface * _openFile( const char * filename,
                                           int max_bytes,
                                           bool create, int mode );
    /** open a file.  this method creates a PageIO, PageCache, 
     * BlockCache, and FileBlockInterface, and returns the latter.
     * \param filename full path to the file to open.
     * \param max_bytes size of the cache.
     * \return pointer to an FBI or NULL if the file couldn't be found
     *     or opened. */
    static FileBlockInterface * openFile( const char * filename,
                                          int max_bytes ) {
        return _openFile( filename, max_bytes, false, 0 );
    }
    /** create a file.  this method creates a PageIO, PageCache, 
     * BlockCache, and FileBlockInterface, and returns the latter.
     * \param filename full path to the file to open.
     * \param max_bytes the size of the cache.
     * \param mode the file mode when creating the file (see open(2)).
     * \return pointer to an FBI or NULL if the file already exists or
     *     cannot be created. */
    static FileBlockInterface * createFile( const char * filename,
                                            int max_bytes, int mode ) {
        return _openFile( filename, max_bytes, true, mode );
    }
    // @}

    /** @name Allocation management */
    // @{
    /** allocate a block of space in a file.
     * \param size the size of the block to allocate.
     * \return a unique identifier that can be passed to get_block,
     *   or returns 0 if an error occurred.
     * \note This function does NOT initialize the block allocated to
     *       Any known values.  If there was old data present in the file
     *       at this position, it will be present in a get operation.
     *
     * This function allocates space in the file.
     */
    virtual FB_AUID_T alloc( int size ) = 0;

    /** free a block in a file.
     * \param auid the identifier of the block to free.
     *
     * frees the part of the file used by a block, so this part of the
     * file can be allocated later.
     */
    virtual void   free( FB_AUID_T auid ) = 0;

    /** change the size of an allocation.
     * \param auid the old block identifier to resize.
     * \param new_size the new size desired.
     * \return the auid of the new block (the same as the ID passed in)
     * \note This function does NOT copy data from the old block to the
     * new block!
     *
     * This will free the old block, and allocate a new block of the new
     * size, but it will retain the same block id.  
     */
    virtual FB_AUID_T realloc( FB_AUID_T auid, int new_size ) = 0;
    // @}

    /** @name File Info Block API
     * retrieve an information block from the file
     * identified by some unique name.
     */
    // @{
    /** provide a unique text string, and return back a FileBlock ID
     * of the info block for that type, if it exists.
     * \param info_name a unique text string up to 128 characters long
     * \return a FileBlock ID of the corresponding info block. */
    virtual FB_AUID_T get_data_info_block( char * info_name ) = 0;
    /** add an association of a string with an info block.
     * \param auid the FileBlock ID of the info block.
     * \param info_name a unique text string up to 128 characters long
     */
    virtual void   set_data_info_block( FB_AUID_T auid, char *info_name ) = 0;
    /** delete an association
     * \param info_name a unique text string up to 128 characters long
     */
    virtual void   del_data_info_block( char * info_name ) = 0;
    // @}

    /** @name Block Access */
    // @{
    /** retrieve a block from the file.
     *
     * This function retrieves data from the file and returns a
     * FileBlock to the caller.  If the caller is not interested in
     * reading this block and intends instead to write to it, the 
     * caller may pass for_write=true, which may have cache optimizations
     * to make the access faster.
     * \param auid the identifier of the block (returned by alloc())
     * \param for_write set to true, if the caller intends to write to the
     *  buffer, not read it from the file.
     * \return a FileBlock pointer that the caller can use to access
     *  the data in the file. */
    virtual FileBlock * get( FB_AUID_T auid, bool for_write = false ) = 0;
    /** return a FileBlock back to the file.
     *
     * This function is called to indicate the user is done with a FileBlock
     * that was previously returned by get_block().  If the user has modified
     * the data, the user should call FileBlock::mark_dirty() before calling
     * this function, or call this function with dirty=true.
     * \param blk pointer to FileBlock previously returned by get_block().
     * \param dirty the caller should indicate if the block was modified.
     *  passing dirty=true is equivalent to calling FileBlock::mark_dirty
     *  prior to releasing. */
    virtual void release( FileBlock * blk, bool dirty=false ) = 0;
    // @}

    /** @name Coherence */
    // @{
    /** flush the cache to the disk.
     *
     * This function sychronizes the in-memory copy of any cached blocks or
     * currently outstanding FileBlocks back to the actual file on disk. */
    virtual void flush(void) = 0;

    /** defragment and compact the file.
     *
     * \param func  A pointer to a user-supplied function which 'compact'
     *          will call with statistics on the ongoing state of the
     *          compaction. The user's function should return true if the
     *          compaction should be continued or false if the compaction
     *          should complete. The user's function may make this decision
     *          based on any goal the user desires, be it percentage of free
     *          space desired, absolute number of regions, amount of time
     *          which has passed, or even the detected presence of more 
     *          file operations required (so that a compaction-during-idle
     *          functionality may be implemented if desired).
     * \param arg  An argument to pass to the user's function.
     */
    virtual void compact(FileBlockCompactionStatusFunc func, void * arg) = 0;
    // @}

    /** @name Statistics */
    // @{
    virtual void get_stats( FileBlockStats * stats ) = 0;
    // @}
};

/** provide a brief way to access a disk block with a type struct.
 * Define any type T using platform-independent data types, such as
 * from types.h.  Then invoke this template and call get(offset), and
 * then reference the data pointer.  Don't worry about releasing, since
 * another get(offset) or deleting this object will cause an automatic
 * release.  You can call the release method manually to force a release,
 * in case you have an ordering requirement. \note Always release or destroy
 * the object first if you plan on freeing the space in the file. Also
 * be sure to call mark_dirty if you have modified the data!
 * \param T the data type to be encapsulated by this template.
 */
template <class T>
struct FileBlockT
{
    /** this object needs to remember the FileBlockInterface object. */
    FileBlockInterface * fbi;
    /** a pointer to the block that was retrieved */
    FileBlock * fb;
    /** constructor takes a BlockCache pointer
     * \param _fbi the FileBlockInterface object to get and put T from */
    FileBlockT(FileBlockInterface * _fbi) {
        fbi = _fbi; fb = NULL; d = NULL;
    }
    /** destructor automatically releases the block back to the file */
    ~FileBlockT(void) { release(); }
    /**
     * allocate space in a file for this type.  it doesn't fetch it
     * though, that is still up to you.
     */
    FB_AUID_T alloc(void) { return fbi->alloc(sizeof(T)); }
    /** fetch from the file; automatically determines size of T.
     * \param id the unique identifier of the block.
     * \param for_write set to true if you don't care what T's previous
     *        value in the file was and you're overwriting it from scratch
     * \return true if the block was fetched OK and false if not. */
    bool get( FB_AUID_T id, bool for_write = false ) {
        release();
        fb = fbi->get( id, for_write );
        if (!fb) return false;
        d = (T *) fb->get_ptr();
        return true;
    }
    /** release the block back to the file */
    void release(void) {
        if (fb) fbi->release(fb);
        fb = NULL;
        d = NULL;
    }
    /** mark the block as dirty if you modified it */
    void mark_dirty(void) { if (fb) fb->mark_dirty(); }
    /** a pointer to the actual data */
    T * d;
};

#include "bst.h"
#include <signal.h>

struct BST_FB_AUID_t : public BST_UINT32_t {
    BST_FB_AUID_t(BST *parent) : BST_UINT32_t(parent) { }
};

/** A base class for a data type which uses the ByteSTream interface.
 */
class FileBlockBST : public BST {
    /** this object needs to remember the FileBlockInterface object. */
    FileBlockInterface * fbi;
    FB_AUID_T id;
    /** set a reminder to ourselves indicating if we need to free something. */
    bool meminuse;
public:
    FileBlockBST(BST *parent, FileBlockInterface * _fbi) : BST(parent) {
        fbi = _fbi;
        id = 0;
        meminuse = false;
    }
    /** destructor for this object.  it is illegal to delete it 
     * without calling bst_free. the destructor will throw an error. */
    ~FileBlockBST(void) {
        if (meminuse) {
            fprintf(stderr,"ERROR: types derived from FileBlockBST must "
                    "call bst_free in their destructors!\n");
            kill(0,SIGABRT);
        }
    }
    /**  fetch an object from the file and decode it.
     * \param _id   the AUID of the block in the file to fetch.
     * \return  true if the block was retrieved and decoded ok,
     *      or false if there was an error.
     */
    bool get( FB_AUID_T _id ) {
        bst_free();
        id = _id;
        FileBlock * fb = fbi->get(id);
        if (!fb)
            return false;
        BST_STREAM_BUFFER str(fb->get_ptr(), fb->get_size());
        str.start(BST_OP_DECODE);
        bool retval = bst_op(&str);
        fbi->release(fb,false);
        if (retval)
            meminuse = true;
        return retval;
    }
private:
    /**  store an object into the file.
     * \param newblock   set this to false if this object already
     *      corresponds to an existing region of the file.  set to
     *      true to force an alloc of a new AUID.
     * \return  true if it was completed without error.
     */
    bool put( bool newblock ) {
        if (id == 0 && newblock == false)
            return false;
        BST_STREAM_BUFFER str(NULL,65536);
        str.start(BST_OP_CALC_SIZE);
        if (bst_op(&str) == false)
            return false;
        int size = str.get_finished_size();
        if (newblock)
        {
            id = fbi->alloc(size);
            if (id == 0)
                return false;
        }
        else
        {
            fbi->realloc(id,size);
        }
        FileBlock * fb = fbi->get(id,true);
        if (!fb)
            return false;
        str.start(BST_OP_ENCODE,fb->get_ptr(),fb->get_size());
        bool retval = bst_op(&str);
        fbi->release(fb,true);
        meminuse = true;
        return retval;
    }
public:
    /** accessor to tell user if this object corresponds to a real
     * block in the file.
     * \return true if there is a corresponding block.
     */
    bool idvalid(void) { return id!=0; }
    /** return the FileBlockInterface.
     * \return the FileBlockInterface.
     */
    FileBlockInterface * get_fbi(void) { return fbi; }
    /** return the AUID of the block in the file.
     * \return the AUID of the block in the file.
     */
    FB_AUID_T get_id(void) { return id; }
    /** store the object into an existing AUID. will return an error
     * if there is no corresponding block yet.
     * \return true if stored in the file with no error.
     */
    bool put( void ) { return put(false); }
    /** allocate a new block in the file and store this object.
     * \param retid   the AUID of the block that was allocated for this object.
     * \return true if it was stored in the file with no error.
     */
    bool putnew( FB_AUID_T * retid ) { 
        bool ret = put(true);
        if (ret)
            *retid = id;
        return ret;
    }
    /**  free up all memory associated with this object.
     * cannot be automatic in the destructor because of the heirarchical
     * nature of how objects are destroyed.
     * \note this overrides BST::bst_free so that we can get our
     * own meminuse tracking.
     */
    void bst_free( void ) {
        BST_STREAM_BUFFER str(NULL,0);
        str.start(BST_OP_MEM_FREE);
        bst_op(&str);
        meminuse = false;
        id = 0;
    }
};

/** \page FileBlockBSTExample Examples of FileBlockBST usage

This is an example of using a FileBlockBST data type:

<pre>
class CrapUnion : public BST_UNION {
public:
    enum { ONE, TWO, MAX };
    CrapUnion(void) : BST_UNION(MAX) { }
    ~CrapUnion(void) { bst_free(); }
    BST_UINT32_t   one;
    BST_STRING     two;
    / * virtual * / bool bst_op( BST_STREAM *str ) {
        BST * fields[] = { &one, &two };
        return bst_do_union(str, fields);
    }
};

class CrapType : public FileBlockBST {
public:
    CrapType(FileBlockInterface * _fbi) : FileBlockBST(_fbi) { }
    ~CrapType(void) { bst_free(); }
    BST_UINT32_t one;
    BST_UINT32_t two;
    CrapUnion un;
    / * virtual * / bool bst_op( BST_STREAM *str ) {
        BST * fields[] = { &one, &two, &un, NULL };
        return bst_do_fields( str, fields );
    }
};
</pre>

*/

#endif /* __FILE_BLOCK_H__ */
