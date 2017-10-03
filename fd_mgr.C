
#include "fd_mgr.H"

#include <unistd.h>

void
fd_mgr :: loop( void )
{
    while ( ifds.get_cnt() > 0 )
    {
        fd_interface * fdi, * nfdi;
        fd_set rfds, wfds;
        int max, cc;

        FD_ZERO( &rfds );
        FD_ZERO( &wfds );

        max = -1;
        for ( fdi = ifds.get_head(); fdi; fdi = nfdi )
        {
            nfdi = ifds.get_next(fdi);

            if ( fdi->do_close )
            {
                ifds.remove( fdi );
                delete fdi;
            }
            else 
            {
                if ( fdi->select_for_read(this))
                {
//                    printf( "selecting for read on fd %d\n", fdi->fd );
                    FD_SET( fdi->fd, &rfds );
                    if ( fdi->fd > max )
                        max = fdi->fd;
                }
                if ( fdi->select_for_write(this))
                {
//                    printf( "selecting for write on fd %d\n", fdi->fd );
                    FD_SET( fdi->fd, &wfds );
                    if ( fdi->fd > max )
                        max = fdi->fd;
                }
            }
        }

        if ( max == -1 )
        {
            printf( "max == -1?!\n" );
            break;
        }

        cc = select( max+1, &rfds, &wfds, NULL, NULL );

        if ( cc == 0 )
        {
            printf( "select returns 0?!\n" );
            continue;
        }

        for ( fdi = ifds.get_head(); fdi; fdi = nfdi )
        {
            bool del = false;
            nfdi = ifds.get_next(fdi);

            if ( FD_ISSET( fdi->fd, &rfds ))
            {
//                printf( "servicing read on fd %d\n", fdi->fd );
                if ( fdi->read(this) == false )
                    del = true;
            }
            if ( FD_ISSET( fdi->fd, &wfds ))
            {
//                printf( "servicing write on fd %d\n", fdi->fd );
                if ( fdi->write(this) == false )
                    del = true;
            }
            if ( del )
            {
                ifds.remove( fdi );
                delete fdi;
            }
        }
    }
}
