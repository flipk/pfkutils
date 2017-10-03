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

//   t3 <my_eid> <my_port> <other_eid> <other_ip> <other_port>

class testThread : public Thread {
    void entry( void );
public:
    testThread( void )
        : Thread( "test", 10, 16384 ) { resume( tid ); }
};

int my_eid;
int my_port;
int other_eid;
int other_ip;
int other_port;

int
main( int argc, char ** argv )
{
    if ( argc != 6 )
    {
        printf( "usage: t3 <my_eid> <my_port> \n"
                "          <other_eid> <other_ip> <other_port>\n" );
        return 1;
    }

    my_eid     = strtoul( argv[1], 0, 0 );
    my_port    = strtoul( argv[2], 0, 0 );
    other_eid  = strtoul( argv[3], 0, 0 );
    other_ip   = strtoul( argv[4], 0, 0 );
    other_port = strtoul( argv[5], 0, 0 );

    ThreadParams   p;
    p.my_eid     = my_eid;
    p.max_eids   = 10;
//    p.debug      = ThreadParams::DEBUG_MSGSCONTENTS;

    Threads th( &p );
    new testThread;
    th.loop();

    return 0;
}

MsgDef( TestMsg, 0x2a145981,
        int val;
    );
MsgDef( TestDataMsg, 0x2a145982,
        int len;
        char string[80];
    );

void
testThread :: entry( void )
{
    union {
        Message * m;
        TestMsg * tm;
        TestDataMsg * tdm;
    } m;

    encrypt_iface * cr = parse_key( "rubik4:"
                                    "0e37adb1631180b8326d6148,5,"
                                    "12666fc65963b5fa531e"
                                    "321d6153961e5abed732" );

    MessagesUdp * mu = new MessagesUdp( 1, cr, my_port, false );
    mu->register_entity( other_ip, other_port, other_eid );

    int mqid, other_mq;

    if ( register_mq( mqid, "test_mq" ) == false )
    {
        printf( "unable to register mq!\n" );
        return;
    }

    sleep( 30 );

    if ( lookup_mq( other_eid, other_mq, "test_mq" ) == false )
        printf( "unable to lookup other mq\n" );
    else
    {
        m.tm = new TestMsg;
        m.tm->dest.set( other_eid, other_mq );
        m.tm->val = random();
        printf( "sending test message to other mq %d at eid %d val %d\n",
                other_mq, other_eid, m.tm->val );
        if ( send( m.m, &m.m->dest ) == false )
        {
            printf( "unable to send message to other entity!\n" );
            delete m.m;
        }
    }

    bool zero_regd = false;
    bool done = false;

    register_fd( 0 );

    while ( !done )
    {
        int mqout;
        TestDataMsg * tdmout;

        if ( !zero_regd )
        {
            register_fd_mq( 0, 0, FOR_READ, mqid );
            zero_regd = true;
        }

        m.m = recv( 1, &mqid, &mqout, WAIT_FOREVER );
        if ( m.m == NULL )
            break;

        switch ( m.m->type.get() )
        {
        case FdActiveMessage::TYPE:
            tdmout = new TestDataMsg;
            tdmout->dest.set( other_eid, other_mq );
            tdmout->len = ::read( 0, tdmout->string, sizeof( tdmout->string ));
            if ( tdmout->len == 0 )
                done = true;
            send( tdmout, &tdmout->dest );
            zero_regd = false;
            break;

        case TestDataMsg::TYPE:
            write( 1, m.tdm->string, m.tdm->len );
            if ( m.tdm->len == 0 )
                done = true;
            break;

        case TestMsg::TYPE:
            printf( "received test message with val = %d\n",
                    m.tm->val );
            break;
        default:
            printf( "message unknown type %#x received\n",
                    m.m->type.get() );
        }

        delete m.m;
    }

    mu->unregister_entity( other_eid );

    sleep( 10 );

    mu->kill();
}
