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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include "remote_ino.H"
#include "config.h"

extern "C" {
    int inet_aton( char *, struct in_addr * );
    int itsfsriw_main( int argc, char ** argv );
};

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
    svr.dispatch_loop( checkparent );

    return 0;
}
