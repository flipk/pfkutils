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

/** \file PageIO.h
 * \brief Defines interfaces: PageIO, PageIOFileDescriptor.
 */

#ifndef __PAGE_IO_H__
#define __PAGE_IO_H__

#include <sys/types.h>
#include <stdio.h>
#include <string>

#include "aes.h"
#include "sha256.h"

class PageCachePage;

/** Pure-virual interface class for accessing a file.
 *
 * This is an abstraction of how to get and put pages to a file.
 * The idea is that the underlying implementation could be anything--
 * a file descriptor, an RPC object to another machine, an interface
 * to a flash memory device, a custom UDP or TCP-based communication
 * path, etc. */
class PageIO {
    //    each 4k page is encrypted/decrypted using aes-256-cbc.
    //    the 32 byte key is the sha256 hash of the password.
    //    the 16 byte IV is the XOR of the two 16 byte halves of the
    //       sha256 hash of password plus page_number.
    std::string encryption_password;
    aes_context  aesenc_ctx;
    aes_context  aesdec_ctx;
    sha256_context hmac_sha256_ctx;
public:
    // some OS's define a macro called PAGE_SIZE, and that would conflict
    // so this has to be unique.
    static const int PCP_PAGE_SIZE = 4096;
protected:
    static const int HMAC_OVERHD = 32; // sha256 hmac is 32 bytes
    static const int CIPHERED_PAGE_SIZE = PCP_PAGE_SIZE + HMAC_OVERHD;
    PageIO(const std::string &_encryption_password);
    bool ciphering_enabled;
    void encrypt_page(int page_number, uint8_t * out, const uint8_t * in);
    void decrypt_page(int page_number, uint8_t * out, const uint8_t * in);
public:
    /** Create a PageIO, by opening a path. 
     *
     * The format of "path" determines the underlying implementation.
     */
    static PageIO * open( const char * path, bool create, int mode );
    /** virtual destructor placeholder.
     *
     * This class provides a virtual destructor, so that any derived
     * classes can implement their own destructors which are invoked
     * when this object is destroyed. */
    virtual ~PageIO(void);
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
    /** cut the file to a certain size. */
    virtual void  truncate_pages(int num_pages) = 0;
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
    PageIOFileDescriptor(const std::string &_encryption_password, int _fd);
    /** Destructor.
     * \note This destructor closes the file descriptor too!  */
    /*virtual*/ ~PageIOFileDescriptor(void);

    // doxygen comments not required, because they will be inherited
    // from the base class documentation.
    /*virtual*/ bool  get_page( PageCachePage * pg );
    /*virtual*/ bool  put_page( PageCachePage * pg );
    /*virtual*/ int   get_num_pages(bool * page_aligned = NULL);
    /*virtual*/ off_t get_size(void);
    /*virtual*/ void  truncate_pages(int num_pages);
};

class PageIONetworkTCPServer : public PageIO {
    int fd; /**< network descriptor, tcp socket */
public:
    PageIONetworkTCPServer(const std::string &_encryption_password,
                           void *addr, // actually, struct in_addr *
                           int port);
    /*virtual*/ ~PageIONetworkTCPServer(void);

    // doxygen comments not required, because they will be inherited
    // from the base class documentation.
    /*virtual*/ bool  get_page( PageCachePage * pg );
    /*virtual*/ bool  put_page( PageCachePage * pg );
    /*virtual*/ int   get_num_pages(bool * page_aligned = NULL);
    /*virtual*/ off_t get_size(void);
    /*virtual*/ void  truncate_pages(int num_pages);
};

#endif /* __PAGE_IO_H__ */
