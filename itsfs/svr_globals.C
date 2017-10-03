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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>

#include "inode_remote.H"
#include "inode_virtual.H"
#include "nfssrv.H"
#include "config.h"
#include "svr.H"

treeinfo :: treeinfo( void )
{
    init();
}

void
treeinfo :: init( void )
{
    slave_tcp_fd = 0;
    treenumber = 0;
    treeid = 0; 
    inort = NULL;
    treename = NULL;
}

void
treeinfo :: setup( char * newfsname, int newfd, int _treenumber, 
                   Inode_remote_tree * irt, int _treeid )
{
    int newfsnamelen;

    newfsnamelen = strlen( newfsname ) + 1;
    treename = (char*) malloc( newfsnamelen );
    memcpy( treename, newfsname, newfsnamelen );

    slave_tcp_fd = newfd;
    treenumber = _treenumber;
    inort = irt;
    treeid = _treeid;
}

void
treeinfo :: teardown( void )
{
    free( treename );
    init();
}

bool
treeinfo :: valid( void )
{
    return (inort != NULL);
}

void
treeinfo :: sprintinfo( char * f )
{
    sprintf( f,
             "mount: treeid %d treeid %d on fd %d name %s\n",
             treenumber, treeid, slave_tcp_fd, treename );
}

void
treeinfo :: printcache( void )
{
    inort->print_cache( 0, itsfs_print_func );
}




svr_globals :: svr_globals( nfssrv * _server, Inode_virtual_tree * _itv )
{
    int i;
    nfs_rpc_udp_fd = 0;
    slave_rendevous_fd = 0;
    for ( i = 0; i < maxvs; i++ )
        trees[i].init();
    source_port = 0;
    nfs_rpc_call_count = 0;
    exit_command = 0;
    close_command = NULL;
    gettimeofday( &starttime, NULL );
    server = _server;
    itv = _itv;
}

svr_globals :: ~svr_globals( void )
{
    for ( int i = 0; i < maxvs; i++ )
    {
        if ( trees[i].valid())
            kill_tree( i );
    }
}

void
svr_globals :: set_close( char * cmd, int len )
{
    close_command = (char*)malloc( len );
    memcpy( close_command, cmd + 6, len - 7 );
    close_command[ len - 7 ] = 0;
}

void
svr_globals :: print_cache( void )
{
    for ( int i = 0; i < maxvs; i++ )
        if ( trees[i].valid())
            trees[i].printcache();
}

void
svr_globals :: sprintinfo( char * f )
{
    for ( int i = 0; i < maxvs; i++ )
        if ( trees[i].valid() )
        {
            trees[i].sprintinfo( f );
            f += strlen( f );
        }
}

int
svr_globals :: new_index( void )
{
    for ( int i = 0; i < maxvs; i++ )
        if ( !trees[i].valid())
            return i;
    return -1;
}

void
svr_globals :: kill_tree( int i )
{
    itv->kill_tree( trees[i].treeid );
    server->unregister_tree( trees[i].treenumber );
    trees[i].teardown();
}

int
svr_globals :: fdset( fd_set * rfds )
{
    int max = 0;
    FD_ZERO( rfds );
#define SET(fd)  FD_SET(fd,rfds); if ( fd >= max ) max = fd+1
    SET( nfs_rpc_udp_fd );
    SET( slave_rendevous_fd );
    for ( int i = 0; i < maxvs; i++ )
        if ( trees[i].valid() && !trees[i].inort->bad_fd )
        {
            SET( trees[i].slave_tcp_fd );
        }
#undef  SET
    return max;
}

void
svr_globals :: checkfds( fd_set * rfds )
{
    int i;
    for ( i = 0; i < maxvs; i++ )
        if ( trees[i].valid() )
            if ( FD_ISSET( trees[i].slave_tcp_fd, rfds ))
            {
                /* we should never read data here! the client
                   only speaks when its spoken to. boot it if
                   it makes noise, or this can also happen if
                   the connection from the client was closed. */
                itsfs_addlog( "killing tree '%s' because of dead fd",
                              trees[i].treename );
                kill_tree( i );
            }
}

void
svr_globals :: check_bad_fds( void )
{
    for ( int i = 0; i < maxvs; i++ )
    {
        if ( trees[i].valid() && trees[i].inort->bad_fd )
        {
            itsfs_addlog( "killing tree '%s' because remote fd is dead",
                          trees[i].treename );
            kill_tree( i );
        }
    }
}

int
svr_globals :: findname( char * f )
{
    int i;
    for ( i = 0; i < maxvs; i++ )
        if ( trees[i].treename && strcmp( f, trees[i].treename ) == 0 )
            return i;
    return -1;
}
