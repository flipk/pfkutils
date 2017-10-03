/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

/** \file Btree.H
 * \brief definition of Btree access methods.
 * \author Phillip F Knaack
 */

#include "FileBlock_iface.H"
#include "bst.H"

class BtreeInternal;

/** helper class for Btree::printinfo.  the user should populate
 * one of these before calling Btree::printinfo to help it decide
 * how to format the data.
 */
class BtreeIterator {
public:
    virtual ~BtreeIterator(void) { /* placeholder */ }
    virtual bool handle_item( UCHAR * keydata, UINT32 keylen,
                              UINT32 data_fbn ) = 0;
    virtual void print( const char * format, ... )
        __attribute__ ((format( printf, 2, 3 ))) = 0;
};

/** The interface to a Btree file with FileBlockInterface backend.
 * The constructor is not public, because the user should not construct
 * these with 'new'.  Instead the user should call Btree::open method,
 * which is static.  This will create a BtreeInternal object and downcast
 * it to the Btree object and return it.  This is to enforce compliance to
 * a simple interface and to minimize the impact on the user's name space.
 */
class Btree {
protected:
    FileBlockInterface * fbi;   /**< this is how Btree accesses the file. */
    /** user not allowed to create Btrees; call Btree::open instead. */
    Btree( void ) { }
    /** internal implementation of file opener; used by
     * openFile and createFile.
     * \param filename the full path to the filename to open.
     * \param max_bytes the number of bytes to allocate for the cache.
     * \param create indicate whether you are trying to create the file
     *         or open an existing file.
     * \param mode the file mode, see open(2); ignored if create==false.
     * \param order if creating the file, this is the btree order of the
     *         new file.  ignored if create==false.
     * \return a pointer to a new Btree object that can be used to access
     *       the file, or NULL if the file could not be created or opened. */
    static Btree * _openFile( const char * filename, int max_bytes,
                              bool create, int mode, int order );
public:
    /** check the file for the required signatures and then
     * return a new Btree object.  If the file does not contain the required
     * signatures, the file cannot be opened, and this function returns NULL.
     * \param fbi the FileBlockInterface used to access the file; this will
     *            be stored inside the Btree object.
     * \return a Btree object if the open was successfull, or NULL if not. */
    static Btree * open( FileBlockInterface * fbi );
    /** take a file without a Btree in it, and add the required signatures.
     * after this, the Btree::open method will be able to open the file.
     * \param fbi the FileBlockInterface to access the file.
     * \param order the desired order of the btree; must be odd, greator
     *              than 1, and less than BtreeInternal::MAX_ORDER.
     * \return true if the init was successful, false if error. */
    static bool init_file( FileBlockInterface * fbi, int order );
    /** open an existing btree file.  this function will open a file
     * descriptor, create a PageIO, create a BlockCache, create a 
     * FileBlockInterface, and then create the Btree.
     * \param filename the existing file to open.
     * \param max_bytes the size of cache to allocate.
     * \return a pointer to a Btree object or NULL if the file could not
     *      be found. */
    static Btree * openFile( const char * filename, int max_bytes ) {
        return _openFile(filename,max_bytes,false,0,0);
    }
    /** create a new btree file.  this function will open a file
     * descriptor, create a PageIO, create a BlockCache, create a 
     * FileBlockInterface, and then create the Btree.
     * \param filename the existing file to open.
     * \param max_bytes the size of cache to allocate.
     * \param mode the file mode during create, see open(2).
     * \param order the order of the Btree to create.
     * \return a pointer to a Btree object or NULL if the file could not
     *      be created. */
    static Btree * createFile( const char * filename, int max_bytes,
                               int mode, int order ) {
        return _openFile(filename,max_bytes,true,mode,order);
    }
    /** check if a file is a valid btree file.
     * \param fbi the FileBlockInterface to access the file.
     * \return true if the file appears to be a valid btree file,
     *              false if not. */
    static bool valid_file( FileBlockInterface * fbi );
    /** Return a pointer to the FileBlockInterface backend.  This allows the
     * user of Btree to store their own data directly in the file without
     * going only through the Btree API.
     * \see BtreeInternal::valid_file
     * \return a pointer to the FileBlockInterface object. */
    FileBlockInterface * get_fbi(void) { return fbi; }
    /** the destructor closes the Btree portion of the file. 
     * \note This destructor ALSO deletes the FileBlockInterface! */
    virtual ~Btree( void ) { }
    /** search the Btree looking for the key. if found, return the FBN id of
     * the corresponding data.  the Btree library does not retrieve the data
     * record itself, only the FBN id of it.
     * \param key pointer to key data.
     * \param keylen length of the key data.
     * \param data_id pointer to the user's variable that will be populated
     *          with the FBN id of the corresponding data.
     * \return true if the data was found (and the 'data_id' parameter will be
     *         populated with the relevant data), or false if the data was 
     *         not found (key not present).*/
    virtual bool get( UCHAR * key, int keylen, UINT32 * data_id ) = 0;
    /** a version of the get method which understands BST data structures.
     * this method will encode the BST bytestream and then call the 
     * pointer/length version of the get method.
     * \param key the key data, in BST form.
     * \param data_id a pointer to the data_fbn that will be returned 
     *      if the search was successful.
     * \return true if the key was found or false if it was not found.
     */
    bool get( BST * key, UINT32 * data_id ) {
        int keylen = 0;
        UCHAR * keybuf = key->bst_encode( &keylen );
        bool retval = false;
        if (keybuf)
        {
            retval = get( keybuf, keylen, data_id );
            delete[] keybuf;
        }
        return retval;
    }
    /** store the key and data into the file.  If the given key already exists
     * in the file, the behavior depends on the replace argument.  If replace
     * is false, this function will return false.  If replace is true, the 
     * old FBN id will be freed from the file and the data_id argument will
     * replace it in the file.
     * \param key pointer to the key data.
     * \param keylen length of the key data.
     * \param data_id FBN id of the data block.
     * \param replace indicate whether to replace (overwrite) or fail.
     * \return true if the item was put in the database; false if the key
     *         already exists and replace argument is false. */
    virtual bool put( UCHAR * key, int keylen, UINT32 data_id,
                      bool replace=false ) = 0;
    /** a version of the put method which supports key in BST form.
     * \param key the key data in BST form.  this key will be encoded into
     *     a temporary buffer before calling the pointer/length form of put.
     * \param data_id the data FBN of the corresponding data
     * \param replace if a matching key is found, this argument indicates
     *      whether it will replace the old data_fbn, or just fail.
     * \return true if put was successful, or false if an error or if the
     *    key already exists and replace==false. */
    bool put( BST * key, UINT32 data_id, bool replace=false ) {
        int keylen = 0;
        UCHAR * keybuf = key->bst_encode( &keylen );
        bool retval = false;
        if (keybuf)
        {
            retval = put( keybuf, keylen, data_id, replace );
            delete[] keybuf;
        }
        return retval;
    }
    /** delete an item from the database given only the key.
     * this function will search for the key/data pair using the key,
     * and then free all disk space associated with the key and its data.
     * \param key  pointer to the key data.
     * \param keylen length of the key data.
     * \return true if the item was deleted, false if not found or error. */
    virtual bool del  ( UCHAR * key, int keylen ) = 0;
    /** a BST version of del.
     * \param key the key data to delete.
     * \return true if the key/data was deleted or false if not. */
    bool del( BST * key ) {
        int keylen = 0;
        UCHAR * keybuf = key->bst_encode( &keylen );
        bool retval = false;
        if (keybuf)
        {
            retval = del( keybuf, keylen );
            delete[] keybuf;
        }
        return retval;
    }
    /** iterate over an entire btree, producing debug data and then
     * calling a user supplied function once for each item in the tree.
     * the tree is walked in key-order.  the user-supplied function must
     * return 'true' if the walk is to continue; if it ever returns 'false',
     * the walk is terminated at its current point.
     * \param bti the user's BtreeIterator object
     * \return true if the entire btree was walked, or false if it was aborted
     *         due to BtreeIterator::handle_item returning false.
     */
    virtual bool iterate( BtreeIterator * bti ) = 0;
};
