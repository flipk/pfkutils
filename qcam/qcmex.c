
/*
    This file is part of the "pkutils" tools written by Phil Knaack
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "headers.h"

int
qcmex_main( int argc, char ** argv )
{
    char * dir, * movfile, * decomp, * decomp2;
    int done, frame, l, fd1, fds[2], children;
    struct qcam_softc * qs;
    char  framefile[250];
    FILE * outf, * inf;
    pid_t pid;

    if ( argc != 3 )
    {
        fprintf( stderr, "qcmex file.pgm[.bz2|.gz] directory\n" );
        return 1;
    }
    dir = argv[2];
    if ( mkdir( dir, 0755 ) < 0 )
    {
        fprintf( stderr, "unable to create target directory '%s'\n", argv[2] );
        return 1;
    }

    movfile = argv[1];
    l = strlen( movfile );
    decomp = NULL;
    if ( l > 3 && strcmp( movfile+l-3, ".gz" ) == 0 )
    {
        decomp = "/usr/bin/gzip";
        decomp2 = "gzip";
    }
    if ( l > 4 && strcmp( movfile+l-4, ".bz2" ) == 0 )
    {
        decomp = "/usr/bin/bzip2";
        decomp2 = "bzip2";
    }
    if ( decomp )
    {
        pipe( fds );
        fd1 = open( movfile, O_RDONLY );
        if ( fd1 < 0 )
        {
            fprintf( stderr, "unable to open input file '%s'\n", movfile );
            return 1;
        }
        pid = fork();
        if ( pid < 0 )
        {
            fprintf( stderr, "could not fork decompressor!\n" );
            return 1;
        }
        if ( pid > 0 )
        {
            /* parent */
            close( fd1 );
            close( fds[1] );
            inf = fdopen( fds[0], "r" );
        }
        if ( pid == 0 )
        {
            /* in child */
            close( fds[0] );
            if ( fds[1] != 1 )
            {
                dup2( fds[1], 1 );
                close( fds[1] );
            }
            if ( fd1 != 0 )
            {
                dup2( fd1, 0 );
                close( fd1 );
            }
            execl( decomp, decomp2, "-dc", NULL );
            close(0);
            close(1);
            fprintf( stderr, "cannot exec decompressor!\n" );
            exit(1);
        }
    }
    else
    {
        inf = fopen( movfile, "r" );
        if ( !inf )
        {
            fprintf( stderr, "unable to open input file '%s'\n", movfile );
            return 1;
        }
    }

    qs = qcam_new();

    pgm_readhdr( qs, inf );

    done = 0;
    frame = 1;
    children = 0;
    while ( !done )
    {
        pgm_read( qs, inf );
        if ( feof(inf) )
            done++;
        sprintf( framefile, "%s/frame%05d.jpg", dir, frame );
        fprintf( stderr, "  %d \r", frame++ );
        fflush( stderr );
        fd1 = open( framefile, O_WRONLY | O_CREAT, 0644 );
        if ( fd1 < 0 )
        {
            fprintf( stderr, "unable to create frame file '%s'\n", framefile );
            return 1;
        }
        pipe( fds );
        pid = fork();
        if ( pid < 0 )
        {
            fprintf( stderr, "cannot fork!!!\n" );
            return 1;
        }
        if ( pid > 0 )
        {
            /* parent */
            close( fd1 );
            close( fds[0] );
            outf = fdopen( fds[1], "w" );
            children ++;
        }
        if ( pid == 0 )
        {
            close( fds[1] );
            if ( fds[0] != 0 )
            {
                dup2( fds[0], 0 );
                close( fds[0] );
            }
            if ( fd1 != 1 )
            {
                dup2( fd1, 1 );
                close( fd1 );
            }
            execl( "/usr/local/bin/cjpeg", "cjpeg", NULL );
            close(0);
            close(1);
            fprintf( stderr, "cannot exec decompressor!\n" );
            exit(1);
        }
        pgm_writehdr( qs, outf );
        pgm_write( qs, outf );
        fclose( outf );
        if ( children >= 10 )
        {
            wait( NULL );
            children--;
        }
    }

    return 0;
}
