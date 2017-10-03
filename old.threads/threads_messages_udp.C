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

struct MessagesUdp::other_eid_defs {
    int mqid;
    struct sockaddr_in sa;

    other_eid_defs( void ) { mqid = -1; }
    ~other_eid_defs( void ) { shutdown(); }
    bool init( int eid, unsigned long ip, short port ) {
        char mqname[32];
        sprintf( mqname, "udp eid %d", eid );
        if ( ThreadShortCuts::register_mq( mqid, mqname ) == false ) {
            ThreadShortCuts::printf( "unable to register eid mailbox!\n" );
            return false;
        }
        sa.sin_family = AF_INET;
        sa.sin_port = htons( port );
        sa.sin_addr.s_addr = htonl( ip );
        return true;
    }
    void shutdown( void ) {
        if ( mqid != -1 )
            ThreadShortCuts::unregister_mq( mqid );
        mqid = -1;
    }
};

MessagesUdp :: MessagesUdp( int _connid, encrypt_iface * _cr, 
                            int port, bool _verbose )
    : Thread( "msgsudp", MY_PRIO, MY_STACK )
{
    connid = _connid;
    cr = _cr;
    verbose = _verbose;
    die = false;

    others = new other_eid_defs[ get_max_eids() ];
    mqids = new int[ get_max_eids() ];
    nummqids = 0;

    fd = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( fd < 0 )
    {
        printf( "ERROR: socket: %s\n", strerror( errno ));
        die = true;
        goto bail;
    }
    // else

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons( port );
    sa.sin_addr.s_addr = INADDR_ANY;

    if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        printf( "ERROR: bind: %s\n", strerror( errno ));
        die = true;
        goto bail;
    }
    // else

    if ( register_mq( mqids[0], "msgs_udp_fdact" ) == false )
    {
        printf( "ERROR: create msgs_udp_fdact mq\n" );
        die = true;
        goto bail;
    }
    // else

    // note entry 0 is always in use for my own mq.
    nummqids++;

 bail:
    resume( tid );
}

MessagesUdp :: ~MessagesUdp( void )
{
    if ( fd > 0 )
        close( fd );
    delete[] others;
    delete[] mqids;
}

MsgDef( ConnectIndication, MagicNumbers_Messages_conn_ind,
        UINT32_t my_eid;
        UINT32_t your_eid;
    );
MsgDef( DisconnectIndication, MagicNumbers_Messages_discon_ind,
        UINT32_t my_eid;
        UINT32_t your_eid;
    );

void
MessagesUdp :: entry( void )
{
    bool fd_registered = false;

    while ( !die )
    {
        if ( fd_registered == false )
        {
            fd_registered = true;
            register_fd_mq( fd, (void*)fd, FOR_READ, mqids[0] );
        }

        union {
            Message         * m;
            FdActiveMessage * fam;
        } m;
        int mqout;

        m.m = recv( nummqids, mqids, &mqout, WAIT_FOREVER );

        if ( m.m == NULL )
            continue;

        if ( m.m->type.get() == FdActiveMessage::TYPE )
        {
            read_packet();
            fd_registered = false;
        }
        else
        {
            // get a message on an eid mailbox, look up the eid #
            // and send to the appropriate destination udp address.

            other_eid_defs * oed = &others[ m.m->dest.eid.get() ];

            if ( oed->mqid != -1 )
            {
                if ( cr != NULL )
                    m.m->encrypt( cr );

                if ( verbose )
                    printf( "transmitting message of size %d tag %#x "
                            "eid %d mid %d\n",
                            m.m->get_size(), m.m->type.get(),
                            m.m->dest.eid.get(),
                            m.m->dest.mid.get() );

                sendto( fd, m.m->get_body(), m.m->get_size(), 0,
                        (struct sockaddr *)&oed->sa, sizeof( oed->sa ));
            }
        }

        delete m.m;
    }
}

void
MessagesUdp :: read_packet( void )
{
    // get a message on the fd, enter it back into the message
    // system.  

    // if i get a connect indication, print out that we have
    // a companion.  if i get a disconnect indication, print out
    // that the companion is gone.

    union {
        Message              * m;
        ConnectIndication    * ci;
        DisconnectIndication * di;
        char                 * b;
    } m;

    int cc = read( fd, body, sizeof( body ));
    if ( cc <= 0 )
    {
        printf( "error reading from udp socket: %s\n", strerror( errno ));
        return;
    }

    m.b = new char[ cc ];
    memcpy( m.m->get_body(), body, cc );

    if ( cr )
        if ( m.m->decrypt( cr ) == false )
        {
            printf( "unable to decrypt received message\n" );
            delete m.b;
            return;
        }

    if ( m.m->type.get() == ConnectIndication::TYPE )
    {
        printf( "Connect indication from eid %d to %d (I am %d)\n",
                m.ci->my_eid.get(), m.ci->your_eid.get(), my_eid() );
        delete m.b;
    }
    else if ( m.m->type.get() == DisconnectIndication::TYPE )
    {
        printf( "Disconnect indication from eid %d to %d (I am %d)\n",
                m.di->my_eid.get(), m.di->your_eid.get(), my_eid() );
        delete m.b;
    }
    else
    {
        if ( verbose )
            printf( "forwarding message of size %d type %#x to eid %d mq %d\n",
                    cc, m.m->type.get(),
                    m.m->dest.eid.get(),
                    m.m->dest.mid.get() );

        memset( &m.m->links, 0, sizeof( m.m->links ));
        if ( send( m.m, &m.m->dest ) == false )
        {
            printf( "resend of msg failed\n" );
            delete m.b;
        }
    }
}

bool
MessagesUdp :: register_entity( unsigned long remote_ip,
                                int remote_port,
                                int eid )
{
    // add the ip/port to local control structures.
    // send a connect indication to remote guy.

    other_eid_defs * oed = &others[eid];

    if ( oed->mqid != -1 )
    {
        printf( "MessagesUdp :: register_entity: entity %d is already "
                "registered!\n", eid );
        return false;
    }

    if ( oed->init( eid, remote_ip, remote_port ) == false )
    {
        printf( "MessagesUdp :: register_entity: entity %d failed to "
                "initialize\n", eid );
        return false;
    }

    if ( register_eid( eid, oed->mqid ) == false )
    {
        printf( "MessagesUdp :: register_entity: entity %d failed to "
                "register\n", eid );
        oed->shutdown();
        return false;
    }

    mqids[nummqids++] = oed->mqid;

    ConnectIndication * ci = new ConnectIndication;
    ci->dest.set( eid, 0 );
    ci->my_eid.set( my_eid() );
    ci->your_eid.set( eid );

    if ( send( ci, &ci->dest ) == false )
    {
        printf( "MessagesUdp :: register_entity: failed to send "
                "ConnectIndication to eid %d!\n", eid );
        delete ci;
    }

    resume( tid );
    return true;
}

bool
MessagesUdp :: unregister_entity( int eid )
{
    // send a disconnect indication to remote guy.
    // delete the ip/port from local control structures.

    other_eid_defs * oed = &others[eid];

    if ( oed->mqid == -1 )
    {
        printf( "MessagesUdp :: unregister_entity: entity %d is not "
                "currently registered!\n", eid );
        return false;
    }

    DisconnectIndication * ci = new DisconnectIndication;
    ci->dest.set( eid, 0 );
    ci->my_eid.set( my_eid() );
    ci->your_eid.set( eid );

    if ( send( ci, &ci->dest ) == false )
    {
        printf( "MessagesUdp :: register_entity: failed to send "
                "DisconnectIndication to eid %d!\n", eid );
        delete ci;
    }

    if ( unregister_eid( eid ) == false )
    {
        printf( "MessagesUdp :: unregister_entity: entity %d "
                "failed to unregister!\n", eid );
        return false;
    }

    int i;
    for ( i = 1; i < nummqids; i++ )
        if ( mqids[i] == oed->mqid )
            break;

    if ( i < nummqids )
    {
        if ( i != ( nummqids-1 ))
            mqids[i] = mqids[nummqids-1];
        nummqids--;
    }

    oed->shutdown();
    resume( tid );
    return true;
}

void
MessagesUdp :: kill( void )
{
    die = true;
    resume( tid );
}
