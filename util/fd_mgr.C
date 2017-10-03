/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>

#include "fd_mgr.H"

void
fd_interface :: make_nonblocking( void )
{
    fcntl( fd, F_SETFL,
           fcntl( fd, F_GETFL, 0 ) | O_NONBLOCK );
}

bool
fd_mgr :: loop( struct timeval *tv )
{
    fd_interface * fdi, * nfdi;

    while ( ifds.get_cnt() > die_threshold )
    {
        fd_set rfds, wfds;
        int max, cc;

        FD_ZERO( &rfds );
        FD_ZERO( &wfds );

        max = -1;

        for ( fdi = ifds.get_head(); fdi; fdi = nfdi )
        {
            nfdi = ifds.get_next(fdi);
            bool dosrd, doswr;
            fdi->select_rw( this, &dosrd, &doswr );
            if ( dosrd )
            {
                if ( debug )
                    fprintf( stderr,
                             "selecting for read on fd %d\n", fdi->fd );
                FD_SET( fdi->fd, &rfds );
                if ( fdi->fd > max )
                    max = fdi->fd;
            }
            if ( doswr )
            {
                if ( debug )
                    fprintf( stderr, "selecting for write on fd %d\n",
                             fdi->fd );
                FD_SET( fdi->fd, &wfds );
                if ( fdi->fd > max )
                    max = fdi->fd;
            }
            if ( fdi->do_close )
            {
                if ( debug )
                    fprintf( stderr,
                             "deleting fd %d due to do_close flag\n",
                             fdi->fd );
                ifds.remove( fdi );
                delete fdi;
            }
        }
        if ( ifds.get_cnt() <= die_threshold )
            break;

        if ( max == -1 )
        {
            fprintf( stderr, "max == -1?!\n" );
            break;
        }

        cc = select( max+1, &rfds, &wfds, NULL, tv );

        if ( tv != NULL && cc == 0 )
            return false;

        if ( cc <= 0 )
        {
            if ( debug )
                fprintf( stderr, "select returns %d?! (%d:%s)\n",
                         cc, errno, strerror( errno ));
            continue;
        }

        for ( fdi = ifds.get_head(); fdi; fdi = nfdi )
        {
            bool del = false;
            nfdi = ifds.get_next(fdi);

            if ( FD_ISSET( fdi->fd, &rfds ))
            {
                if ( debug )
                    fprintf( stderr, "servicing read on fd %d\n", fdi->fd );
                if ( fdi->read(this) == fd_interface::DEL )
                {
                    if ( debug )
                        fprintf( stderr,
                                 "deleting %d due to false read\n", fdi->fd );
                    del = true;
                }
            }
            if ( FD_ISSET( fdi->fd, &wfds ))
            {
                if ( debug )
                    fprintf( stderr, "servicing write on fd %d\n", fdi->fd );
                if ( fdi->write(this) == fd_interface::DEL )
                {
                    if ( debug )
                        fprintf( stderr,
                                 "deleting %d due to false write\n", fdi->fd );
                    del = true;
                }
            }
            if ( !del && fdi->do_close )
            {
                del = true;
                if ( debug )
                    fprintf( stderr,
                             "deleting %d due to do_close\n", fdi->fd );
            }
            if ( del )
            {
                ifds.remove( fdi );
                delete fdi;
            }
        }
    }
    for ( fdi = ifds.get_head(); fdi; fdi = nfdi )
    {
        nfdi = ifds.get_next(fdi);
        ifds.remove( fdi );
        delete fdi;
    }
    return true;
}
