/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/** \file Btree.h
 * \brief definition of Btree access methods.
 * \author Phillip F Knaack
 */

#ifndef __BTREE_H__
#define __BTREE_H__

#include "FileBlock_iface.h"
#include "bst.h"

class BtreeInternal;

/** helper class for Btree::printinfo.  the user should populate
 * one of these before calling Btree::printinfo to help it decide
 * how to format the data.
 */
class BtreeIterator {
public:
    virtual ~BtreeIterator(void) { /* placeholder */ }
    virtual bool handle_item( uint8_t * keydata, uint32_t keylen,
                              FB_AUID_T data_fbn ) = 0;
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
    virtual bool get( uint8_t * key, int keylen, FB_AUID_T * data_id ) = 0;
    /** a version of the get method which understands BST data structures.
     * this method will encode the BST bytestream and then call the 
     * pointer/length version of the get method.
     * \param key the key data, in BST form.
     * \param data_id a pointer to the data that will be returned 
     *      if the search was successful.
     * \return true if the key was found or false if it was not found.
     */
    bool get( BST * key, FB_AUID_T * data_id ) {
        int keylen = 0;
        uint8_t * keybuf = key->bst_encode( &keylen );
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
    virtual bool put( uint8_t * key, int keylen, FB_AUID_T data_id,
                      bool replace=false, bool *replaced=NULL,
                      FB_AUID_T *old_data_id=NULL ) = 0;
    /** a version of the put method which supports key in BST form.
     * \param key the key data in BST form.  this key will be encoded into
     *     a temporary buffer before calling the pointer/length form of put.
     * \param data_id the data the user wishes to associate with the key.
     * \param replace if a matching key is found, this argument indicates
     *      whether it will replace the old data_id, or just fail.
     * \param replaced a pointer to a user's bool; if a matching key was
     *     found in the btree, the corresponding data will be returned, the
     *     new data_id will replace it, and *replaced will be set to true.
     *     if the key was new in the database, *replaced will be set to false.
     * \param old_data_id a pointer to a user's variable.  if *replaced is
     *     set to true (see the replaced param above) then *old_data_id will
     *     contain the old data associated with the key prior to replacement.
     * \return true if put was successful, or false if an error or if the
     *    key already exists and replace==false. */
    bool put( BST * key, FB_AUID_T data_id, bool replace=false,
              bool *replaced=NULL, FB_AUID_T *old_data_id=NULL ) {
        int keylen = 0;
        uint8_t * keybuf = key->bst_encode( &keylen );
        bool retval = false;
        if (keybuf)
        {
            retval = put( keybuf, keylen, data_id, replace,
                          replaced, old_data_id );
            delete[] keybuf;
        }
        return retval;
    }
    /** delete an item from the database given only the key.
     * this function will search for the key/data pair using the key,
     * and then free all disk space associated with the key and its data.
     * \param key  pointer to the key data.
     * \param keylen length of the key data.
     * \param old_data_id if the matching key was found, and the record
     *    is deleted, *old_data_id will be populated with the data value
     *    that was present in the btree.  the disposition of this data is
     *    left to the user, i.e. if it was being used to store FBN numbers,
     *    presumably the user will wish to free the associated file block.
     * \return true if the item was deleted, false if not found or error. */
    virtual bool del  ( uint8_t * key, int keylen, FB_AUID_T *old_data_id ) = 0;
    /** a BST version of del.
     * \param key the key data to delete.
     * \param old_data_id if the matching key was found, and the record
     *    is deleted, *old_data_id will be populated with the data value
     *    that was present in the btree.  the disposition of this data is
     *    left to the user, i.e. if it was being used to store FBN numbers,
     *    presumably the user will wish to free the associated file block.
     * \return true if the key/data was deleted or false if not. */
    bool del( BST * key, FB_AUID_T *old_data_id ) {
        int keylen = 0;
        uint8_t * keybuf = key->bst_encode( &keylen );
        bool retval = false;
        if (keybuf)
        {
            retval = del( keybuf, keylen, old_data_id );
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

#endif /* __BTREE_H__ */
