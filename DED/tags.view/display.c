
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

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"

/* return 1 if match */
int
viewtags_string_match( char * pattern, char * string )
{
    if ( strstr( string, pattern ) == NULL )
        return 0;
    return 1;
}

static void display_files( TAGS_FILE * tf, struct taginfo * ti );

static void
bail( void )
{
    printf( "invalid file format while parsing display entry\n" );
    exit( 1 );
}

void
viewtags_display_tags( TAGS_FILE * tf, char * tagname )
{
    struct taginfo ** matching_tags;
    int i, j, count;

    /* first count matches */
    for ( i = count = 0; i < tf->numtags; i++ )
        if ( viewtags_string_match( tagname, tf->tags[i]->tagname ))
            count++;

    matching_tags = xmalloc( sizeof( void * ) * count );

    /* do again, this time collecting pointers */
    for ( i = j = 0; i < tf->numtags; i++ )
        if ( viewtags_string_match( tagname, tf->tags[i]->tagname ))
            matching_tags[j++] = tf->tags[i];

    for ( i = 0; i < count; i++ )
        printf( "%d. %s\n", i+1, matching_tags[i]->tagname );

    if ( count == 0 )
    {
        printf( "no matches found\n" );
        goto out;
    }

    printf( "which tag? " );
    fflush( stdout );
    scanf( "%d", &i );

    i--;
    if ( i > count )
    {
        printf( "invalid entry\n" );
        goto out;
    }

    display_files( tf, matching_tags[i] );

 out:
    xfree( matching_tags );
}

struct tag_file_info {
    struct tag_file_info * next;
    char * filename;
    int lineno;
    char line[0];
};

void
display_files( TAGS_FILE * tf, struct taginfo * ti )
{
    struct tag_file_info * tfi_head, * tfi, ** tfip;
    int i, numargs, count;
    char * short_filename;

    tfi_head = NULL;
    tfip = &tfi_head;

    fseek( tf->f, ti->tagfileoffset, SEEK_SET );
    count = 0;

    while ( 1 )
    {
        int j;
        numargs = viewtags_get_line( tf->f );

        if ( numargs == -1 )
            bail();

        if ( strcmp( viewtags_lineargs[0], "TAG" ) == 0 )
            break;

        for ( i = 1; i < numargs; i++ )
        {
            int len;
            int lineno = atoi( viewtags_lineargs[i] );
            int fileno = atoi( viewtags_lineargs[0] );
            char * fname = tf->files[ fileno ].filename;
            FILE * tmpf;

            tmpf = fopen( fname, "r" );
            if ( tmpf == NULL )
            {
                printf( "error opening %s\n", fname );
                continue;
            }

            for ( j = 1; j <= lineno; j++ )
                if ( fgets( viewtags_input_line, MAXLINE, tmpf ) == NULL )
                {
                    printf( "error reading line %d of %s\n",
                            j, fname );
                    fclose( tmpf );
                    continue;
                }

            fclose( tmpf );

            len = strlen( viewtags_input_line ) + 1;

            if ( viewtags_input_line[len-2] == '\n' )
            {
                viewtags_input_line[len-2] = 0;
                len--;
            }

            tfi = xmalloc( sizeof( struct tag_file_info ) + len );
            tfi->next = NULL;
            tfi->filename = fname;
            tfi->lineno = lineno;
            strcpy( tfi->line, viewtags_input_line );

            *tfip = tfi;
            tfip = &tfi->next;
            count++;
        }
    }

    for ( i = 1, tfi = tfi_head; tfi; i++, tfi = tfi->next )
    {
        printf( "%3d: %32s %4d  :  %s\n",
                i, tfi->filename, tfi->lineno, tfi->line );
    }

    printf( "which file? " );
    fflush( stdout );
    scanf( "%d", &i );

    if ( i > count )
    {
        printf( "invalid entry\n" );
        goto out;
    }

    for ( tfi = tfi_head; i > 1; tfi = tfi->next )
        i--;

    short_filename = (char*) xmalloc( i = strlen( tfi->filename ));
    strcpy( short_filename, tfi->filename );

    while ( i > 0 && short_filename[i] != '/' )
        i--;

    sprintf( viewtags_input_line, 
             "xterm -g 80x35 -title %s -e less +%d %s &",
             short_filename+i+1, tfi->lineno, tfi->filename );
    xfree( short_filename );

    system( viewtags_input_line );

 out:
    for ( tfi = tfi_head; tfi; )
    {
        struct tag_file_info * tfin = tfi->next;
        xfree( tfi );
        tfi = tfin;
    }
}
