
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

struct fileinfo {
    char * filename;
};

struct taginfo {
    int tagfileoffset;
    char tagname[0];
};

typedef struct {
    FILE * f;
    int numfiles;
    int numtags;
    struct fileinfo * files;
    struct taginfo ** tags;
}  TAGS_FILE;

void * xmalloc( int size );
void   xfree( void * );
extern int xmalloc_allocated;

TAGS_FILE * viewtags_tags_open( char * file );
void viewtags_tags_close( TAGS_FILE * );

#define MAXLINE 8192
#define MAXARGS 400
extern char viewtags_input_line[ MAXLINE ];
extern char * viewtags_lineargs[MAXARGS];

int viewtags_get_line( FILE * f );

void viewtags_display_tags( TAGS_FILE * tf, char * tagname );

/* return 0 when time to exit */
char * viewtags_input_tag( void );
