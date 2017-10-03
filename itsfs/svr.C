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
#include <sys/wait.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>

#include "inode_remote.H"
#include "inode_virtual.H"
#include "nfssrv.H"
#include "config.h"
#include "svr.H"
#include "id_name_db.H"
#include "lognew.H"

static bool    check_ascii         ( uchar *, int );
static uchar * update_status_info  ( void );
static void    initlog             ( void );

void
itsfs_print_func( int arg, char * format, ... )
{
    va_list ap;
    va_start( ap, format );
    vprintf( format, ap );
    va_end( ap );
}

static svr_globals * _globs;

extern "C" {
    int itsfssvr_main( int argc, char ** argv );
    extern pid_t do_mount( char * handle );
    extern void do_unmount( void );
};

static char command_help_string[] =
"\n"
"   itsfssvr commands:\n"
"\n"
"   exit            exit the server\n"
"   close <dir>     close mount <dir>\n"
"   pc              print the cache from various mounted trees\n"
"\n"
"   cat 'status' file for mounted filesystems;\n"
"   echo the above commands strings into 'command'\n"
"   to perform commands;\n"
"   see the program 'itsfsriw' to mount filesystems.\n"
"\n"
"   this program written by Phillip F Knaack <pknaack1@email.mot.com>.\n"
"\n";

static void
command_handler( char * cmd, int len )
{
    if ( strncmp( cmd, "exit", 4 ) == 0 )
    {
        seteuid( 0 );
        do_unmount();
        seteuid( getuid() );
    }
    else if ( strncmp( cmd, "close ", 6 ) == 0 )
    {
        _globs->set_close( cmd, len );
    }
    else if ( strncmp( cmd, "pc", 2 ) == 0 )
    {
        _globs->print_cache();
    }
    else
        itsfs_addlog( "unknown command!" );
}

static void
sigusr1_handler( int s )
{
    _globs->exit_command = 1;
}

static void
sigint_handler( int s )
{
    seteuid( 0 );
    do_unmount();
    seteuid( getuid() );
}

extern "C" void  print_malloc( char *s );
extern "C" void sprint_malloc( char *s );

#ifdef LOG_PACKETS
static void
log_packet( char * s, uchar * buf, int len )
{
    int i;
    printf( "*** packet %s:", s );
    for ( i = 0; i < len; i++ )
    {
        if (( i % 16 ) == 0 )
            printf( "\n*** 0x%03x: ", i );
        printf( "%02x ", buf[i] );
    }
    printf( "\n" );
}
#else
#define log_packet(s,buf,len) /* nothing */
#endif

int _itsfssvr_main( int argc, char ** argv );

int
itsfssvr_main( int argc, char ** argv )
{
    int ret;
    ret = _itsfssvr_main( argc, argv );
    print_malloc( "shutdown" );
    return ret;
}

id_name_db * inode_name_db;

int
_itsfssvr_main( int argc, char ** argv )
{
    encrypt_iface * crypt;
    Inode_tree * it;
    Inode_remote_tree * irt;
    Inode_virtual_tree * itv;
    int treenumber;
    pid_t mount_pid;

#ifdef REALLY_RANDOM
    srandom( time( NULL ) * getpid() );
#else
    srandom( 0 );
#endif

    // revoke root priviledges

    seteuid( getuid() );
    setegid( getgid() );

#ifdef USE_CRYPT
    // the only way to make a "random" key of a specific type is to 
    // make a real key and then randomize it.
    crypt = parse_key( "rubik4:000000000000000000000000,1,00000000" );
    crypt->key->random_key( 32 );
#else
    crypt = NULL;
#endif

    id_name_db indb;
    inode_name_db = &indb;

    // construct server and root inode first,
    // and globs last, so that they'll be destroyed 
    // in reverse order.  globs destructor will attempt
    // to delete and unregister remaining trees, so it'd
    // be nice if the virtual tree and server still existed
    // when it tried.

    nfssrv server( crypt );

    treenumber = server.reserve_treeid();

    itv = LOGNEW Inode_virtual_tree( treenumber, update_status_info,
                                     command_handler, command_help_string );

    it = itv;
    server.register_tree( treenumber, it );

    svr_globals globs( &server, itv );
    _globs = &globs;

    initlog();

    signal( SIGPIPE, SIG_IGN         );
    signal( SIGUSR1, sigusr1_handler );
    signal( SIGINT,  sigint_handler  );
    signal( SIGTERM, sigint_handler  );

    // create nfs and rendevous sockets.

    globs.nfs_rpc_udp_fd = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( globs.nfs_rpc_udp_fd < 0 )
    {
        printf( "unable to create socket\n" );
        return -1;
    }

    globs.slave_rendevous_fd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( globs.slave_rendevous_fd < 0 )
    {
        printf( "unable to create socket\n" );
        return -1;
    }

    struct sockaddr_in addr;
    int len;
    int flag = 1;

    // bind the sockets to the appropriate port numbers.

    memset( (void*)&addr, 0, sizeof( addr ));
    addr.sin_len = len;
    addr.sin_family = AF_INET;

    addr.sin_port = htons( SERVER_PORT );
    len = sizeof( addr );

#if defined(SOLARIS)
    setsockopt( globs.nfs_rpc_udp_fd, SOL_SOCKET, SO_REUSEADDR,
                (char*)&flag, sizeof( flag ));
#else
    setsockopt( globs.nfs_rpc_udp_fd,
                SOL_SOCKET, SO_REUSEADDR, &flag, sizeof( flag ));
#endif

    if ( bind( globs.nfs_rpc_udp_fd, (struct sockaddr *)&addr, len ) < 0 )
    {
        printf( "bind 1 failed with error %d\n", errno );
        return -1;
    }

    addr.sin_port = htons( SLAVE_PORT );
    len = sizeof( addr );

#if defined(SOLARIS)
    setsockopt( globs.slave_tcp_fd, SOL_SOCKET, SO_REUSEADDR,
                (char*)&flag, sizeof( flag ));
#else
    setsockopt( globs.slave_rendevous_fd,
                SOL_SOCKET, SO_REUSEADDR, &flag, sizeof( flag ));
#endif

    if ( bind( globs.slave_rendevous_fd, (struct sockaddr *)&addr, len ) < 0 )
    {
        printf( "bind 2 failed with error %d\n", errno );
        return -1;
    }
    listen( globs.slave_rendevous_fd, 1 );

    {
        // generate root directory file handle from virtual tree
        FileHandle fh;
        nfs_fh root_fh_buf;

        fh.userid.set( getuid() );
        fh.tree_id.set( treenumber );
        fh.file_id.set( it->ROOT_INODE_ID );
        fh.mount_id.set( it->mount_id );

        memset( &root_fh_buf, 0xee, sizeof( root_fh_buf ));
        fh.encode( crypt, &root_fh_buf );

        // switch to root userid, mount the virtual tree,
        // then revoke root privs again.

        if ( seteuid( 0 ) < 0 )
        {
            int e = errno;
            printf( "switching to root failed: %s\n",
                    strerror( e ));
            if ( e == EPERM )
                printf( "*** this executable must be setuid root\n" );
            exit( 1 );
        }

        mount_pid = do_mount( (char*) &root_fh_buf.data );
        seteuid( getuid() );
    }

    int i;
    int clean_time = time( NULL );
    bool source_portset = false;

    while ( 1 )
    {
        fd_set rfds;
        struct timeval tv;
        int r, max, now;

        // set a bit in rfds for each virtual tree which is 
        // currently registered, as well as one for the nfs port
        // and one for the slave rendevous port.

        max = globs.fdset( &rfds );

        if ( indb.need_periodic_purge() )
        {
            tv.tv_sec = 0;
            tv.tv_usec = 50000;
        }
        else
        {
            tv.tv_sec = 1;
            tv.tv_usec = 0;
        }

        r = select( max, &rfds, NULL, NULL, &tv );

        // when we get sigusr1, we exit.  this is sent by the
        // unmounter, if unmounting occurred successfully.

        if ( globs.exit_command )
            break;

        if ( r == 0 )
        {
            int child_status;
            pid_t waitret;

            // the do_mount creates a child that must be
            // reaped at some point.
            if ( mount_pid > 0 )
            {
                waitret = waitpid( mount_pid, &child_status, WNOHANG );
                if ( waitret > 0 )
                {
                    mount_pid = 0;
                    if ( child_status != 0 )
                    {
                        printf( "do_mount failed!! status = %#x\n",
                                child_status );
                        break;
                    }
                }
            }

            if ( indb.need_periodic_purge() )
                indb.periodic_purge();
        }
        // periodically clean the server's cache

        now = time( NULL );
        if (( now - clean_time ) > 2 )
        {
            clean_time = now;
            server.clean();
            globs.check_bad_fds();
            indb.flush();
        }

        if ( r < 0 )
        {
            itsfs_addlog( "select failed with error %d", errno );
            continue;
        }

        // if any virtual tree slave worker is misbehaving or has
        // closed its tcp port, delete the virtual tree.

        globs.checkfds( &rfds );

        // process any nfs requests

        if (FD_ISSET(globs.nfs_rpc_udp_fd,&rfds))
        {
            int packlen;
            uchar rcvbuf[ 9000 ];
            socklen_t fromlen;

            // receive the UDP packet from the NFS client.

            fromlen = sizeof( addr );
            packlen = recvfrom( globs.nfs_rpc_udp_fd,
                                &rcvbuf, 9000, /* flags */ 0,
                                (struct sockaddr *)&addr, &fromlen );

            // validate that the UDP packet came from the only 
            // NFS client we accept (ourselves)

            if ( packlen > 0 )
            {
                if ( ntohl( addr.sin_addr.s_addr ) != 0x7f000001 )
                {
                    itsfs_addlog( "rejecting NFS packet from IP address %08x",
                            ntohl( addr.sin_addr.s_addr ) );
                    continue;
                }

                if ( !source_portset )
                {
                    globs.source_port = ntohs( addr.sin_port );
                    source_portset = true;
                }

                if ( ntohs( addr.sin_port ) != globs.source_port )
                {
                    itsfs_addlog( "rejecting NFS packet from port %d",
                            ntohs( addr.sin_port ));
                    continue;
                }

                uchar * replybuf;
                int replysize = 0;

                // dispatch the NFS request and catch an appropriate
                // NFS reply.  then ship the NFS reply to the requestor.

                globs.nfs_rpc_call_count++;
                log_packet( "received", rcvbuf, packlen );
                replybuf = server.dispatch( rcvbuf, packlen, replysize );

                if ( replybuf )
                {
                    log_packet( "sent", replybuf, replysize );
                    if ( sendto( globs.nfs_rpc_udp_fd,
                                 replybuf, replysize, /* flags */ 0,
                                 (struct sockaddr *)&addr,
                                 sizeof( addr )) < 0 )
                    {
                        itsfs_addlog( "sendto failed with error %d", errno );
                    }
                }
            }
            else
            {
                if ( packlen < 0 )
                    itsfs_addlog( "rcvfrom failed with error %d", errno );
            }
        }

        // process tcp connect requests for new slaves

        if (FD_ISSET(globs.slave_rendevous_fd,&rfds))
        {
            int newfd;

            len = sizeof( addr );
            newfd = accept( globs.slave_rendevous_fd,
                            (struct sockaddr *)&addr,
                            (socklen_t*)&len );

            // allocate a new index.  note that we don't
            // have to free it if we don't end up using it,
            // because the index isn't actually marked as used
            // until we call setup method.

            i = globs.new_index();

            if ( i == -1 )
            {
                close( newfd );
                itsfs_addlog( "new conn rejected, cuz no room" );
                continue;
            }

            uchar newfsname[ 64 ];

            if ( read( newfd, newfsname, 64 ) != 64 )
            {
                itsfs_addlog( "bogus newfsname read from client (!=64)" );
                close( newfd );
                continue;
            }

            // validate that the newfsname received from the slave
            // is not a bogus string.

            if ( !check_ascii( newfsname, 64 ))
            {
                newfsname[63] = 0;
                itsfs_addlog( "bogus newfsname '%s' from client", newfsname );
                close( newfd );
                continue;
            }

            // connect the tree. 

            itsfs_addlog( "connecting new tree '%s'", newfsname );

            treenumber = server.reserve_treeid();
            irt = LOGNEW Inode_remote_tree( treenumber, (uchar*)".", newfd );

            // really we should check for -1 return from register_tree,
            // but since we just reserved it above, there's almost no
            // reason it should fail.  just trust it for now.

            server.register_tree( treenumber, irt );
            int itv_id = itv->register_tree( newfsname, irt );
            globs.trees[i].setup( (char*)newfsname, newfd, 
                                  treenumber, irt, itv_id );

        }

        // if we got a close command from the client while processing
        // the nfs request above, process it here.

        if ( globs.close_command )
        {
            char * f = globs.close_command;
            globs.close_command = NULL;

            // search for the file system's name.

            i = globs.findname( f );

            if ( i == -1 )
                itsfs_addlog( "can't kill tree '%s' : not found", f );
            free( f );

            // kill the tree if it was found.

            if ( i != -1 )
            {
                itsfs_addlog( "killing tree '%s' due to 'close' command",
                        globs.trees[i].treename );
                globs.kill_tree( i );
            }
        }
    }

// debug only
//    inode_name_db->dump_btree();

    return 0;
}

static bool
check_ascii( uchar * s, int c )
{
    int i;
    for ( i = 0; i < c; i++ )
    {
        if ( s[i] >= 'a' && s[i] <= 'z' )
            continue;
        if ( s[i] >= 'A' && s[i] <= 'Z' )
            continue;
        if ( s[i] >= '0' && s[i] <= '9' )
            continue;
        if ( s[i] == '-' )
            continue;
        if ( s[i] == '_' )
            continue;
        if ( s[i] == 0 )
            return true;
        return false;
    }
    // no null ?  fuckers.
    return false;
}

static const int numlogs = 9;
static const int maxloglen = 132;
static char status_log[ numlogs ][ maxloglen ];

void
initlog( void )
{
    memset( &status_log, 0, sizeof( status_log ));
}

void
itsfs_addlog( char * format, ... )
{
    int i;
    va_list ap;

    for ( i = 1; i < numlogs; i++ )
        memcpy( status_log[i-1], status_log[i], maxloglen );

    va_start( ap, format );
    vsnprintf( status_log[numlogs-1], maxloglen, format, ap );
    va_end( ap );
}

static uchar *
update_status_info( void )
{
    // we can make this static cuz this will
    // never be called reentrantly
    static char full[ 4096 ];
    char * f;

    struct timeval uptime;
    struct timeval now;

    gettimeofday( &now, NULL );

    uptime.tv_sec  = now.tv_sec  - _globs->starttime.tv_sec;
    uptime.tv_usec = now.tv_usec - _globs->starttime.tv_usec;

    if ( uptime.tv_usec < 0 )
    {
        uptime.tv_usec += 1000000;
        uptime.tv_sec -= 1;
    }

    f = full;

#define UPDATE(f) f += strlen( f )

    sprintf( f,
             "PID: %d\n"
             "NFS RPC file destriptor: %d\n"
             "remino_worker rendevous descriptor: %d\n"
             "NFS client UDP port: %d\n"
             "NFS RPC calls total: %d\n"
             "server uptime: %d.%06d\n\n",
             getpid(),
             _globs->nfs_rpc_udp_fd, _globs->slave_rendevous_fd,
             _globs->source_port,
             _globs->nfs_rpc_call_count, uptime.tv_sec, uptime.tv_usec );
    UPDATE(f);

    sprint_malloc( f );
    UPDATE(f);

    _globs->sprintinfo( f );
    UPDATE(f);

    sprintf( f, "log information:\n" );
    UPDATE(f);

    for ( int i = 0; i < numlogs; i++ )
    {
        if ( status_log[i][0] )
        {
            sprintf( f, "%d:%s\n", i, status_log[i] );
            UPDATE(f);
        }
    }

#undef UPDATE

    return (uchar*)full;
}
