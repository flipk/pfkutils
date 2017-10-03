
/** \file PageIO.H
 * \brief Defines interface to input and output of pages from a file.
 * \author Phillip F Knaack
 */

#ifndef __PAGE_IO_H__
#define __PAGE_IO_H__

#include <sys/types.h>

/** \page PageIO PageIO object

The lowest layer is a derived object from PageIO.  This object knows
only how to read and write PageCachePage objects, whose body is of
size PageCache::PAGE_SIZE.  An example implementation of PageIO is the
object PageIOFileDescriptor, which uses a file descriptor (presumably
an open file) to read and write offsets in the file.

If the user wishes some other storage mechanism (such as a file on a
remote server, accessed via RPC/TCP for example) then the user may
implement another PageIO backend which performs the necessary
interfacing.  This new PageIO object may be passed to the PageCache
constructor.

Next: \ref PageCache

*/

class PageCachePage;

/** Pure-virual interface class for accessing a file.
 *
 * This is an abstraction of how to get and put pages to a file.
 * The idea is that the underlying implementation could be anything--
 * a file descriptor, an RPC object to another machine, an interface
 * to a flash memory device, a custom UDP or TCP-based communication
 * path, etc. */
class PageIO {
public:
    /** virtual destructor placeholder.
     *
     * This class provides a virtual destructor, so that any derived
     * classes can implement their own destructors which are invoked
     * when this object is destroyed. */
    virtual ~PageIO(void) { /* placeholder */ }
    /** Fetch a page from the file.
     * \param pg A PageCachePage object to populate
     * \return true if the fetch succeeded, false if error
     * \note This method assumes pg->page_number was already populated */
    virtual bool  get_page( PageCachePage * pg ) = 0;
    /** Write a page to the file.
     * \param pg A PageCachePage to write
     * \return true if the write succeeded, false if error */
    virtual bool  put_page( PageCachePage * pg ) = 0;
    /** return size of the file in pages.
     * \param page_aligned pointer to a bool; if NULL, it is ignored;
     *  if not NULL, the bool will be written with true if the size of
     *  the file is an even multiple of the page size, or false if the 
     *  file size is not an even multiple of the page size.
     * \note This method rounds up the return value to the nearest page,
     *  if the size of the file is not an even multiple of a page size. */
    virtual int   get_num_pages(bool * page_aligned = NULL) = 0;
    /** return size of the file in bytes. */
    virtual off_t get_size(void) = 0;
};

/** An example implementation of PageIO using a file descriptor.
 *
 * This class is an example of how to create a PageIO object.
 * This one is useful for local files.  Just open(2) the file
 * and pass the fd to this class. */
class PageIOFileDescriptor : public PageIO {
    /** The file descriptor of the file being accessed. */
    int fd;
public:
    /** Constructor.
     * \param _fd The file descriptor of the file to access */
    PageIOFileDescriptor(int _fd);
    /** Destructor.
     * \note This destructor does \b NOT close the file descriptor!  */
    /*virtual*/ ~PageIOFileDescriptor(void);

    // doxygen comments not required, because they will be inherited
    // from the base class documentation.
    /*virtual*/ bool get_page( PageCachePage * pg );
    /*virtual*/ bool put_page( PageCachePage * pg );
    /*virtual*/ int get_num_pages(bool * page_aligned = NULL);
    /*virtual*/ off_t get_size(void);
};

#endif /* __PAGE_IO_H__ */
