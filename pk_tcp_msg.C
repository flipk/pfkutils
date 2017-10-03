
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/select.h>

#include "pk_tcp_msg.H"

bool
pk_tcp_msgr :: send( pk_tcp_msg * m )
{
    m->set_checksum();
    int l = m->get_len();
    char * p = m->get_ptr();
    while ( l > 0 )
    {
        int cc = writer( p, l );
        if ( cc <= 0 )
            return false;
        p += cc;
        l -= cc;
    }
    return true;
}

enum states { HEADER, BODY };

bool
pk_tcp_msgr :: recv( pk_tcp_msg * m, int max_size )
{
    char * buf = m->get_ptr();
    states state = HEADER;
    int stateleft = sizeof( pk_tcp_msg );

    while ( 1 )
    {
        int cc = reader( buf, stateleft );

        if ( cc <= 0 )
            return false;

        stateleft -= cc;
        buf += cc;

        if ( stateleft == 0 )
        {
            switch ( state )
            {
            case HEADER:
                if ( !m->verif_magic() )
                    return false;

                stateleft = m->get_len();
                if ( stateleft > max_size )
                    return false;

                stateleft -= sizeof( pk_tcp_msg );
                if ( stateleft == 0 )
                    return true;

                state = BODY;
                break;

            case BODY:
                if ( !m->verif_checksum() )
                    return false;

                return true;
            }
        }
    }
}


#if 0

/* example definition of a message */

PkTcpMsgDef( TestMessage, 0x12345,
             int a;
             int b;
    );

class my_tcp_msgr : public pk_tcp_msgr {
private:
    /*virtual*/ int reader( void * buf, int buflen ) {
    }
    /*virtual*/ int writer( void * buf, int buflen ) {
    }
    /*virtual*/ bool read_pending( void ) {
    }
    int fd;
    int junk;
public:
    my_tcp_msgr( int _fd, int _junk ) { fd = _fd; junk = _junk; }
    ~my_tcp_msgr( void ) { close( fd ); }
};

int
testfunc( void )
{
    TestMessage tm;
    pk_tcp_msgr mgr( /*fd*/ 1, /*junk*/ 4 );

    mgr.send( &tm );
}

#endif
