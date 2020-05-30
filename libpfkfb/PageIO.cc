
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

/** \file PageIO.cc
 * \brief Implements PageIOFileDescriptor.
 */

/** \page PageIO PageIO object

The lowest layer is a derived object from PageIO.  This object knows
only how to read and write PageCachePage objects, whose body is of
size PageIO::PAGE_SIZE.  An example implementation of PageIO is the
object PageIOFileDescriptor, which uses a file descriptor (presumably
an open file) to read and write offsets in the file.

If the user wishes some other storage mechanism (such as a file on a
remote server, accessed via RPC/TCP for example) then the user may
implement another PageIO backend which performs the necessary
interfacing.  This new PageIO object may be passed to the PageCache
constructor.

Next: \ref PageCache

*/

#include "pfkutils_config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <sstream>

#include "PageCache.h"
#include "PageIO.h"
#include "regex.h"

static const char * path_pattern = "\
^fbsrv:([0-9]+.[0-9]+):([0-9]+)$|\
^fbsrv:([0-9]+.[0-9]+):([0-9]+):([0-9a-zA-Z]+)$|\
^fbsrv:([0-9]+.[0-9]+.[0-9]+.[0-9]+):([0-9]+.)$|\
^fbsrv:([0-9]+.[0-9]+.[0-9]+.[0-9]+):([0-9]+.):([0-9a-zA-Z]+)$|\
^fbsrv:(.+):([0-9]+.)$|\
^fbsrv:(.+):([0-9]+.):([0-9a-zA-Z]+)$|\
^(.+)/:([0-9a-zA-Z]+)$|\
^(.+):([0-9a-zA-Z]+)$|\
^(.+)/$|\
^(.+)$";

#define MATCH_LIST \
    MATCH_ENTRY(ALL) \
    MATCH_ENTRY(FBSRV_IP_2_CT) \
    MATCH_ENTRY(FBSRV_PORT_2_CT) \
    MATCH_ENTRY(FBSRV_IP_2_ENC) \
    MATCH_ENTRY(FBSRV_PORT_2_ENC) \
    MATCH_ENTRY(FBSRV_PORT_2_KEY) \
    MATCH_ENTRY(FBSRV_IP_4_CT) \
    MATCH_ENTRY(FBSRV_PORT_4_CT) \
    MATCH_ENTRY(FBSRV_IP_4_ENC) \
    MATCH_ENTRY(FBSRV_PORT_4_ENC) \
    MATCH_ENTRY(FBSRV_PORT_4_KEY) \
    MATCH_ENTRY(FBSRV_HOSTNAME_CT) \
    MATCH_ENTRY(FBSRV_PORT_HN_CT) \
    MATCH_ENTRY(FBSRV_HOSTNAME_ENC) \
    MATCH_ENTRY(FBSRV_PORT_HN_ENC) \
    MATCH_ENTRY(FBSRV_PORT_HN_KEY) \
    MATCH_ENTRY(FULLPATH_SPLIT_ENC) \
    MATCH_ENTRY(FULLPATH_SPLIT_KEY) \
    MATCH_ENTRY(FULLPATH_ENC) \
    MATCH_ENTRY(FULLPATH_KEY) \
    MATCH_ENTRY(FULLPATH_SPLIT_CT) \
    MATCH_ENTRY(FULLPATH_CT) \
    MATCH_ENTRY(MAX_MATCHES)

enum {
#define MATCH_ENTRY(x)  MATCH_##x,
    MATCH_LIST
#undef MATCH_ENTRY
};

#define REGEX_ARG_PARSING_DEBUG 0

#if REGEX_ARG_PARSING_DEBUG
const char * match_names[] = {
#define MATCH_ENTRY(x)  #x,
    MATCH_LIST
#undef MATCH_ENTRY
};
#endif

//static
PageIO *
PageIO :: open( const char * _path, bool create, int mode )
{
    regex_t expr;
    regmatch_t matches[ MATCH_MAX_MATCHES ];
    int regerr;
    char errbuf[80];
    std::string path(_path);

    regerr = regcomp( &expr, path_pattern, REG_EXTENDED );
    if (regerr != 0)
    {
        regerror( regerr, &expr, errbuf, sizeof(errbuf) );
        fprintf(stderr,"regcomp error: %s\n", errbuf);
        return NULL;
    }

    regerr = regexec( &expr, _path, MATCH_MAX_MATCHES, matches, 0 );
    if (regerr != 0)
    {
        regerror( regerr, &expr, errbuf, sizeof(errbuf) );
        fprintf(stderr,"regexec error: %s\n", errbuf);
        return NULL;
    }

    regfree( &expr );

#if REGEX_ARG_PARSING_DEBUG
    for (int i = 0; i < MATCH_MAX_MATCHES; i++)
    {
        std::string  substr;
        if (matches[i].rm_so != -1)
            substr = path.substr(matches[i].rm_so,
                            matches[i].rm_eo - matches[i].rm_so);
        printf("entry %2d: %20s so %2d eo %2d   \"%s\"\n",
               i, match_names[i], matches[i].rm_so, matches[i].rm_eo,
               substr.c_str());
    }
#endif

#define SUBSTR(match) path.substr(\
        matches[MATCH_##match].rm_so, \
        matches[MATCH_##match].rm_eo - matches[MATCH_##match].rm_so)

#define ISMATCH(match) (matches[MATCH_##match].rm_so != -1)

    char string[512];
    struct in_addr ipaddr;
    struct hostent * he;
    int port = 0;
    bool tcp = false;
    bool splitfile = false;
    std::string str_ip, str_host, str_port, str_key, str_file;

    if (ISMATCH(FBSRV_IP_2_CT))
    {
        tcp = true;
        str_ip = SUBSTR(FBSRV_IP_2_CT);
        str_port = SUBSTR(FBSRV_PORT_2_CT);
    }
    else if (ISMATCH(FBSRV_IP_2_ENC))
    {
        tcp = true;
        str_ip = SUBSTR(FBSRV_IP_2_ENC);
        str_port = SUBSTR(FBSRV_PORT_2_ENC);
        str_key = SUBSTR(FBSRV_PORT_2_KEY);
    }
    else if (ISMATCH(FBSRV_IP_4_CT))
    {
        tcp = true;
        str_ip = SUBSTR(FBSRV_IP_4_CT);
        str_port = SUBSTR(FBSRV_PORT_4_CT);
    }
    else if (ISMATCH(FBSRV_IP_4_ENC))
    {
        tcp = true;
        str_ip = SUBSTR(FBSRV_IP_4_ENC);
        str_port = SUBSTR(FBSRV_PORT_4_ENC);
        str_key = SUBSTR(FBSRV_PORT_4_KEY);
    }
    else if (ISMATCH(FBSRV_HOSTNAME_CT))
    {
        tcp = true;
        str_host = SUBSTR(FBSRV_HOSTNAME_CT);
        str_port = SUBSTR(FBSRV_PORT_HN_CT);
    }
    else if (ISMATCH(FBSRV_HOSTNAME_ENC))
    {
        tcp = true;
        str_host = SUBSTR(FBSRV_HOSTNAME_ENC);
        str_port = SUBSTR(FBSRV_PORT_HN_ENC);
        str_key = SUBSTR(FBSRV_PORT_HN_KEY);
    }
    else if (ISMATCH(FULLPATH_SPLIT_ENC))
    {
        tcp = false;
        splitfile = true;
        str_file = SUBSTR(FULLPATH_SPLIT_ENC);
        str_key = SUBSTR(FULLPATH_SPLIT_KEY);
    }
    else if (ISMATCH(FULLPATH_ENC))
    {
        tcp = false;
        str_file = SUBSTR(FULLPATH_ENC);
        str_key = SUBSTR(FULLPATH_KEY);
    }
    else if (ISMATCH(FULLPATH_SPLIT_CT))
    {
        tcp = false;
        splitfile = true;
        str_file = SUBSTR(FULLPATH_SPLIT_CT);
    }
    else if (ISMATCH(FULLPATH_CT))
    {
        tcp = false;
        str_file = SUBSTR(FULLPATH_CT);
    }

    if (str_port.length() > 0)
        port = atoi(str_port.c_str());

    if (str_host.length() > 0)
    {
        he = gethostbyname(string);
        if (he)
            memcpy(&ipaddr.s_addr, he->h_addr, sizeof(ipaddr.s_addr));
        else
        {
            fprintf(stderr, "unknown host name '%s'\n", string);
            return NULL;
        }
    }

    if (str_ip.length() > 0)
        inet_aton(str_ip.c_str(), &ipaddr);

    PageIO * ret = NULL;
    
    if (tcp)
        ret = new PageIONetworkTCPServer(str_key, &ipaddr, port);
    else
    {
        if (splitfile == false)
        {
            int options = O_RDWR;
            if (create)
                options |= O_CREAT;
#ifdef O_LARGEFILE
            options |= O_LARGEFILE;
#endif
            int fd = ::open( str_file.c_str(), options, mode );
            if (fd > 0)
                ret = new PageIOFileDescriptor(str_key, fd);
        }
        else
        {
            PageIODirectoryTree * _ret =
                new PageIODirectoryTree(str_key, str_file.c_str(), create);
            if (_ret && _ret->get_ok())
                ret = _ret;
            else
                delete _ret;
        }
    }

    return ret;
}

PageIO :: PageIO(const std::string &_encryption_password)
    : encryption_password(_encryption_password)
{
    if (encryption_password.length() > 0)
    {
        ciphering_enabled = true;
        // init cipher shit
        mbedtls_aes_init( &aesenc_ctx );
        mbedtls_aes_init( &aesdec_ctx );
        unsigned char file_key[32];
        MBEDTLS_SHA256( (const unsigned char *) encryption_password.c_str(),
                encryption_password.length(),
                file_key, 0/*use SHA256*/);
        mbedtls_aes_setkey_enc( &aesenc_ctx, file_key, 256 );
        mbedtls_aes_setkey_dec( &aesdec_ctx, file_key, 256 );
        mbedtls_md_init( &hmac_md_ctx );
        mbedtls_md_setup( &hmac_md_ctx,
                          mbedtls_md_info_from_type( MBEDTLS_MD_SHA256 ),
                          /*use hmac*/ 1);
        mbedtls_md_hmac_starts( &hmac_md_ctx, file_key, 32 );
    }
    else
        ciphering_enabled = false;
}

PageIO :: ~PageIO(void)
{
    if (ciphering_enabled)
    {
        mbedtls_aes_free( &aesenc_ctx );
        mbedtls_aes_free( &aesdec_ctx );
        mbedtls_md_free( &hmac_md_ctx );
    }
}

static inline void
make_iv(unsigned char IV_plus_sha256[32],
        const std::string &pass, uint64_t page)
{
    std::ostringstream  ostr;
    ostr << pass << ":" << page;
    MBEDTLS_SHA256( (const unsigned char*) ostr.str().c_str(),
                    ostr.str().length(),
                    IV_plus_sha256, 0/*use SHA256*/);
    for (int ind = 0; ind < 16; ind++)
        IV_plus_sha256[ind] ^= IV_plus_sha256[ind+16];
}

void
PageIO :: encrypt_page(uint64_t page_number, uint8_t * out, const uint8_t * in)
{
    unsigned char IV[32];
    make_iv(IV, encryption_password, page_number);
    mbedtls_aes_crypt_cbc( &aesenc_ctx, MBEDTLS_AES_ENCRYPT,
                   PCP_PAGE_SIZE, IV, in, out);
    mbedtls_md_hmac_reset( &hmac_md_ctx );
    mbedtls_md_hmac_update( &hmac_md_ctx, out, PCP_PAGE_SIZE);
    mbedtls_md_hmac_finish( &hmac_md_ctx, out + PCP_PAGE_SIZE );
}

void
PageIO :: decrypt_page(uint64_t page_number, uint8_t * out, const uint8_t * in)
{
    unsigned char IV[32];
    make_iv(IV, encryption_password, page_number);
    uint8_t  hmac_buf[32];
    mbedtls_md_hmac_reset( &hmac_md_ctx );
    mbedtls_md_hmac_update( &hmac_md_ctx, in, PCP_PAGE_SIZE);
    mbedtls_md_hmac_finish( &hmac_md_ctx, hmac_buf );

    if (memcmp(hmac_buf, in + PCP_PAGE_SIZE, 32) != 0)
    {
        printf("PageIO :: decrypt_page : HMAC FAILURE!\n");
    }
    mbedtls_aes_crypt_cbc( &aesdec_ctx, MBEDTLS_AES_DECRYPT,
                   PCP_PAGE_SIZE, IV, in, out);
}
