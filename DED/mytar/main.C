
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

#include "file_db.H"
#include "update_file.H"
#include "update_dir.H"
#include "extract_file.H"

#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

class list_iterator : public file_db_iterator {
public:
    char * pattern;
    int pattern_len;
    file_info_list  list;
    virtual void file( file_info * f ) {
        if ( pattern == NULL  ||
             strncmp( pattern, f->fname, pattern_len ) == 0 )
        {
            list.add( f );
        }
        else
        {
            delete f;
        }
    }
    list_iterator( char * _pattern ) {
        pattern = _pattern;
        if ( pattern )
            pattern_len = strlen(pattern);
        else
            pattern_len = 0;
    }
    virtual ~list_iterator( void ) { }
    static int sort_comparator( const void * a, const void * b );
    void sort( void );
};

//static
int
list_iterator :: sort_comparator( const void * _a, const void * _b )
{
    file_info * a = *(file_info **)_a;
    file_info * b = *(file_info **)_b;
    return strcmp( a->fname, b->fname );
}

void
list_iterator :: sort( void )
{
    file_info ** fs, * f;
    int fsindex = 0, i, cnt;

    cnt = list.get_cnt();
    fs = new file_info*[ cnt ];
    while (( f = list.dequeue_head() ) != NULL )
        fs[fsindex++] = f;
    qsort( fs, cnt, sizeof(void*), &sort_comparator );
    for ( i = 0; i < cnt; i++ )
        list.add( fs[i] );
    delete[] fs;
}

void
usage( void )
{
    fprintf( stderr,
             " usage:\n"
             "   mytar c ../file.mtar\n"
             "       - creates file.mtar starting in current directory\n"
             "   mytar u ../file.mtar\n"
             "       - updates file.mtar\n"
             "   mytar t file.mtar <paths>\n"
             "       - lists contents of file.mtar\n"
             "   mytar x ../file.mtar <paths>\n"
             "       - extracts from archive\n" );
    exit( 1 );
}

int update_mtar( char * dbname, bool create_it );
int list_mtar( char * dbname, char * pattern );
int list_multiple_mtar( char * dbname, int argc, char ** argv );
int extract_mtar( char * dbname, char * pattern );
int extract_multiple_mtar( char * dbname, int argc, char ** argv );

extern "C" int
mytar_main( int argc, char ** argv )
{
    if ( argc == 1 || argv[1][1] != 0 )
        usage();
    switch ( argv[1][0] )
    {
    case 'c':
        if ( argc != 3 )
            usage();
        return update_mtar( argv[2], true );

    case 'u':
        if ( argc != 3 )
            usage();
        return update_mtar( argv[2], false );

    case 't':
        if ( argc < 3 )
            usage();
        if ( argc == 3 )
            return list_mtar( argv[2], NULL );
        else
            return list_multiple_mtar( argv[2], argc-3, argv+3 );

    case 'x':
        if ( argc < 3 )
            usage();
        if ( argc == 3 )
            return extract_mtar( argv[2], NULL );
        else
            return extract_multiple_mtar( argv[2], argc-3, argv+3 );
    }
    usage();
    return 1;
}

int
update_mtar( char * dbname, bool create_it )
{
    file_db * db = new file_db( dbname, create_it, !create_it );
    if ( !db )
        return 1;
    update_dir( db, (char*)"." );
    db->delete_old();
    delete db;
    return 0;
}

int
list_mtar( char * dbname, char * pattern )
{
    file_db * db = new file_db( dbname, false, false );
    if ( !db )
        return 1;
    list_iterator  li(pattern);
    db->iterate( &li );
    li.sort();
    while ( file_info * fi = li.list.dequeue_head() )
    {
        printf( "%9lld %s\n", fi->size, fi->fname );
        delete fi;
    }
    return 0;
}

int
list_multiple_mtar( char * dbname, int argc, char ** argv )
{
    int retval;
    while ( argc > 0 )
    {
        retval = list_mtar( dbname, *argv );
        argv++;
        argc--;
        if ( retval != 0 )
            return retval;
    }
    return 0;
}

int
extract_mtar( char * dbname, char * pattern )
{
    file_db * db = new file_db( dbname, false, false );
    if ( !db )
        return 1;
    list_iterator  li(pattern);
    db->iterate( &li );
    while ( file_info * fi = li.list.dequeue_head() )
    {
        extract_file( db, fi->fname );
        delete fi;
    }
    return 0;
}

int
extract_multiple_mtar( char * dbname, int argc, char ** argv )
{
    int retval;
    while ( argc > 0 )
    {
        retval = extract_mtar( dbname, *argv );
        argv++;
        argc--;
        if ( retval != 0 )
            return retval;
    }
    return 0;
}
