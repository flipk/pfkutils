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

#include <osreldate.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/ucred.h>
#include <nfs/rpcv2.h>
#include <sys/mount.h>
#include <nfs/nfsproto.h>
#if __FreeBSD_version < 500023
#include <nfs/nfs.h>
#else
#include <nfsclient/nfs.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>

#include "config.h"

#define ulong unsigned long

#ifndef DO_MOUNT
/*
// to generate this data, edit svr.C and
// turn off USE_CRYPT and REALLY_RANDOM; 
// turn on LOG_PACKETS.  then start the server,
// and copy the resulting "packet received" chunk.
// then bring that data here, put in the commas and 0x's.
// then turn off DO_MOUNT in this file and rerun.
*/
static unsigned char mount_packet[] = {
    0x2b, 0x9a, 0x53, 0xcb, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x86, 0xa3, 
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe8, 
    0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0xe8, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0xa6, 0x2f, 0xf4, 0xe6, 0x10, 0xf3, 0x80, 0xd9, 
    0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 
    0x7a, 0x0f, 0x42, 0x71, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x7a, 0x50, 0xda, 0x93 
};
#endif

typedef struct {
    unsigned char fh[32];
} nfs_fh;

pid_t
do_mount( char * rootfh_bytes )
{
    int ret;
    struct nfs_args nfs_args;
    struct sockaddr_in sa;
    nfs_fh initial_fh;
    char full_hostpath[64];
    ulong addr = 0x7f000001;
    int port = SERVER_PORT;
    pid_t pid;

    pid = fork();
    if ( pid < 0 )
    {
        printf( "fork error: %s\n", strerror( errno ));
        return;
    }
    if ( pid > 0 )
    {
        /* allow the parent to continue with the main event loop
           while the child performs the mount command. */
        return pid;
    }

    memcpy( &initial_fh.fh, rootfh_bytes, 32 );

    sprintf( full_hostpath, "pid%d", getppid() );

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl( addr );
    sa.sin_port = htons( port );

    nfs_args.version = 3;
    nfs_args.addr = (struct sockaddr *)&sa;
    nfs_args.addrlen = sizeof( sa );
    nfs_args.sotype = SOCK_DGRAM;
    nfs_args.proto = IPPROTO_UDP;
    nfs_args.fh = initial_fh.fh;
    nfs_args.fhsize = 32;
    nfs_args.flags =
        NFSMNT_SOFT | NFSMNT_TIMEO |
        NFSMNT_RETRANS | NFSMNT_INT;

    nfs_args.wsize = 8192;
    nfs_args.rsize = 8192;
    nfs_args.readdirsize = 8192;
    nfs_args.timeo = 10;
    nfs_args.retrans = 20;
    nfs_args.maxgrouplist = 16;
    nfs_args.readahead = 1;
/*  nfs_args.leaseterm = 30; */
    nfs_args.deadthresh = 9;
    nfs_args.hostname = full_hostpath;
    nfs_args.acregmin = 3;
    nfs_args.acregmax = 60;
    nfs_args.acdirmin = 30;
    nfs_args.acdirmax = 60;

#ifdef DO_MOUNT
    ret = mount( /* type  */ FS_TYPE,
                 /* dir   */ FS_DIR,
                 /* flags */ FS_FLAGS,
                 /* data  */ (void*)&nfs_args );
#else
    printf( "skipping mount!\n" ); ret = 0;
    {
        int fd = socket( AF_INET, SOCK_DGRAM, 0 );
        sendto( fd, mount_packet, sizeof( mount_packet ),
                0, (struct sockaddr *)&sa, sizeof( sa ));
    }
#endif

    if ( ret < 0 )
    {
        perror( "mount" );
        exit( 1 );
    }

    exit( 0 );
}

void
do_unmount( void )
{
    int ret;

    if ( fork() != 0 )
    {
        /* the parent returns, the child performs the unmount */
        return;
    }

    chdir( "/" );
    sleep( 1 );

    ret = unmount( FS_DIR, FS_FLAGS );
    if ( ret < 0 )
    {
        perror( "umount" );
        usleep( 250000 );
    }
    else
    {
        kill( getppid(), SIGUSR1 );
    }

    exit( 0 );
}
