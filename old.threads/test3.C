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
    p.debug      = ThreadParams::DEBUG_MSGSCONTENTS;

    Threads th( &p );
    new testThread;
    th.loop();

    return 0;
}

void
testThread :: entry( void )
{
    MessagesUdp * mu = new MessagesUdp( 1, 0, my_port, true );
    mu->register_entity( other_ip, other_port, other_eid );

    int mqid, other_mq;

    if ( register_mq( mqid, "test_mq" ) == false )
    {
        printf( "unable to register mq!\n" );
        return;
    }

    printf( "start sleep 1\n" );
    sleep( 30 );
    printf( "end sleep one, looking up test_mq on other side\n" );

    if ( lookup_mq( other_eid, other_mq, "test_mq" ) == false )
    {
        printf( "unable to lookup other mq\n" );
    }
    else
    {
        printf( "sending test message to other mq %d at eid %d\n",
                other_mq, other_eid );
    }

    printf( "lookup complete; starting sleep 2\n" );
    sleep( 10 );
    printf( "ending sleep 2, shutting down\n" );

    mu->unregister_entity( other_eid );

    printf( "unregister complete, starting sleep 3\n" );
    sleep( 10 );
    printf( "ending sleep 3\n" );

    mu->kill();
}
