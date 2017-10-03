
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "dll2.H"
#include "btree.H"
#include "treesync.h"
#include "regen.H"

FileList        file_list;

#define CLEAR(f) memset( &f, 0, sizeof(f) )

class i_file_entry_bad { };
struct i_file_entry {
    LListLinks  <i_file_entry>   links[1];
    struct stat sb;
    char filename[0]; // must be last

    void initsb( void ) {
        if ( lstat( filename, &sb ) < 0 )
        {
            fprintf( stderr, "cannot stat '%s': %s\n",
                     filename, strerror( errno ));
            throw i_file_entry_bad();
        }
#if PK_CYGWIN
        CLEAR( sb.st_atim );
#else
        CLEAR( sb.st_atime );
        CLEAR( sb.st_lspare );
#endif
    }

    i_file_entry( char * newname, bool validate ) {
        strcpy( filename, newname );
        if ( validate )
            initsb();
        else
            memset( &sb, 0, sizeof(sb) );
    }

    i_file_entry( char * newname1, char * newname2 ) {
        sprintf( filename, "%s/%s", newname1, newname2 );
        initsb();
    }

    static void * operator new( size_t __len, char * str ) {
        int len;
        len = sizeof(i_file_entry);
        len += strlen( str ) + 1;
        void * ret = (void*) malloc( len );
        return ret;
    }
    static void * operator new( size_t __len,
                                char * str1, char * str2 ) {
        int len;
        len = sizeof(i_file_entry);
        len += strlen( str1 ) + strlen( str2 ) + 3;
        void * ret = (void*) malloc( len );
        return ret;
    }
    static void operator delete( void * ptr ) {
        free( ptr );
    }
};

typedef LList <i_file_entry,0>  ListOfFiles;

int signature;

static char *
regen_sprintf( void * arg, int noderec,
               int keyrec, void * key, int keylen,
               int datrec, void * dat, int datlen )
{
    ListOfFiles    * file_entries = (ListOfFiles *) arg;
    db_file_entry  * dfe = (db_file_entry *) dat;

    if ( dfe->random_signature != signature )
    {
        i_file_entry * entry;

        char  fname[ keylen + 1 ];
        memcpy( fname, key, keylen );
        fname[keylen] = 0;

        entry = new(fname) i_file_entry(fname,false);
        file_entries->add( entry );
    }

    return ".";
}

static void
regen_sprintf_free( void * arg, char * s )
{
//    ListOfFiles * file_entries = (ListOfFiles *) arg;
}

static void
regen_printfunc( void * arg, char * format, ... )
{
//    ListOfFiles * file_entries = (ListOfFiles *) arg;
}

void
regenerate_database( void )
{
    Btree                * dbfile;
    FileBlockNumber      * fbn;
    ListOfFiles            file_entries;
    bool                   created = false;

    srandom( time(NULL) * getpid() );
    signature = random();

    {
        struct stat sb;
        if ( stat( TREESYNC_DB_FILE, &sb ) < 0 )
            created = true;
    }

    fbn = new FileBlockNumber( TREESYNC_DB_FILE,
                               TREESYNC_CACHE_INFO );
    if ( !fbn )
    {
        fprintf( stderr, 
                 "could not open db file %s\n", TREESYNC_DB_FILE );
        return;
    }

    if ( created )
        Btree::new_file( fbn, TREESYNC_ORDER );

    dbfile = new Btree( fbn );

    // add initial entry to file list
    file_entries.add( new(".") i_file_entry(".",true) );

    // then begin parsing the list until its empty
    i_file_entry * entry, * ne;
    for ( entry = file_entries.get_head(); entry; entry = ne )
    {
        if ( S_ISDIR( entry->sb.st_mode ))
        {
            DIR * dirp = opendir( entry->filename );

            if ( dirp )
            {
                struct dirent * de;

                while ( de = readdir( dirp ))
                {
                    char * nam = de->d_name;

                    if ( strcmp( nam, "." ) == 0    ||
                         strcmp( nam, ".." ) == 0 )
                        continue;

                    if ( strncmp( nam, TREESYNC_FILE_PREFIX,
                                  sizeof(TREESYNC_FILE_PREFIX) - 1 ) == 0 )
                        continue;

                    try {
                        ne =
                            new(entry->filename,nam)
                            i_file_entry(entry->filename,nam);
                        file_entries.add( ne );
                    }
                    catch ( ... ) {
                    }
                }

                closedir( dirp );
            }
            else
            {
                fprintf( stderr, "error opening dir '%s': %s\n",
                         entry->filename, strerror( errno ));
            }
        }
        else if ( S_ISREG( entry->sb.st_mode ))
        {
            Btree::rec  * rec;
            int name_length = strlen( entry->filename );
            db_file_entry * dfe;

            rec = dbfile->get_rec( (UCHAR*) entry->filename, name_length );
            if ( rec == NULL )
            {
                // new entry

                rec = dbfile->alloc_rec( name_length,
                                         sizeof( db_file_entry ));
                if ( !rec )
                {
                    fprintf( stderr, "alloc_rec failed!!\n" );
                    return;
                }

                memcpy( rec->key.ptr, entry->filename, name_length );
                dfe = (db_file_entry *) rec->data.ptr;
                dfe->random_signature = signature;
                dfe->sb = entry->sb;

                dbfile->put_rec( rec );

                file_list.add( 
                    new(entry->filename)
                    FileEntry( entry->filename,
                               FileEntry::NEW,
                               entry->sb.st_size ));
            }
            else
            {
                // existing entry

                dfe = (db_file_entry *) rec->data.ptr;
                dfe->random_signature = signature;
                if ( memcmp( &dfe->sb,
                             &entry->sb, sizeof(struct stat) ) != 0 )
                {
                    // entry was updated
                    dfe->sb = entry->sb;

                    file_list.add( 
                        new(entry->filename)
                        FileEntry( entry->filename,
                                   FileEntry::CHANGED,
                                   entry->sb.st_size ));
                }

                // signature update causes all entries to be dirty
                rec->data.dirty = true;
                dbfile->unlock_rec( rec );
            }
        }

        ne = file_entries.get_next( entry );
        file_entries.remove( entry );
        delete entry;
    }

    // now search bt file looking for entries with mismatching
    // signature; these files have been deleted.

    Btree::printinfo  printinfo = {
        regen_sprintf,
        regen_sprintf_free,
        regen_printfunc,    
        &file_entries,
        false
    };

    dbfile->dumptree( &printinfo );

    while ( entry = file_entries.dequeue_head() )
    {
        file_list.add( 
            new(entry->filename)
            FileEntry( entry->filename,
                       FileEntry::REMOVED,
                       entry->sb.st_size ));
        dbfile->delete_rec( (UCHAR*) entry->filename,
                            strlen( entry->filename ));
        delete entry;
    }

    delete dbfile;

    printf( "database regeneration complete\n" );
}
