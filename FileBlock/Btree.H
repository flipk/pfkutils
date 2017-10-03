
/** \file Btree.H
 * \brief definition of Btree access methods.
 * \author Phillip F Knaack
 */

#include "FileBlock_iface.H"

class _BTDatum;
class BtreeInternal;

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
     * \return true if the init was successful, false if error. */
    static bool init_file( FileBlockInterface * fbi );
    /** Return a pointer to the FileBlockInterface backend.  This allows the
     * user of Btree to store their own data directly in the file without
     * going only through the Btree API.
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
     * in the file, the data will be replaced by the data argument here. 
     * \note This function should /em only be used for /em new items;
     * it should not be used to replace existing items.
     * \param key the BDatum object with key data.
     * \param data the BDatum object with data.
     * \return true if the item was put in the database; false if an error. */
    virtual bool put( _BTDatum * key, _BTDatum * data ) = 0;
    /** delete an item from the database given only the key.
     * this function will search for the key/data pair using the key,
     * and then free all disk space associated with the key and its data. */
    virtual bool del  ( _BTDatum * key  ) = 0;
};

class _BTDatum {
protected:    
    friend class BtreeInternal;
    Btree * bt;
public:
    FileBlock * fb;
    _BTDatum(Btree * _bt) { bt = _bt; fb = NULL; }
    virtual ~_BTDatum(void) { /*placeholder*/ }
    virtual void  setptr (UCHAR *) = 0;
    virtual void  setfb (FileBlock *) = 0;
    virtual UCHAR *  get_ptr (void) = 0;
    virtual int     get_size (void) = 0;
    virtual int get_max_size (void) = 0;
};

// type T must have the following members, with the following
// suggested implementations:
//    UCHAR * get_ptr (void) { return (UCHAR*) this; }
//    int get_size (void) { return sizeof(T); }
//    static int get_max_size (void) { return sizeof(T); }
// if type T has a constructor, it must take no arguments.

template <class T>
class BTDatum : public _BTDatum {
public:
    BTDatum(Btree * _bt) : _BTDatum(_bt) { d = NULL; }
    ~BTDatum(void) { release(); }
    void alloc(void) {
        if (fb) { bt->get_fbi()->release(fb); fb = NULL; d = NULL; }
        if (d) { delete d; d = NULL; }
        d = new T;
    }
    void release(void) {
        if (fb) { bt->get_fbi()->release(fb); fb = NULL; d = NULL; }
        if (d) { delete d; d = NULL; }
    }
    /*virtual*/ void  setptr (UCHAR *ptr) {
        release();
        d = (T *) ptr;
    }
    /*virtual*/ void  setfb (FileBlock * _fb) {
        release();
        fb = _fb;
        d = (T *) fb->get_ptr();
    }
    /*virtual*/ UCHAR *  get_ptr (void) { return (UCHAR*) d; }
    /*virtual*/ int     get_size (void) { return d->size(); }
    /*virtual*/ int get_max_size (void) { return T::max_size(); }
    T * d;
};
