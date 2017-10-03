
#include "scan.H"
#include "trade.H"
#include "trade_internal.H"
#include "trade_messages.H"
#include "pk_tcp_msg.H"

int
tsn_tcp_chan :: reader( void * buf, int buflen )
{
    int cc = read( fd, buf, buflen );
    return cc;
}

int
tsn_tcp_chan :: writer( void * buf, int buflen )
{
    int cc = write( fd, buf, buflen );
    return cc;
}

bool
tsn_tcp_chan :: read_pending( void )
{
    fd_set  rfds;
    struct timeval tv = { 0, 0 };
    FD_ZERO( &rfds );
    FD_SET( fd, &rfds );
    select( fd+1, &rfds, NULL, NULL, &tv );
    if ( FD_ISSET( fd, &rfds ))
        return true;
    return false;
}

tsn_tcp_chan * tcp_chan;
static int sent_without_ack;

#define MAX_UNACKED 3

static void
handle_message( pk_tcp_msg * m )
{
    Final_File_Entry * ffe;
    union {
        pk_tcp_msg * m;
        MsgFileDigest * mfd;
        MsgFileDigestAck * mfda;
    } u;

    u.m = m;
    switch ( u.m->get_type() )
    {
    case MsgFileDigestAck::TYPE:
        ffe = scan_list->find( u.mfda->filename );
        if ( ffe )
        {
            sent_without_ack--;
            scan_list->remove( ffe );
            delete ffe;
        }
        break;

    case MsgFileDigest::TYPE:
    {
        //xxx
        MsgFileDigestAck  ack;
        strcpy( ack.filename, u.mfd->filename );
        tcp_chan->send( &ack );
        break;
    }

    }
}

bool
trade_first( void )
{
    Final_File_Entry * ffe, * nffe;
    MaxPkMsgType  maxrcvd;

    sent_without_ack = 0;
    for ( ffe = scan_list->get_head(); ffe; ffe = nffe )
    {
        nffe = scan_list->get_next(ffe);
        MsgFileDigest dig;
        int l = strlen( ffe->filename );
        if ( l >= TRADE_MAX_PATHLEN )
        {
            fprintf( stderr, "error: filename '%s' too long!\n",
                     ffe->filename );
            return false;
        }
        strcpy( dig.filename, ffe->filename );
        memcpy( dig.digest.digest, ffe->whole_digest.digest, 
                sizeof( dig.digest.digest ));
        tcp_chan->send( &dig );
        sent_without_ack++;
        while ( tcp_chan->recv_pending() || (sent_without_ack >= MAX_UNACKED))
        {
            if ( tcp_chan->recv( &maxrcvd, sizeof(maxrcvd) ) == false )
            {
                fprintf( stderr, "error on tcp channel\n" );
                return false;
            }
            handle_message( &maxrcvd );
        }
    }
}

bool
trade_second( void )
{
    MaxPkMsgType  maxrcvd;
    while ( 1 )
    {
        if ( tcp_chan->recv( &maxrcvd, sizeof(maxrcvd) ) == false )
        {
            fprintf( stderr, "error on tcp channel 2\n" );
            return false;
        }
        handle_message( &maxrcvd );
    }
}
