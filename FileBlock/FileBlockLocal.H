/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

/** \file FileBlockLocal.H
 * \brief Defines the FileBlockLocal implementation of FileBlockInterface
 * \author Phillip F Knaack
 */

#ifndef __FILE_BLOCK_LOCAL_H__
#define __FILE_BLOCK_LOCAL_H__

#include "FileBlock_iface.H"
#include "dll2.H"
#include "ExtentMap.H"

class FileBlockLocalInt;
struct FileBlockLocalHeader;

/** A "Local File" implementation of FileBlockInterface.
 * 
 * This class talks to a BlockCache to implement file storage.
 * internally it uses an ExtentMap to keep track of the condition
 * of the file.
 * \note The contents of the ExtentMap is stored within the file
 *       itself; the ExtentMap will also have entries marking the 
 *       portions of the file in use where the ExtentMap is stored. */
class FileBlockLocal : public FileBlockInterface {
    /** linked list of all currently open blocks. */
    LList <FileBlockLocalInt,0>   FBLlist;
    /** the ExtentMap showing used and free portions of the file */
    Extents map;
    /** is the map space allocated from the map currently? */
    bool map_present;
    /** the interface to read and write the file itself */
    BlockCache * bc;
    /** read in a map from the file.
     * \param fbh a pointer to a header structure describing
     *            the file. */
    void load_map( FileBlockLocalHeader * fbh );
    /** free all disk space used by Extent pieces and piece maps.
     * \param fbh a pointer to a header structure describing
     *            the file. */
    void free_map( FileBlockLocalHeader * fbh );
    /** render the map into on-disk format, alloc space for it
     * from the map, and then write it to bc.  This function is
     * static in the class, so that init_file can call it too.
     * \param m the Extents map to write to disk
     * \param bc the interface to the file to use.
     * \param fbh a pointer to a header structure describing
     *            the file. */
    static void store_map( Extents * m, BlockCache * bc,
                           FileBlockLocalHeader * fbh );
public:
    /** Constructor.
     * \param _bc a BlockCache object which will be used to access
     *   the contents of the file */
    FileBlockLocal( BlockCache * _bc );
    /** Destructor.
     * \note This destroys the BlockCache which was passed in
     *   to the constructor! */
    ~FileBlockLocal( void );

    /** checks for signature etc, returns true if file appears to
     * be valid.  this function is static so that it can be called
     * prior to the constructor.
     * \see FileBlockInterface::valid_file
     * \param bc the interface to the file to check
     * \return true if file has signature, false if it does not */
    static bool valid_file( BlockCache * bc );
    /** takes a new file, and writes a signature etc to initialize
     * the file.  this function is static, because it needs to be called
     * before the class can be constructed.
     * \param bc the interface to the file to check. */
    static void init_file( BlockCache * bc );

    /*virtual*/ UINT32 get_data_info_block( char * info_name );
    /*virtual*/ void   set_data_info_block( UINT32 id, char *info_name );
    /*virtual*/ void   del_data_info_block( char * info_name );

    /*virtual*/ UINT32 realloc( UINT32 id, int new_size );
    /*virtual*/ UINT32 alloc( int size );
    /*virtual*/ void free( UINT32 id );
    /*virtual*/ FileBlock * get( UINT32 id, bool for_write = false );
    /*virtual*/ void release( FileBlock * blk, bool dirty );
    /*virtual*/ void flush(void);
    /*virtual*/ void compact(int time_limit);


    /** this function is for debug purposes. */
    void dump_extents( void );
};

#endif /* __FILE_BLOCK_LOCAL_H__ */
