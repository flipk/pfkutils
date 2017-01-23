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

#include "PageCache.h"
#include "PageIO.h"

/** \todo: implement these functions. */


PageIONetworkTCPServer :: PageIONetworkTCPServer(
    const std::string &_encryption_password,
    void *addr, // actually, struct in_addr *
    int port)
    : PageIO(_encryption_password)
{
}

PageIONetworkTCPServer :: ~PageIONetworkTCPServer(void)
{
}

//virtual
bool
PageIONetworkTCPServer :: get_page( PageCachePage * pg )
{
    return false;
}

//virtual
bool
PageIONetworkTCPServer :: put_page( PageCachePage * pg )
{
    return false;
}

//virtual
int
PageIONetworkTCPServer :: get_num_pages(bool * page_aligned)
{
    return 0;
}

//virtual
off_t
PageIONetworkTCPServer :: get_size(void)
{
    return 0;
}

//virtual
void
PageIONetworkTCPServer :: truncate_pages(int num_pages)
{
}
