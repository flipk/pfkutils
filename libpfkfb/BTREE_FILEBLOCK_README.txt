
FileBlock and Btree API Primer
------------------------------

The FileBlock API can be used on its own, or with Btree.  Since the majority
of use cases I know about will use Btree, I'll describe that API first.

DATA TYPES
----------

FB_AUID_T - this is a file block allocation unit identifer. it is a 32-bit
            number which is a 'handle' to some block of storage in the file.
	    the exact location of this block of storage is hidden from the
	    user, and may move (for instance if the 'compact' method is run
	    to consolidate free space).  the AUID of a block does not change
	    if the block is moved.

btree key (uint8_t*,int len) - a key blob is a unique identifier. you can store
            anything you want in this blob. many different types of
	    data may be stored in a btree, as long as the raw bytes of
	    the key blob are unique.
	    NOTE: structs are dangerous to use for keys, because the
	    compiler inserts padding between members to maintain
	    alignment.  the Btree API doesn't understand member types,
	    only raw bytes. if padding is populated with uninitialized
	    random data during a btree-put, then it may become
	    impossible to retrieve the data just put.  you can address
	    this in one of several ways:
	        - use __attribute__((packed)) on the struct definition
		- memset the struct to 0 before populating members
		- use a serializing API to build the key buffer, such as
		  protobufs, flatbuffers, or the libpfkutil BST API.

BTREE API
---------

the constructor for the "Btree" class is private. instead, use static
methods 'createFile' (to create a new file) or 'openFile' (to open an
existing file) to create a Btree object.  when done with the Btree,
you may use 'delete' to destroy the Btree object.  upon delete, Btree
will flush the PageCache and close the file cleanly.

if you wish the file to be encrypted, pass the 'filename' as
'filename:password'.  The colon separates the filename and password.

note that construction of a Btree object also constructs underlying
FileBlock, BlockCache, PageCache, and PageIO objects.

the Btree destructor performs a compaction on the data file and then
cleanly destroys all the underlying objects.

the underlying PageCache has a limit on how much memory it will allocate
for cache.  you specify that limit using the 'max_bytes' parameter.
minimum is one page (4K). the maximum is all available ram.

the btree maps key blobs to 32-bit data items. you may use this 32-bit
storage for anything you want, as the Btree API simply stores and
retrieves it. interpreting it is left to your application. a
convenient thing to put in this storage is a file block allocation
unit ID (FB_AUID_T).  see the section below on the FileBlock API.

when a btree file is created, the 'order' must be specified, and must
be an odd number.  this is the max number of data items in each node,
and logarithmically affects the height of the resulting tree. for
example, a btree of order 9 with a hundred million records would be a
tree between 10 and 13 levels deep, while an order 25 tree with a
hundred million records would be 7 to 9 levels deep.


    /** create a new btree file.  this function will open a file
     * descriptor, create a PageIO, create a BlockCache, create a 
     * FileBlockInterface, and then create the Btree.
     * \param filename the existing file to open.
     * \param max_bytes the size of cache to allocate.
     * \param mode the file mode during create, see open(2).
     * \param order the order of the Btree to create.
     * \return a pointer to a Btree object or NULL if the file could not
     *      be created. */
static Btree * Btree::createFile( const char * filename, int max_bytes,
                                  int mode, int order )

    /** open an existing btree file.  this function will open a file
     * descriptor, create a PageIO, create a BlockCache, create a 
     * FileBlockInterface, and then create the Btree.
     * \param filename the existing file to open.
     * \param max_bytes the size of cache to allocate.
     * \return a pointer to a Btree object or NULL if the file could not
     *      be found. */
static Btree * Btree::openFile( const char * filename, int max_bytes )

    /** store the key and data into the file.  If the given key already exists
     * in the file, the behavior depends on the replace argument.  If replace
     * is false, this function will return false.  If replace is true, the 
     * old data will be put into *old_data_id, and *replaced will be set 
     * to true.  this API does not make any assumptions about the meaning
     * of the data_id or old_data_id-- the user may specify any 32 bit data
     * which is meaningful to the application.  the assumption is that the
     * most common usage is to place FileBlockInterface FBN numbers in this
     * field.  in this case, when a data is replaced, the disposition of the
     * old FBN is left to the user, for example to call
     * get_fbi()->free(old_data_id) or place the old_data_id in some other
     * data structure if the data is still relevant.
     * \param key pointer to the key data.
     * \param keylen length of the key data.
     * \param data_id user data to associate with the key.
     * \param replace indicate whether to replace (overwrite) or fail.
     * \param replaced a pointer to a user's bool; if a matching key was
     *     found in the btree, the corresponding data will be returned, the
     *     new data_id will replace it, and *replaced will be set to true.
     *     if the key was new in the database, *replaced will be set to false.
     * \param old_data_id a pointer to a user's variable.  if *replaced is
     *     set to true (see the replaced param above) then *old_data_id will
     *     contain the old data associated with the key prior to replacement.
     * \return true if the item was put in the database; false if the key
     *         already exists and replace argument is false. */
bool Btree::put( uint8_t * key, int keylen, FB_AUID_T data_id,
                  bool replace=false, bool *replaced=NULL,
                  FB_AUID_T *old_data_id=NULL )

    /** search the Btree looking for the key. if found, return the data field
     * associated with this key.
     * \param key pointer to key data.
     * \param keylen length of the key data.
     * \param data_id pointer to the user's variable that will be populated
     *          with the data corresponding to the key.  this data will
     *          exactly match the data specified to the put method when
     *          this key was put in the database.  it can be an FBN id,
     *          but the Btree API makes no assumptions about it.
     * \return true if the data was found (and the 'data_id' parameter will be
     *         populated with the relevant data), or false if the data was 
     *         not found (key not present).*/
bool Btree::get( uint8_t * key, int keylen, FB_AUID_T * data_id )

    /** delete an item from the database given only the key.
     * this function will search for the key/data pair using the key,
     * and then free all disk space associated with the key. if the data
     * is a file block ID, this API does not know or assume that, and does NOT
     * free the associated data block.
     * \param key  pointer to the key data.
     * \param keylen length of the key data.
     * \param old_data_id if the matching key was found, and the record
     *    is deleted, *old_data_id will be populated with the data value
     *    that was present in the btree.  the disposition of this data is
     *    left to the user, i.e. if it was being used to store FBN numbers,
     *    presumably the user will wish to free the associated file block.
     * \return true if the item was deleted, false if not found or error. */
bool Btree::del  ( uint8_t * key, int keylen, FB_AUID_T *old_data_id )

    /** iterate over an entire btree, producing debug data and then
     * calling a user supplied function once for each item in the tree.
     * the tree is walked in key-order.  the user-supplied function must
     * return 'true' if the walk is to continue; if it ever returns 'false',
     * the walk is terminated at its current point.
     * \param bti the user's BtreeIterator object
     * \return true if the entire btree was walked, or false if it was aborted
     *         due to BtreeIterator::handle_item returning false. */
bool Btree::iterate( BtreeIterator * bti )

    /** Return a pointer to the FileBlockInterface backend.  This allows the
     * user of Btree to store their own data directly in the file without
     * going only through the Btree API. do NOT delete this object. if the
     * Btree object is deleted, this FileBlockInterface will be deleted as
     * well.
     * \return a pointer to the FileBlockInterface object. */
FileBlockInterface * Btree::get_fbi(void)

FileBlockInterface API
----------------------

the FileBlock API may be used on its own, or with Btree.  like Btree,
the FileBlockInterface constructor is private.  if FileBlock is being
used on its own, you must use the static 'createFile' and/or
'openFile' methods found in the FileBlockInterface class.

if the user is using the Btree API, the FileBlockInterface::createFile
and FileBlockInterface::openFile methods are not used and not needed.
use Btree::createFile and Btree::openFile instead.

every block of storage in the file is identified with a unique
identifier, FB_AUID_T (file block allocation unit ID).  a block can be
any size from 1 byte to just under 512k.  (note at this time sizes
over 64k are very lightly tested.)

blocks are allocated and freed using an 'alloc' and 'free' method.

data transfer to and from the file is done thru an intermediary type
'FileBlock'.  the 'get' method is used to obtain a FileBlock, and the
memory referenced by the FileBlock is then accessed, and the 'release'
method is used to return the FileBlock to the file. if the user wishes
to read from the file, this is done without ever specifying a 'dirty'
flag.  if the user modified the FileBlock memory, the user must either
call FileBlock::mark_dirty(), or pass dirty=true to 'release'.



    /** create a file.  this method creates a PageIO, PageCache, 
     * BlockCache, and FileBlockInterface, and returns the latter.
     * \param filename full path to the file to open.
     * \param max_bytes the size of the cache.
     * \param mode the file mode when creating the file (see open(2)).
     * \return pointer to an FBI or NULL if the file already exists or
     *     cannot be created. */
static FileBlockInterface * 
       FileBlockInterface :: createFile( const char * filename,
                                         int max_bytes, int mode )

    /** open a file.  this method creates a PageIO, PageCache, 
     * BlockCache, and FileBlockInterface, and returns the latter.
     * \param filename full path to the file to open.
     * \param max_bytes size of the cache.
     * \return pointer to an FBI or NULL if the file couldn't be found
     *     or opened. */
static FileBlockInterface *
       FileBlockInterface :: openFile( const char * filename,
                                       int max_bytes )

    /** allocate a block of space in a file.
     * \param size the size of the block to allocate.
     * \return a unique identifier that can be passed to get_block,
     *   or returns 0 if an error occurred.
     * \note This function does NOT initialize the block allocated to
     *       Any known values.  If there was old data present in the file
     *       at this position, it will be present in a get operation.
     * This function allocates space in the file. */
FB_AUID_T FileBlockInterface::alloc( int size )

    /** free a block in a file.
     * \param auid the identifier of the block to free.
     * frees the part of the file used by a block, so this part of the
     * file can be allocated later. */
void   FileBlockInterface::free( FB_AUID_T auid )

    /** change the size of an allocation.
     * \param auid the old block identifier to resize.
     * \param new_size the new size desired.
     * \return the auid of the new block (the same as the ID passed in)
     * \note This function does NOT copy data from the old block to the
     * new block!
     * This will free the old block, and allocate a new block of the new
     * size, but retain the same block id. */
FB_AUID_T FileBlockInterface::realloc( FB_AUID_T auid, int new_size )

    /** retrieve a block from the file.
     * This function retrieves data from the file and returns a
     * FileBlock to the caller.  If the caller is not interested in
     * reading this block and intends instead to write to it, the 
     * caller may pass for_write=true, which may have cache optimizations
     * to make the access faster.
     * \param auid the identifier of the block (returned by alloc())
     * NOTE: "for_write" does NOT mean "give me permission to modify
     * this block."  you may modify the contents any FileBlock at any time,
     * as long as you mark it as "dirty" (using FileBlock::mark_dirty() or
     * pass dirty=true to 'release').  "for_write" simply means "don't
     * bother going to the file to read this block, because I know I am
     * going to overwrite the entire contents anway, and I don't care
     * what was in this block before."
     * \param for_write set to true, if the caller intends to write to the
     *  buffer, not read it from the file.
     * \return a FileBlock pointer that the caller can use to access
     *  the data in the file. */
FileBlock * FileBlockInterface::get( FB_AUID_T auid,
                                     bool for_write = false )

    /** return a FileBlock back to the file.
     * This function is called to indicate the user is done with a FileBlock
     * that was previously returned by get_block().  If the user has modified
     * the data, the user should call FileBlock::mark_dirty() before calling
     * this function, or call this function with dirty=true.
     * \param blk pointer to FileBlock previously returned by get_block().
     * \param dirty the caller should indicate if the block was modified.
     *  passing dirty=true is equivalent to calling FileBlock::mark_dirty
     *  prior to releasing. */
void FileBlockInterface::release( FileBlock * blk, bool dirty=false )

    /** flush the cache to the disk.
     * This function sychronizes the in-memory copy of any cached blocks or
     * currently outstanding FileBlocks back to the actual file on disk. */
void FileBlockInterface::flush(void)

    /** defragment and compact the file.
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
     * \param arg  An argument to pass to the user's function. */
void FileBlockInterface::compact(FileBlockCompactionStatusFunc func, void * arg)

FILEBLOCK API
-------------

a FileBlock is obtained through FileBlockInterface::get() and released
through FileBlockInterface::release(). FileBlock has the following methods:

    /** return a pointer to memory which holds this block's data.
     * \return a pointer to this block */
uint8_t * get_ptr   (void);

    /** return return the size of this block.
     * NOTE: this does not return the exact size passed to alloc() !
     *       the size value this returns is rounded up to the nearest
     *       internal block boundary, and thus may be up to 127 bytes
     *       larger than what was passed to alloc().
     * \return the size of this block */
int     get_size  (void);

    /** return the identifier of this block.
     * \return the identifier */
FB_AUID_T  get_auid  (void);

    /** call this if you have modified this block's data */
void    mark_dirty(void);


BST
---

there is a simplistic method for encoding a struct into a byte stream,
called BST.  it is modeled after Sun's XDR format (used in Sun RPC and NFS).

FileBlock and Btree have BST convenience methods.  it is not necessary to
use them, they are provided only for convenience.

TODO
----
document BST interface, FileBlockBST convenience methods, and Btree BST usage.
