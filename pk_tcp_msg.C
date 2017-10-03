
#include "pk_tcp_msg.H"

bool
pk_tcp_msgr :: send( pk_tcp_msg * m )
{
    m->set_checksum();
    int l = m->get_len();
    char * p = (char*) m;
    while ( l > 0 )
    {
        int cc = ::write( fd, p, l );
        if ( cc <= 0 )
            return false;
        p += cc;
        l -= cc;
    }
    return true;
}

bool
pk_tcp_msgr :: recv( pk_tcp_msg * m, int max_size )
{
    char * buf = (char*) m;
    states state = HEADER;
    int stateleft = sizeof( pk_tcp_msg );
    while ( 1 )
    {
        int cc = read( fd, buf, stateleft );
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
