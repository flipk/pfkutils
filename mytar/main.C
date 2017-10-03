
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
};

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

int
main( int argc, char ** argv )
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
}

int
update_mtar( char * dbname, bool create_it )
{
    file_db * db = new file_db( dbname, create_it );
    if ( !db )
        return 1;
    update_dir( db, "." );
    db->delete_old();
    delete db;
    return 0;
}

int
list_mtar( char * dbname, char * pattern )
{
    file_db * db = new file_db( dbname, false );
    if ( !db )
        return 1;
    list_iterator  li(pattern);
    db->iterate( &li );
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
    file_db * db = new file_db( dbname, false );
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

int
delete_mtar( char * dbname )
{
    
}
