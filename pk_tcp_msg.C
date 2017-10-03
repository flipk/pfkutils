
#include "pk_tcp_msg.H"

bool
pk_tcp_msgr :: send( pk_tcp_msg * m )
{
    m->set_checksum();
    int l = m->get_len();
    return ( ::write( fd, (char*)m, l ) == l );
}

bool
pk_tcp_msgr :: recv( pk_tcp_msg * m, int max_size )
{
    char * buf = (char*) m;
    states state = HEADER;
    int stateleft = sizeof( pk_tcp_msg );
    int pos = 0;
    while ( 1 )
    {
        int cc = read( fd, buf + pos, stateleft );
        if ( cc <= 0 )
            return false;
        stateleft -= cc;
        pos += cc;
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
