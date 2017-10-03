
/** \file Btree.H
 * \brief definition of Btree access methods.
 * \author Phillip F Knaack
 */

#include "FileBlock_iface.H"

class _BTDatum;
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
    /** the destructor closes the Btree portion of the file, but does NOT 
     * delete the FileBlockInterface backend--the caller is
     * responsible for that. */
    virtual ~Btree( void ) { }
    /** search the Btree looking for the key, and populate the data if found.
     * once the data has been returned, the user is free to modify the data
     * at will, as long as the data does not change size.  If the data must
     * change size, the user must delete the item and put a new one.  If the
     * data is modified, always remember to call mark_dirty before releasing.
     * \param key a BTDatum object which is populated with a ram-pointer and
     *            the data to search for.
     * \param data a BDatum object which will, upon return, contain the data
     *             that was being searched for.  The date of this object prior
     *             to calling is irrelevant--it will be reinitialized before
     *             return.
     * \return true if the data was found (and the 'data' parameter will be
     *         populated with the relevant data), or false if the data was 
     *         not found (key not present).  if this function returns true,
     *         both key and data will now point to the FileBlock records for
     *         the key and data in the file.  if it returns false, neither
     *         object is modified. */
    virtual bool get( _BTDatum * key, _BTDatum * data ) = 0;
    /** store the key and data into the file.  If the given key already exists
     * in the file, the behavior depends on the replace argument.  If replace
     * is false, this function will return false.  If replace is true, the 
     * old data will be freed from the file and the data argument will replace
     * it in the file.
     * \note This function should /em only be used for /em new items;
     * it should not be used to replace existing items.
     * \param key the BDatum object with key data.
     * \param data the BDatum object with data.
     * \param replace indicate whether to replace (overwrite) or fail.
     * \return true if the item was put in the database; false if the key
     *         already exists and replace argument is false. */
    virtual bool put( _BTDatum * key, _BTDatum * data, bool replace=false ) = 0;
    /** delete an item from the database given only the key.
     * this function will search for the key/data pair using the key,
     * and then free all disk space associated with the key and its data.
     * \param key  the key to locate the item that should be deleted.
     * \return true if the item was deleted, false if not found or error. */
    virtual bool del  ( _BTDatum * key  ) = 0;
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

/** a base class for a BTREE unit of data (key or contents).
 * this is a pure virtual class which represents a unit of data.
 * the unit of data can be either a key or the data contents.
 * the unit of data can either be an ordinary heap-based object,
 * or it can be pointing directly into a FileBlock object.
 * it is intended to be used with the BTDatum template below.
 * the Btree object only knows this type, it does not know any
 * derived types, and will access all of the virtual methods.
 */
class _BTDatum {
protected:    
    Btree * bt;       /**< the btree this object belongs to */
    FileBlock * fb;   /**< the block on disk which holds this data */
public:
    /** the constructor takes the btree pointer, and inits FileBlock
     * pointer to NULL. */
    _BTDatum(Btree * _bt) { bt = _bt; fb = NULL; }
    /** the destructor is empty and virtual just in case the derived
     * objects needs cleanup. */
    virtual ~_BTDatum(void) { /*placeholder*/ }
    /** if we have created or found a FileBlock for this data,
     * Btree will call this to assign the FileBlock pointer. */
    virtual void  setfb (FileBlock *) = 0;
    /** this returns a pointer to the datum; this will either be
     * an internal heap-based object, or a pointer into the FileBlock. */
    virtual UCHAR *  get_ptr (void) = 0;
    /** this returns the size of the datum; this method must account
     * for the contents of the object, if the object is variable sized. */
    virtual int     get_size (void) = 0;
    /** this returns the maximum size of the datum; this method must not
     * depend on the contents of the object, as it might be called before
     * the memory for the object is allocated. */
    virtual int get_max_size (void) = 0;
};

/** a template for cleanly encapsulating a user type into a datum.
 * it is a derived type from _BTDatum and thus is suitable as an argument
 * to Btree methods.
 * \param T the data type for the contents of this object.
 * type T must have the following members, with the following
 * suggested implementations:
 * <pre>
 *    UCHAR * get_ptr (void) { return (UCHAR*) this; }
 *    int get_size (void) { return sizeof(T); }
 *    static int get_max_size (void) { return sizeof(T); }
 * </pre>
 * if type T has a constructor, it must take no arguments.
 */
template <class T>
class BTDatum : public _BTDatum {
public:
    /** default constructor; this does NOT populate the Btree pointer,
     * and thus is only suitable for a key during a get or delete
     * operation. */
    BTDatum(void) : _BTDatum(NULL) { d = NULL; }
    /** constructor which takes a Btree pointer; this one is required
     * for any datum which will reside in a FileBlock, such as a data
     * pointer in a get call. */
    BTDatum(Btree * _bt) : _BTDatum(_bt) { d = NULL; }
    /** destructor which automatically releases the memory or FileBlock;
     * be sure to mark data as dirty if you have modified! */
    ~BTDatum(void) { release(); }
    /** allocate heap memory for type T; this is required for building
     * a key for a get or delete operation. */
    void alloc(void) {
        if (fb) { bt->get_fbi()->release(fb); fb = NULL; d = NULL; }
        if (d) { delete d; d = NULL; }
        d = new T;
    }
    /** release any resources; just like destructor but under the user's
     * control.  */
    void release(void) {
        if (fb) { bt->get_fbi()->release(fb); fb = NULL; d = NULL; }
        if (d) { delete d; d = NULL; }
    }
    /*virtual*/ void  setfb (FileBlock * _fb) {
        release();
        fb = _fb;
        d = (T *) fb->get_ptr();
    }
    /*virtual*/ UCHAR *  get_ptr (void) { return (UCHAR*) d; }
    /*virtual*/ int     get_size (void) { return d->get_size(); }
    /*virtual*/ int get_max_size (void) { return T::get_max_size(); }
    /** the actual data pointer; this will either point into the heap
     * if this is a heap-based object, or it will point into a FileBlock
     * if this data is in the file. */
    T * d;
};
