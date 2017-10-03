
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
