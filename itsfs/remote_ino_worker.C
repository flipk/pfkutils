/*
 * This code is originally written by Phillip F Knaack.
 * There are absolutely no restrictions on the use of this code.  It is
 * NOT GPL!  If you use this code in a product, you do NOT have to release
 * your alterations, nor do you have to acknowledge in any way that you
 * are using this software.
 * The only thing I ask is this: do not claim you wrote it.  My name
 * should appear in this file forever as the original author, and every
 * person who modifies this file after me should also place their names
 * in this file (so those that follow know who broke what).
 * This restriction is not enforced in any way by any license terms, it
 * is merely a personal request from me to you.  If you wanted, you could
 * even completely remove this entire comment and place a new one with your
 * company's confidential proprietary license on it-- but if you are a good
 * internet citizen, you will comply with my request out of the goodness
 * of your heart.
 * If you do use this code and you make a buttload of money off of it,
 * I would appreciate a little kickback-- but again this is a personal
 * request and is not required by any licensing of this code.  (Of course
 * in the offchance that anyone every actually DOES make a lot of money 
 * using this code, I know I'm going to regret that statement, but at
 * this point in time it feels like the right thing to say.)
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include "remote_ino.H"
#include "config.h"

extern "C" {
    int inet_aton( char *, struct in_addr * );
    int itsfsriw_main( int argc, char ** argv );
};

jmp_buf signal_buffer;

void
sighand( int s )
{
    longjmp( signal_buffer, -1 );
}

int
itsfsriw_main( int argc, char ** argv )
{
    bool verbose = false;
    bool symlinks = true;
    bool dirsymlinks = true;
    bool checkparent = true;

    char * server = getenv( "ITSFS_SERVER_IPADDR" );
    if ( !server )
    {
        printf( "please setenv ITSFS_SERVER_IPADDR\n" );
        return -1;
    }

    if ( argc == 1 )
    {
    usage:
        printf( "usage: itsfsriw [-nolinks -nodirlinks -verbose -ncp] <fsname>\n" );
        return -1;
    }

    argc--;
    argv++;

    while ( argc > 1 )
    {
        if ( argv[0][0] != '-' )
            goto usage;
        if ( strcmp( argv[0], "-nolinks" ) == 0 )
            symlinks = false;
        else if ( strcmp( argv[0], "-nodirlinks" ) == 0 )
            dirsymlinks = false;
        else if ( strcmp( argv[0], "-verbose" ) == 0 )
            verbose = true;
        else if ( strcmp( argv[0], "-ncp" ) == 0 )
            checkparent = false;
        else
            goto usage;
        argc--;
        argv++;
    }

    if ( argv[0][0] == '-' )
        goto usage;

    struct in_addr ad;
    if ( !inet_aton( server, &ad ))
    {
        printf( "inet bailed\n" );
        return -1;
    }

    remote_inode_server_tcp svr( htonl( ad.s_addr ),
                                 SLAVE_PORT,
                                 (uchar*)argv[0],
                                 verbose, symlinks, dirsymlinks );
    char logpath[ 200 ];
    FILE * fd;
    char * home = getenv( "HOME" );
    char * host = getenv( "HOST" );
    char * root = getenv( "CLEARCASE_ROOT" );
    char * pwd  = getenv( "PWD" );

    if ( !home ) home = "nohome";
    if ( !host ) host = "unknownhost";
    if ( !root ) root = "";
    if ( !pwd  ) pwd  = "unknownpwd";

    sprintf( logpath, "%s/.y.itsfsriw.%s.%d", home, host, getpid() );
    fd = fopen( logpath, "w" );
    if ( fd )
    {
        fprintf( fd, "itsfsriw,%s,%d,p%d,%s,%s\n",
                 host, getpid(), getppid(), root, pwd );
        fclose( fd );
    }
    signal( SIGINT,  sighand );
    signal( SIGHUP,  sighand );
    signal( SIGTERM, sighand );
    if ( setjmp( signal_buffer ) == 0 )
        svr.dispatch_loop( checkparent );
    unlink( logpath );

    return 0;
}
