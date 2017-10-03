
#include <stdio.h>
#include <errno.h>
#include "main.h"

struct tagfile {
    struct tagfile * next;
    int file_number;
    short numlines;
    short numlines_alloced;
    int lines[0];
};

struct tag {
    struct tag * next;
    struct tagfile * files;
    int count;
    short taglen;
    char tagname[0];
};

static int
dohash( TAGS_OUTPUT * to, char * tagname, int len )
{
    int sum = 0;
    int i;

    for ( i = 0; i < len; i++ )
    {
        int v = tagname[i];

        v <<= (i & 31);
        v += i;
        sum += v;
    }

    if ( sum < 0 )
        sum = -sum;

    return sum % to->taghashsize;
}

TAGS_OUTPUT *
maketags_open_output( char * filename, int taghashsize,
                      int numfiles, char ** filenames )
{
    TAGS_OUTPUT * ret;
    int i;

    ret = xmalloc( sizeof( TAGS_OUTPUT ));
    if ( ret == NULL )
    {
        printf( "malloc failed\n" );
        return NULL;
    }

    ret->current_file_number = 0;
    ret->current_line = 1;
    ret->taghash = xmalloc( sizeof( void * ) * taghashsize );
    ret->taghashsize = taghashsize;

    ret->outfile = fopen( filename, "w" );
    if ( ret->outfile == NULL )
    {
        printf( "opening output file %s: %s\n",
                filename, strerror( errno ));
        xfree( ret );
        return NULL;
    }

    ret->next_compaction = FIRST_COMPACTION_INTERVAL;
    ret->sorted = NULL;

    fprintf( ret->outfile, "FILES %d\n", numfiles );
    for ( i = 0; i < numfiles; i++ )
        fprintf( ret->outfile, "%d: %s\n", i, filenames[i] );
    fprintf( ret->outfile, "ENDFILES\n" );

    return ret;
}

void
maketags_close_output( TAGS_OUTPUT * to )
{
    /* write everything to the file, and close */

    int i, j;
    struct tag * t;
    struct tagfile * tf;

    if ( to->sorted == NULL )
    {
        printf( "cannot close_output! TAGS_OUTPUT must be sorted first\n" );
        return;
    }

    fprintf( to->outfile, "TAGS %d\n", to->numtags );

    for ( i = 0; i < to->numtags; i++ )
    {
        t = to->sorted[i];
        fprintf( to->outfile, "TAG %s\n", t->tagname );
        for ( tf = t->files; tf; tf = tf->next )
        {
            fprintf( to->outfile, "%d: ", tf->file_number );
            for ( j = 0; j < tf->numlines; j++ )
                fprintf( to->outfile, "%d ", tf->lines[ j ] );
            fprintf( to->outfile, "\n" );
        }
    }
}

void
maketags_emit_output( TAGS_OUTPUT * to, char * tagname, int taglen )
{
    struct tag * t;
    struct tagfile * tf, ** ptf;
    int h = dohash( to, tagname, taglen );

    /* search the hash for a matching tag */

    for ( t = to->taghash[h]; t; t = t->next )
        if ( taglen == t->taglen )
            if ( memcmp( t->tagname, tagname, taglen ) == 0 )
                break;

    if ( t == NULL )
    {
        /* we didn't find the tag in the hash, so make a new one */

        t = xmalloc( sizeof( struct tag ) + taglen + 1 );
        t->files = NULL;
        t->taglen = taglen;
        t->count = 0;
        memcpy( t->tagname, tagname, taglen );
        t->tagname[taglen] = 0;

        t->next = to->taghash[h];
        to->taghash[h] = t;
        to->numtags++;
    }

    if ( t->count > MAXTAGCOUNT )
    {
        /* too many of this type of tag.
           purge the tagfile list for this tag. */
        if ( t->files != NULL )
        {
            struct tagfile * ntf;
            for ( tf = t->files; tf; tf = ntf )
            {
                ntf = tf->next;
                xfree( tf );
            }
            t->files = NULL;
            to->numtags--;
        }
        t->count++;
        return;
    }

    /* now we have a struct tag, search it for the current file. */

    ptf = &t->files;
    for ( tf = t->files; tf; tf = tf->next )
    {
        if ( tf->file_number == to->current_file_number )
            break;
        ptf = &tf->next;
    }

    if ( tf == NULL )
    {
        /* we didn't find a file entry already existing for this
           tag, so make one. */

        tf = xmalloc( sizeof( struct tagfile ) +
                      ( INITIAL_NUMLINES * sizeof( tf->lines[0] )));
        tf->file_number = to->current_file_number;
        tf->numlines = 0;
        tf->numlines_alloced = INITIAL_NUMLINES;

        tf->next = t->files;
        t->files = tf;
    }

    /* we now have a tagfile entry. if there's room, insert
       this line number into this tagfile entry. if there is 
       not room, realloc the entry and try again. */

    if ( tf->numlines == tf->numlines_alloced )
    {
        /* realloc the space for this */

        struct tagfile *ntf;

        tf->numlines_alloced += ADDTL_NUMLINES;
        ntf = xmalloc( sizeof( struct tagfile ) +
                       ( tf->numlines_alloced * sizeof( tf->lines[0] )));

        memcpy( ntf, tf, 
                sizeof( struct tagfile ) +
                ( tf->numlines * sizeof( tf->lines[0] )));

        *ptf = ntf;

        xfree( tf );
        tf = ntf;
    }

    /* now we know there is room. insert the new value. */

    if ( tf->numlines > 0 )
        if ( tf->lines[ tf->numlines - 1 ] == to->current_line )
            return;

    tf->lines[ tf->numlines ] = to->current_line;
    tf->numlines++;
    t->count++;
}

void
maketags_output_finish_a_file( TAGS_OUTPUT * to, int done_so_far )
{
    /* walk the hash looking for overallocated tagfile entries,
       and shrink them to save memory. */
    int i;
    struct tag * t;
    struct tagfile * tf, * ntf, ** ptf;

    if ( done_so_far < to->next_compaction )
        return;

    for ( i = 0; i < to->taghashsize; i++ )
    {
        for ( t = to->taghash[i]; t; t = t->next )
        {
            for ( ptf = &t->files, tf = t->files; tf; tf = tf->next )
            {
                if ( tf->numlines < tf->numlines_alloced )
                {
                    int sz = sizeof( struct tagfile ) +
                        ( tf->numlines * sizeof( tf->lines[0] ));
                    ntf = xmalloc( sz );
                    memcpy( ntf, tf, sz );
                    ntf->numlines_alloced = tf->numlines;
                    xfree( tf );
                    *ptf = ntf;
                    tf = ntf;
                }
                ptf = &tf->next;
            }
        }
    }

    if ( done_so_far < FIRST_COMPACTION_THRESHOLD )
        to->next_compaction += FIRST_COMPACTION_INTERVAL;
    else if ( done_so_far < SECOND_COMPACTION_THRESHOLD )
        to->next_compaction += SECOND_COMPACTION_INTERVAL;
    else if ( done_so_far < THIRD_COMPACTION_THRESHOLD )
        to->next_compaction += THIRD_COMPACTION_INTERVAL;
}

static int
tag_compare( void * _a, void * _b )
{
    struct tag * a = *(struct tag **)_a;
    struct tag * b = *(struct tag **)_b;

    return strcmp( a->tagname, b->tagname );
}

void
maketags_sort_output( TAGS_OUTPUT * to )
{
    struct tag ** sorted;
    struct tag * t;
    int i, j;

    sorted = xmalloc( sizeof( void * ) * to->numtags );

    j = 0;
    for ( i = 0; i < to->taghashsize; i++ )
        for ( t = to->taghash[i]; t; t = t->next )
            if ( t->files != NULL )
                sorted[j++] = t;

    qsort( sorted, to->numtags, sizeof( void * ), tag_compare );

    to->sorted = sorted;
}

