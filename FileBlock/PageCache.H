
/** \file PageCache.H
 * \brief Define interfaces:
 *   PageCache, PageCachePage, PageIO, PageIOFileDescriptor.
 * \author Phillip F Knaack */

#ifndef __PAGE_CACHE_H__
#define __PAGE_CACHE_H__

#include "types.H"

#include <sys/types.h>
#include <unistd.h>

class PageIO;
class PageCachePage;
class PageCachePageList;

/** \brief A cache of PageCachePage objects fetched thru a PageIO object
 *
 * This object will fetch pages from a file using the PageIO interface
 * and return pointers to the user.  It will also maintain reference counts
 * on pages so that multiple callers may reference the same page multiple
 * times.  To prevent thrashing on the filesystem, pages are only written
 * back thru PageIO when the caching limit has been reached or when a flush
 * operation is requested. */
class PageCache {
    /** \brief PageCache must limit number of pages it will cache */
    int max_pages;
    /** \brief the PageIO interface for getting data in/out of a file */
    PageIO * io;
    /** \brief the compound data type for storing the cached pages. */
    PageCachePageList * pgs;
public:
    /** \brief a constant indicating how big a page is. */
    static const int PAGE_SIZE = 65536;
    /** \brief Constructor 
     * \param _io the PageIO that will be used to fetch and put pages.
     * \param _max_pages the number of pages that we will be allowed to
     *  keep in memory; this limits the memory utilization of this object
     *  to resonable levels. */
    PageCache( PageIO * _io, int _max_pages );
    /** \brief Destructor.
     * \note This destructor does \b NOT delete the PageIO object!
     * \note All in-use pages should be released prio to destroying this
     *   object! */
    ~PageCache(void);
    /** \brief return the internal PageIO object
     * \note This function bypasses the cache and can thus be dangerous!
     *   Improper use may corrupt the file contents due to incoherent cache.
     * \return A pointer to the PageIO in use by this object.
     *
     * Assumption is that user may want to store other information in
     * this file; this interface allows the user to access file not managed
     * by the cache. */
    PageIO * get_io(void) { return io; }
    /** \brief Retrieve a page from the file.
     * \param page_number the page number of the page to be retrieved.
     * \param for_write If the user intends to write the entire page and does
     *  not care to read anything from the file, this parameter allows an 
     *  optimization where an empty page is returned without invoking PageIO
     *  to read it.
     * \return A pointer to a PageCachePage object for the page.
     *
     * If the page is currently in the cache, this will return a pointer
     * to the cached page and increase the page's reference count.  If the
     * page is not in cache, the PageIO interface is accessed to retrieve
     * the page from the file. */
    PageCachePage * get(int page_number, bool for_write);
    /** \brief Release a page from user's access.
     * \param p A PageCachePage previously returned by the get() method.
     * \param dirty The user must indicate if he modified this page, so that
     *  we can decide if we have to access PageIO to write it back again.
     *
     * This method dereferences the page and puts it back on an LRU. If 
     * the user modified the page, the page must be marked as dirty, so that
     * it will eventually be written back to the file (thru the PageIO
     * interface. */
    void release( PageCachePage * p, bool dirty );
    /** \brief Flush the cache, force synchronization
     *
     * This method walks the list of all dirty pages, as well as all 
     * locked pages, and writes all changes thru the PageIO interface. */
    void flush(void);
};

/** \brief Access object for a page of a file.
 *
 * This object contains one page of a file. The user may access the
 * contents of the page through a pointer in this object.  The user
 * must notify this object (thru the mark_dirty method) if he has
 * modified the contents of this data. */
class PageCachePage {
    friend class PageCache;
    /** \brief the number of the page in the file this object contains */
    int page_number;
    /** \brief indicates whether the data has been modified */
    bool dirty;
protected:
    /** \brief allocate memory for the page and initialize member fields.
     * \param _page_number the number of the page being referenced.
     * \note this constructor does not populate the contents of the page;
     *   it is assumed the caller will do that. */
    PageCachePage(int _page_number) {
        dirty = false;  page_number = _page_number;
        ptr = new UCHAR[PageCache::PAGE_SIZE];
    }
    /** \brief destructor frees memory for the page.
     * \note this destructor does not write the contents back to the file.
     *   it is assumed the caller will do that. */
    ~PageCachePage(void) { delete[] ptr; }
    /** \brief a pointer to the page data itself. */
    UCHAR * ptr;
public:
    /** \brief access method to return the page number. */
    int get_page_number(void) { return page_number; }
    /** \brief access method to get the data pointer. */
    UCHAR * get_ptr(void) { return ptr; }
    /** \brief user must call this if he has modified the page data. */
    void mark_dirty(void) { dirty = true; }
};

/** \brief Pure-virual interface class for accessing a file.
 *
 * This is an abstraction of how to get and put pages to a file.
 * The idea is that the underlying implementation could be anything--
 * a file descriptor, an RPC object to another machine, an interface
 * to a flash memory device, a custom UDP or TCP-based communication
 * path, etc. */
class PageIO {
public:
    /** \brief virtual destructor placeholder
     *
     * This class provides a virtual destructor, so that any derived
     * classes can implement their own destructors which are invoked
     * when this object is destroyed. */
    virtual ~PageIO(void) { /* placeholder */ }
    /** \brief Fetch a page from the file
     * \param pg A PageCachePage object to populate
     * \return true if the fetch succeeded, false if error
     * \note This method assumes pg->page_number was already populated */
    virtual bool  get_page( PageCachePage * pg ) = 0;
    /** \brief Write a page to the file
     * \param pg A PageCachePage to write
     * \return true if the write succeeded, false if error */
    virtual bool  put_page( PageCachePage * pg ) = 0;
    /** \brief return size of the file in pages
     * \param page_aligned pointer to a bool; if NULL, it is ignored;
     *  if not NULL, the bool will be written with true if the size of
     *  the file is an even multiple of the page size, or false if the 
     *  file size is not an even multiple of the page size.
     * \note This method rounds up the return value to the nearest page,
     *  if the size of the file is not an even multiple of a page size. */
    virtual int   get_num_pages(bool * page_aligned = NULL) = 0;
    /** \brief return size of the file in bytes. */
    virtual off_t get_size(void) = 0;
};

/** \brief An example implementation of PageIO using a file descriptor.
 *
 * This class is an example of how to create a PageIO object.
 * This one is useful for local files.  Just open(2) the file
 * and pass the fd to this class. */
class PageIOFileDescriptor : public PageIO {
    /** \brief The file descriptor of the file being accessed. */
    int fd;
public:
    /** \brief Constructor
     * \param _fd The file descriptor of the file to access */
    PageIOFileDescriptor(int _fd);
    /** \brief Destructor
     * \note This destructor does \b NOT close the file descriptor!  */
    /*virtual*/ ~PageIOFileDescriptor(void);

    // doxygen comments not required, because they will be inherited
    // from the base class documentation.
    /*virtual*/ bool get_page( PageCachePage * pg );
    /*virtual*/ bool put_page( PageCachePage * pg );
    /*virtual*/ int get_num_pages(bool * page_aligned = NULL);
    /*virtual*/ off_t get_size(void);
};

#endif /* __PAGE_CACHE_H__ */
