/*
 * This code is written by Phillip F Knaack. This code is in the
 * public domain. Do anything you want with this code -- compile it,
 * run it, print it out, pass it around, shit on it -- anything you want,
 * except, don't claim you wrote it.  If you modify it, add your name to
 * this comment in the COPYRIGHT file and to this comment in every file
 * you change.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

//
// this is the implementation of a message-passing object that
// uses TCP.  two different processes, each running the Threads
// system, can connect a TCP connection to each other using this
// object.  As long as the two Threads objects are initialized with
// different entity_ids, they can communicate, because the Messages
// subsystem knows how to forward data to other Entities.
//
// this object also uses encryption from an encrypt_iface object.
// it will encrypt every message sent over TCP and decrypt it on the
// other side.  the only caveat is that both sides must use the same key.
// key exchange is left to the application (for now, that may come in the
// future). 
//
// the only data sent over the link without encryption is a magic number
// header to each packet followed by a packet length. the body of the
// packet is then encrypted.
//
// it is possible to change keys in the middle of a session. one side of
// the connection initiates that by calling the switch_key() method. this
// object then sends the key to the other side and both sides begin using
// the new key.   NB: the new key is only as secure as the old encryption
// key in use during the switchover.  this dependency should be removed.
// one difficulty is the lack of an asymmetric encryption implementation.
// we should implement at some point an rsa-like encryption for use during
// key exchange.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/uio.h>

#include "threads.H"

#if defined(sparc) || defined(__CYGWIN32__)
// no socklen_t on solaris...
// but must use socklen_t on freebsd to prevent
// compiler warnings
#define socklen_t int
#define setsockoptcast char*
#else
#define setsockoptcast void*
#endif

const char * MsgsTcpInd::ind_names[NUM_IND_TYPES] = {
    "IND_STARTED",
    "IND_CONN_ESTABLISHED",
    "IND_CONN_HAVE_EID",
    "IND_CONN_FAILED",
    "IND_CONN_CLOSED"
};

MessagesTcp :: MessagesTcp( int _indication_mq, int _connid,
                            encrypt_iface * _cr,
                            unsigned long _remote_ip, int _remote_port,
                            bool _verbose )
    : Thread( "msgstcp", MY_PRIO, MY_STACK )
{
    verbose = _verbose;
    connid = _connid;
    remote_ip = _remote_ip;
    remote_port = _remote_port;
    ind_dest.set( _indication_mq );
    cr =_cr;
    fd = -1;
    buf1_size = 0;
    other_eid = -1;
    exit_ind = MsgsTcpInd::IND_CONN_FAILED;

    mbids[ FD_ACTIVE ] = -1;
    mbids[ OTHER_EID ] = -1;
    mbids[ REQ_EID   ] = -1;

    rcvrstate = HEADERHUNT;
    rcvrsubstate = 0;
    rcvrmsglen = 0;
    waiting_for_newkeyack = false;

    UINT32_t * _connest   = (UINT32_t*) header_connest;
    UINT32_t * _msgprefix = (UINT32_t*) header_msgprefix;

    _connest  ->set( MagicNumbers_Messages_ConnEst       );
    _msgprefix->set( MagicNumbers_MessageEncoding_Header );

    resume( tid );
}

MessagesTcp :: ~MessagesTcp( void )
{
    if ( fd != -1 )
        close( fd );

    if ( mbids[ FD_ACTIVE ] != -1 )
        unregister_mq( mbids[ FD_ACTIVE ] );
    if ( mbids[ OTHER_EID ] != -1 )
        unregister_mq( mbids[ OTHER_EID ] );
    if ( mbids[ REQ_EID   ] != -1 )
        unregister_mq( mbids[ REQ_EID   ] );
    if ( other_eid != -1 )
        unregister_eid( other_eid );

    send_indication( exit_ind );
}

void
MessagesTcp :: print( int myerrno, char * format, ... )
{
    if ( !verbose && myerrno == 0 )
        return;

    if ( myerrno == -1 )
        myerrno = 0;

    va_list ap;
    va_start( ap, format );
    _debug_print( myerrno, "MessagesTcp", format, ap );
    va_end( ap );
}

void
MessagesTcp :: send_indication( MsgsTcpInd::ind_type ind )
{
    MsgsTcpInd * mti = new MsgsTcpInd;

    mti->connid.set( connid );
    mti->ind.set( ind );
    mti->other_eid.set( other_eid );

    char * printed_msg = "internal error, unknown ind sent";
    switch ( ind )
    {
    case MsgsTcpInd::IND_STARTED:
        printed_msg = "connection attempt started";
        break;
    case MsgsTcpInd::IND_CONN_ESTABLISHED:
        printed_msg = "connection established";
        break;
    case MsgsTcpInd::IND_CONN_HAVE_EID:
        printed_msg = "ConnEst received from entity %d";
        break;
    case MsgsTcpInd::IND_CONN_FAILED:
        printed_msg = "connection failed";
        break;
    case MsgsTcpInd::IND_CONN_CLOSED:
        printed_msg = "connection closed";
        break;
    default:
        break;
    }

    print( 0, printed_msg, other_eid );

    if ( send( mti, &ind_dest ) == false )
    {
        print( -1, "failure sending indication msg %s\n",
               MsgsTcpInd::ind_names[ind] );
        delete mti;
    }
}

void
MessagesTcp :: send_my_eid( void )
{
    struct {
        UINT32_t ConnEstMagic;
        UINT32_t my_eid;
        UINT32_t req_mid;
    } msg;

    msg.ConnEstMagic.set( MagicNumbers_Messages_ConnEst );
    msg.my_eid.set( my_eid() );
    msg.req_mid.set( mbids[REQ_EID] );

    write( fd, &msg, sizeof( msg ));
}
bool
MessagesTcp :: setup_fd_active_mq( void )
{
    if ( register_fd_mq( fd, FOR_READ, mbids[FD_ACTIVE] ) == false )
    {
        print( -1, "register_fd_mq for fd failed" );
        return false;
    }
    return true;
}

class KillTcp : public Message {
public:
    static const unsigned int TYPE = MagicNumbers_Messages_KillConn;
    KillTcp( void )
        : Message( sizeof( KillTcp ), TYPE ) { }
};

class NewKeyReq : public Message {
public:
    static const unsigned int TYPE = MagicNumbers_Messages_NewKeyReq;
    NewKeyReq( void )
        : Message( sizeof( NewKeyReq ), TYPE ) { }
    // we can send a ptr because this message always goes from
    // one thread to another in the same process
    encrypt_iface * new_cr;
};

class NewKeyReqInt : public Message {
public:
    static const unsigned int TYPE = MagicNumbers_Messages_NewKeyReqInt;
    NewKeyReqInt( void )
        : Message( sizeof( NewKeyReqInt ), TYPE ) { }
    static const int MAX_KEY_LEN = 256;
    char newkeystring[ MAX_KEY_LEN ];
};

class NewKeyAck : public Message {
public:
    static const unsigned int TYPE = MagicNumbers_Messages_NewKeyAck;
    NewKeyAck( void )
        : Message( sizeof( NewKeyAck ), TYPE ) { }
};

class NewKeyAckInt : public Message {
public:
    static const unsigned int TYPE = MagicNumbers_Messages_NewKeyAckInt;
    NewKeyAckInt( void )
        : Message( sizeof( NewKeyAckInt ), TYPE ) { }
};

void
MessagesTcp :: kill( void )
{
    KillTcp * kt = new KillTcp;
    kt->dest.set( 0, mbids[REQ_EID] );
    if ( send( kt, &kt->dest ) == false )
    {
        th->printf( "MessagesTcp :: kill : err sending kill ind\n" );
        delete kt;
    }
}

void
MessagesTcp :: entry( void )
{
    send_indication( MsgsTcpInd :: IND_STARTED );

    if ( register_mq( mbids[FD_ACTIVE], "MsgsTcp FdAct" ) == false )
    { print( -1, "register_mq for active fd failed" ); return; }

    if ( register_mq( mbids[OTHER_EID], "MsgsTcp EidFor" ) == false )
    { print( -1, "register_mq for other eid failed" ); return; }

    if ( register_mq( mbids[REQ_EID], "MsgsTcp ReqEid" ) == false )
    { print( -1, "register_mq for kill eid failed" ); return; }

    fd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( fd < 0 )
    {
        print( errno, "socket" );
        return;
    }

    int siz = 64 * 1024;
    (void)setsockopt( fd, SOL_SOCKET, SO_RCVBUF,
                      (setsockoptcast)&siz, sizeof( siz ));
    (void)setsockopt( fd, SOL_SOCKET, SO_SNDBUF,
                      (setsockoptcast)&siz, sizeof( siz ));
    siz = 1;
    (void)setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
                      (setsockoptcast)&siz, sizeof( siz ));

    sa.sin_family = AF_INET;
    sa.sin_port = htons( remote_port );
    sa.sin_addr.s_addr = htonl( remote_ip );

    if ( remote_ip == INADDR_ANY )
    {
        if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
        { print( errno, "bind" ); return; }

        listen( fd, 1 );
        int out;
        if ( select( 1, &fd, 0, NULL,
                     1, &out, WAIT_FOREVER ) <= 0 )
        { print( errno, "select" ); return; }

        socklen_t salen = sizeof( sa );
        int ear = accept( fd, (struct sockaddr *)&sa, &salen );
        if ( ear < 0 )
        { print( errno, "accept" ); return; }

        close( fd );
        fd = ear;
    }
    else
    {
        // start a connect. note that register_fd makes the
        // fd nonblocking, so the connect should immediately
        // return EINPROGRESS.
        // once the connect starts,
        // we select for both reading and writing.
        // if the select-for-read responds first,
        // then the connect failed.
        // if the select-for-write responds first,
        // then the connect succeeded.

        register_fd( fd );

        if ( connect( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
        {
            if ( errno != EINPROGRESS )
            {
                print( errno, "connect" );
                return;
            }
        }

        int selret;
        int selout;
        selret = select( 1, &fd, 1, &fd,
                         1, &selout, WAIT_FOREVER );

        struct sockaddr_in name;
        socklen_t namelen = sizeof( name );

        if ( getpeername( fd, (struct sockaddr *)&name, &namelen ) < 0 )
            if ( errno == ENOTCONN )
                return;
    }

    send_indication( MsgsTcpInd :: IND_CONN_ESTABLISHED );
    exit_ind = MsgsTcpInd::IND_CONN_CLOSED;

    send_my_eid();

    if ( setup_fd_active_mq() == false ) 
        return;

    bool connection_open = true;
    while ( connection_open )
    {
        union {
            Message     * m;
            FdActiveMessage * fam;
            KillTcp     * ktm;
            NewKeyReq   * nkrm;
            NewKeyReqInt    * nkri;
            NewKeyAckInt    * nkai;
        } m;
        int mbidout;

        m.m = recv( 3, mbids, &mbidout, WAIT_FOREVER );
        if ( m.m == NULL )
        { print( -1, "recv returned null pointer!" ); return; }

        if ( mbidout == mbids[FD_ACTIVE] )
        {
            if ( m.fam->activity.get() ==
                 ThreadMessages::FOR_READ )
            {
                connection_open = handle_fd_data();
                // getting the msg causes deregistration
                // with the msg helper
                if ( connection_open &&
                     ( setup_fd_active_mq() == false ))
                {
                    printf( "setup_fd_act return\n" );
                    return;
                }
            }
            else if ( m.fam->activity.get() ==
                      ThreadMessages::FOR_ERROR )
            {
                print( -1, "error on fd %d", m.fam->fd.get() );
                connection_open = false;
            }
        }
        else if ( mbidout == mbids[OTHER_EID] )
        {
            connection_open = forward_data( m.m );
        }
        else if ( mbidout == mbids[REQ_EID] )
        {
            switch ( m.m->type.get() )
            {
            case KillTcp::TYPE:
                connection_open = false;
                unregister_fd_mq( fd );
                break;

            case NewKeyReq::TYPE:
            {
                NewKeyReqInt nkri;
                char * keystring;

                new_cr = m.nkrm->new_cr;
                keyreq_sender = m.nkrm->src;
                keystring = new_cr->key->key_dump();
                // fill nkri.newkeystring with random
                strncpy( nkri.newkeystring, keystring,
                         NewKeyReqInt::MAX_KEY_LEN );
                delete keystring;
                nkri.dest.set( other_eid, other_req_mid );
                forward_data( &nkri );
                waiting_for_newkeyack = true;
                break;
            }

// actually the next two cases don't trigger, because messages of
// this type are caught in the MESSAGECOLLECT case of rcvr_statemachine()
// before being forwarded. see the comment in rcvr_statemachine().

            case NewKeyReqInt::TYPE:
                handle_newkey_reqint( m.m );
                break;

            case NewKeyAckInt::TYPE:
                handle_newkey_ackint( m.m );
                break;

            default:
                print( -1, "unknown msg type %x rcvd "
                       "on REQ_EID mailbox", 
                       m.m->type.get() );
            }
        }

        delete m.m;
    }
}

void
MessagesTcp :: handle_newkey_reqint( Message * _m )
{
    NewKeyReqInt * nkri = (NewKeyReqInt *)_m;
    new_cr = parse_key( nkri->newkeystring );
    if ( new_cr == NULL )
    {
        // ?
    }
    NewKeyAckInt nkai;
    nkai.dest.set( other_eid, other_req_mid );
    forward_data( &nkai );
    delete cr;
    cr = new_cr;
}

void
MessagesTcp :: handle_newkey_ackint( Message * _m )
{
    if ( waiting_for_newkeyack == false )
    {
        // ?
    }
    NewKeyAck * nka = new NewKeyAck;
    nka->dest = keyreq_sender;
    if ( send( nka, &nka->dest ) == false )
    {
        delete nka;
    }
    delete cr;
    cr = new_cr;
    waiting_for_newkeyack = false;
}

bool
MessagesTcp :: forward_data( Message * m )
{
    if ( cr != NULL )
    {
        encrypt_iface * mycr;
        if ( waiting_for_newkeyack )
            mycr = new_cr;
        else
            mycr = cr;

        if ( m->encrypt( mycr ) == false )
        {
            th->printf( "failure encrypting on forwarding\n" );
            return false;
        }
    }

    if ( th->write( fd, m->get_body(), m->get_size() ) < 0 )
    {
        print( errno, "forward_data : write" );
        return false;
    }

    return true;
}

bool
MessagesTcp :: handle_fd_data( void )
{
    int rcvd_data;

    do {

        rcvd_data = ::read( fd, buffer1, BUFSIZE );
        if ( rcvd_data <= 0 )
            break;

        buf1_size = rcvd_data;
        rcvr_statemachine();

    } while ( rcvd_data == BUFSIZE );

    if ( rcvd_data == 0 )
        return false;

    if ( rcvd_data < 0 )
    {
        if ( errno != EAGAIN )
        {
            print( errno, "handle_fd_data : read" );
            return false;
        }
        /* EAGAIN appears to happen sometimes
           on Windows/CYGWIN -- but its OK,
           don't close the connection. */
    }

    return true;
}

// look for ConnEst and MsgPrefix signatures

void
MessagesTcp :: rcvr_statemachine( void )
{
    char * buf1 = buffer1;
    int tocopy;

    while ( buf1_size > 0 )
    {
        tocopy = -1;
        switch ( rcvrstate )
        {
        case HEADERHUNT:
            buffer2[rcvrsubstate] = *buf1;

            if ( *buf1 == header_connest[ rcvrsubstate ])
                rcvrsubstate++;
            else if ( *buf1 == header_msgprefix[ rcvrsubstate ])
                rcvrsubstate++;
            else
                rcvrsubstate = 0;

            if ( rcvrsubstate == 4 )
            {
                rcvrsubstate = 0;

                switch ( ((UINT32_t *)buffer2)->get() )
                {
                case MagicNumbers_Messages_ConnEst:
                    rcvrstate = EIDCOLLECT;
                    break;

                case MagicNumbers_MessageEncoding_Header:
                    rcvrstate = MSGLENCOLLECT;
                    break;

                default:
                    rcvrstate = HEADERHUNT;
                    break;
                }
            }

            buf1++;
            buf1_size--;
            break;

        case MSGLENCOLLECT:
            tocopy = 4 - rcvrsubstate;
            // fallthru

        case EIDCOLLECT:
            if ( tocopy == -1 )
                tocopy = 8 - rcvrsubstate;

            if ( buf1_size < tocopy )
                tocopy = buf1_size;

            memcpy( buffer2 + 4 + rcvrsubstate, buf1, tocopy );
            buf1 += tocopy;
            buf1_size -= tocopy;
            rcvrsubstate += tocopy;

            if ( rcvrsubstate == 8 &&
                 rcvrstate == EIDCOLLECT )
            {
                rcvrsubstate = 0;
                rcvrstate = HEADERHUNT;
                other_eid = ((UINT32_t *)(buffer2 + 4))->get();
                other_req_mid =
                    ((UINT32_t *)(buffer2 + 8))->get();

                send_indication( MsgsTcpInd::IND_CONN_HAVE_EID );

                if ( register_eid( other_eid,
                                   mbids[ OTHER_EID ] ) == false )
                {
                    print( -1, "register_eid failed!" );
                    // nuke other_eid so we dont
                    // accidentally unregister the
                    // real eid registration when
                    // destructor is called.
                    other_eid = -1;
                }
            }
            else if ( rcvrsubstate == 4 &&
                      rcvrstate == MSGLENCOLLECT )
            {
                rcvrsubstate = 8;
                rcvrmsglen = ((UINT32_t*)(buffer2+4))->get();
                if ( rcvrmsglen < 0 || 
                     rcvrmsglen > ( BUFSIZE - 8 ))
                {
                    print( -1, "message prefix, but "
                           "length field bad (%d)\n",
                           rcvrmsglen );
                    rcvrstate = HEADERHUNT;
                    rcvrsubstate = 0;
                }
                else
                    rcvrstate = MESSAGECOLLECT;
            }
            break;

        case MESSAGECOLLECT:
            tocopy = rcvrmsglen - rcvrsubstate;
            if ( buf1_size < tocopy )
                tocopy = buf1_size;

            memcpy( buffer2 + rcvrsubstate, buf1, tocopy );
            buf1 += tocopy;
            buf1_size -= tocopy;
            rcvrsubstate += tocopy;

            if ( rcvrsubstate == rcvrmsglen )
            {
                rcvrsubstate = 0;
                rcvrstate = HEADERHUNT;
                // message is done, decode + forward
                union {
                    Message * m;
                    char * b;
                } m;
                m.b = new char[rcvrmsglen + 12];

                memcpy( m.m->get_body(), buffer2, rcvrmsglen );

                if ( decryptmsg( m.m ) == false )
                {
                    th->printf( "failure "
                                "decrypting\n" );
                    delete m.b;
                }

// sometimes we can get new key reqs or new key acks in the middle
// of large packets. there is code in the main loop of this application
// for handling these messages, thru the normal recv() method. however,
// the problem is that if more than one message is found in this recent
// burst of data read from the pipe, we don't get back to the mail recv()
// loop until we're done with this packet. but that may be too late, because
// we must switch decryption keys as soon as these messages arive. so, 
// special case code here to process them w/o forwarding them into 
// message system. 

                else if ( m.m->type.get() == NewKeyReqInt::TYPE )
                {
                    handle_newkey_reqint( m.m );
                    delete m.b;
                }
                else if ( m.m->type.get() == NewKeyAckInt::TYPE )
                {
                    handle_newkey_ackint( m.m );
                    delete m.b;
                }

// at this point we can just forward this
// message back into the message system.

                else
                {
                    // zero out the links field, because we didn't
                    // properly construct this Message.  if we had,
                    // the LLinks constructor would have cleaned it
                    // out already.

                    memset( &m.m->links, 0, sizeof( m.m->links ));
                    if ( send( m.m, &m.m->dest ) == false )
                    {
                        print( -1, "resend failed" );
                        delete m.b;
                    }
                }
            }

            break;
        }
    }
}

bool
MessagesTcp :: decryptmsg( Message * m )
{
    if ( cr == NULL )
        return true;

    return m->decrypt( cr );
}

// key switching is delicate and must be timed to perfection. heh.

// this func sends NewKeyReq to msgsTcp; msgsTcp sends NewKeyReqInt over
// link to other msgsTcp. when msgsTcp receives NewKeyReqInt he knows
// next msg he gets will be encrypted with new key instead of old. he
// also sends NewKeyAckInt using old encryption and then next msg he
// sends uses new encryption.  when NewKeyAckInt arrives, msgsTcp sends
// NewKeyAck to this function which has been waiting all along.

// there is a possibility that a message is streamed in behind the
// NewKeyReqInt that this code may attempt to decrypt using the old key
// instead of the new.  this is why rcvr_statemachine() specifically
// looks for the existence of a NewKeyReqInt or NewKeyAckInt in the
// incoming data stream and processes them before continuing to process
// the data in the stream after them.

bool
MessagesTcp :: switch_key( encrypt_iface * newkey )
{
    int mqid;

    if ( register_mq( mqid, "keyswitch reply" ) == false )
    {
        // ?
        return false;
    }

    NewKeyReq * nkr = new NewKeyReq;
    nkr->new_cr = newkey;
    nkr->dest.set( mbids[REQ_EID] );
    nkr->src.set( mqid );

    bool ret = false;
    if ( send( nkr, &nkr->dest ) == true )
    {
        Message * m = recv( 1, &mqid, NULL, tps() * 2 );
        if ( m != NULL )
        {
            if ( m->type.get() != NewKeyAck::TYPE )
                printf( "error, switch_key got type %x\n",
                        m->type.get());
            else
            {
                ret = true;
            }

            delete m;
        }
    }
    else
    {
        printf( "error, switch_key couldn't send\n" );
        delete nkr;
    }

    unregister_mq( mqid );
    return ret;
}
