
/** \file FileBlockLocal.H
 * \brief Defines the FileBlockLocal implementation of FileBlockInterface
 * \author Phillip F Knaack
 */

#ifndef __FILE_BLOCK_LOCAL_H__
#define __FILE_BLOCK_LOCAL_H__

#include "FileBlock_iface.H"
#include "dll2.H"
#include "ExtentMap.H"

/** \brief A "Local File" implementation of FileBlockInterface
 * 
 * This class talks to a BlockCache to implement file storage.
 * internally it uses an ExtentMap to keep track of the condition
 * of the file.
 * \note The contents of the ExtentMap is stored within the file
 *       itself; the ExtentMap will also have entries marking the 
 *       portions of the file in use where the ExtentMap is stored. */
/** \todo the file info junk must be added here eventually */

class FileBlockLocal : public FileBlockInterface {
    /** \brief the ExtentMap showing used and free portions of the file */
    Extents map;
    /** \brief the interface to read and write the file itself */
    BlockCache * bc;
public:
    /** \brief Constructor
     * \param _bc a BlockCache object which will be used to access
     *   the contents of the file */
    FileBlockLocal( BlockCache * _bc );
    /** \brief Destructor
     * \note This does \b not destroy the BlockCache which was passed in
     *   to the constructor!  The caller must destroy the BlockCache. */
    ~FileBlockLocal( void );

    /*virtual*/ UINT32 alloc( int size );
    /*virtual*/ void free( UINT32 id );
    /*virtual*/ FileBlock * get_block( UINT32 id, bool for_write = false );
    /*virtual*/ void unlock_block( FileBlock * blk );
    /*virtual*/ void flush(void);
    /*virtual*/ void compact(int time_limit);
};

#endif /* __FILE_BLOCK_LOCAL_H__ */
