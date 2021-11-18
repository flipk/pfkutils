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

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "ipipe_connector.h"

ipipe_connector :: ipipe_connector( struct sockaddr_in * sa,
                                    ipipe_new_connection * inc )
{
    connection_factory = inc;

    fd = socket( AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0 );
    if ( fd < 0 )
    {
        fprintf( stderr, "\nsocket: %s\n", strerror( errno ));
        exit( 1 );
    }

    make_nonblocking();

    int cc = connect( fd, (struct sockaddr *)sa, sizeof( *sa ));

    if ( cc < 0  &&  errno != EINPROGRESS )
    {
        fprintf( stderr, "\nconnect: %s\n", strerror( errno ));
        exit( 1 );
    }
}

//virtual
ipipe_connector :: ~ipipe_connector( void )
{
    delete connection_factory;
    // no close! the fd was passed on
}

//virtual
void
ipipe_connector :: select_rw ( fd_mgr *, bool * rd, bool * wr )
{
    *rd = false;

    if ( !do_close )
        *wr = true;
    else
        *wr = false;
}

//virtual
fd_interface :: rw_response
ipipe_connector :: write( fd_mgr * mgr )
{
    int cc;
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);

    cc = getpeername( fd, (struct sockaddr *)&sa, &salen );

    if ( cc < 0 )
    {
        fprintf( stderr, "\nconnect: Unable to connect\n" );
        return DEL;
    }

    (void) connection_factory->new_conn( mgr, fd );
    return DEL;
}
