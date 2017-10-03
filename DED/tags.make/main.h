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

typedef struct { 
    FILE * outfile;
    int current_file_number;
    int current_line;
    int next_compaction;
    struct tag ** taghash;
    int taghashsize;
    struct tag ** sorted;
    int numtags;
} TAGS_OUTPUT;

TAGS_OUTPUT * maketags_open_output( char * filename, int taghashsize,
                                    int numfiles, char ** filenames );
void maketags_emit_output( TAGS_OUTPUT * to, char * tagname, int tagsize );
void maketags_output_finish_a_file( TAGS_OUTPUT *, int done_so_far );
void maketags_sort_output( TAGS_OUTPUT * );
void maketags_close_output( TAGS_OUTPUT * );
void maketags_parse_file( FILE * in, TAGS_OUTPUT * out );
int  maketags_do_logit( char * tagname, int taglen );
void * xmalloc( int size );
void   xfree( void * );
extern int xmalloc_allocated;

/* tunable parameters for speed */

#define TAG_HASH_SIZE              16384
#define MAXTAGCOUNT                  300
#define INITIAL_NUMLINES              20
#define ADDTL_NUMLINES                40

#define FIRST_COMPACTION_THRESHOLD   500
#define FIRST_COMPACTION_INTERVAL     10

#define SECOND_COMPACTION_THRESHOLD 1000
#define SECOND_COMPACTION_INTERVAL    50

#define THIRD_COMPACTION_THRESHOLD  2000
#define THIRD_COMPACTION_INTERVAL     75
