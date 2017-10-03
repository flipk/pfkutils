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

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#include "threads.H"

#define KEY "rubik4:74848ea6646f9b010f58f2d3,32,088f2a2f32e1d83533c895ae1f242dd1308a2bd53f0227947c60401b4bfe1d1e65ccf8b9371d40f63e0c434a143ebd3e1d15d4ae2bf8705516111b9033a4ac19144803584e0ba46b39dbf64c24dcc2c71898cbd31fef39004cadfcd42697fdd00112542f3929fb54383163563354dde367a59ad341a2b67130a705527034c502"

class creatorThread : public Thread {
    void entry( void );
    encrypt_iface * cr;
    unsigned int ipaddr;
    int port;
public:
    creatorThread( unsigned int _ipaddr, int _port,
                   encrypt_iface * _cr )
        : Thread( "creator" ) {
        ipaddr = _ipaddr;
        port = _port;
        cr = _cr;
        resume( tid );
    };
    ~creatorThread( void ) { 
        delete cr;
    };
};

int
main( int argc, char ** argv )
{
    bool active;

    if ( argc != 2 )
    {
        printf( "usage: %s [a | p]\n", argv[0] );
        return 1;
    }

    active = (argv[1][0] == 'a');

    encrypt_iface * cr = NULL;

    cr = parse_key( KEY );
    if ( cr == NULL )
    {
        printf( "parse key error\n" );
        return 1;
    }

    ThreadParams p;
    p.debug = 1;
    p.my_eid = active ? 1 : 2;
    Threads th( &p );

    if ( active )
        (void) new creatorThread( 0x0a000002, 2400, cr ); 
//      (void) new creatorThread( 0xa016820e, 2400, cr ); 
//      (void) new creatorThread( 0x7f000001, 2400, cr );
    else
        (void) new creatorThread( INADDR_ANY, 2400, cr );

    th.loop();
    return 0;
}

class creatorAbort : public Message {
public:
    static const unsigned int TYPE = 0xe95b78ee;
    creatorAbort( void )
        : Message( sizeof( creatorAbort ), TYPE ) { }
};

class exersizeThread : public Thread { 
    void entry( void );
    int other_eid;
    bool active;
public:
    exersizeThread( int _other_eid, bool _active )
        : Thread( "excersize" ) {
        other_eid = _other_eid;
        active = _active;
        resume( tid );
    };
    ~exersizeThread( void ) {
        int mqid;
        if ( lookup_mq( 0, mqid, "creator indications" ) == false )
            printf( "couldn't find mq\n" );
        if ( active )
        {
            creatorAbort * ca = new creatorAbort;
            ca->dest.set( mqid );
            if ( send( ca, &ca->dest ) == false )
            {
                printf( "couldn't send creatorAbort\n" );
                delete ca;
            }
        }
    }
};

void
creatorThread :: entry( void )
{
    int indications_mq;

    if ( register_mq( indications_mq, "creator indications" ) == false )
    {
        printf( "failure registering\n" );
        return;
    }

    MessagesTcp * mtp = new MessagesTcp( indications_mq, 1, cr,
                                         ipaddr, port, true );

    bool done = false;
    while ( !done )
    {
        union {
            Message * m;
            MsgsTcpInd * mti;
        } m;

        m.m = recv( 1, &indications_mq, NULL, WAIT_FOREVER );

        switch ( m.m->type.get() )
        {
        case MsgsTcpInd::TYPE:
            switch ( m.mti->ind.get() )
            {
            case MsgsTcpInd::IND_STARTED:
                printf( "connection started\n" );
                break;
            case MsgsTcpInd::IND_CONN_ESTABLISHED:
                printf( "connection established!\n" );
                break;
            case MsgsTcpInd::IND_CONN_HAVE_EID:
                printf( "connection is to eid %d\n", 
                        m.mti->other_eid.get() );
                sleep( tps() / 2 );
                (void) new exersizeThread(
                    m.mti->other_eid.get(),
                    !(ipaddr == INADDR_ANY) );
                break;
            case MsgsTcpInd::IND_CONN_FAILED:
                printf( "connection failed!\n" );
                done = true;
                break;
            case MsgsTcpInd::IND_CONN_CLOSED:
                printf( "connection closed\n" );
                done = true;
                break;
            default:
                break;
            }
            break;
        case creatorAbort::TYPE:
            printf( "aborting TCP connection\n" );
            mtp->kill();
            break;
        default:
            printf( "unknown msg type %x\n", m.m->type.get() );
        }
        delete m.m;
    }
}

class exorMsg : public Message {
public:
    static const unsigned int TYPE = 0xbf735756;
    exorMsg( void ) : Message( sizeof( exorMsg ), TYPE ) { }
    UINT32_t v;
};

void
exersizeThread :: entry( void )
{
    int mqid;

    if ( active )
    {
        if ( register_mq( mqid, "exersize" ) == false )
            printf( "register exersize failed\n" );
        printf( "exersize mailbox is mqid %d\n", mqid );
        union {
            Message * m;
            exorMsg * em;
        } m;
        m.m = recv( 1, &mqid, NULL, WAIT_FOREVER );
        if ( m.m->type.get() != exorMsg::TYPE )
            printf( "got msg type %x\n", m.m->type.get() );
        else
            printf( "got exersize message, v = %d\n",
                    m.em->v.get() );
        delete m.m;
    }
    else
    {
        sleep( tps() / 2 );
        if ( lookup_mq( other_eid, mqid, "exersize" ) == false )
            printf( "exersize mailbox not found\n" );
        printf( "excersize mailbox at eid %d is mqid %d\n", 
                other_eid, mqid );
        exorMsg * m = new exorMsg;
        m->dest.set( other_eid, mqid );
        m->v.set( random() );
        printf( "sending exormsg to eid %d mqid %d v %d\n",
                other_eid, mqid, m->v.get() );
        if ( send( m, &m->dest ) == false )
        {
            printf( "couldn't send msg\n" );
            delete m;
        }
    }
}
