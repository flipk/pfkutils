
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "dll2.H"
#include "btree.H"
#include "treesync.h"
#include "regen.H"

#define DEB_PR      1
#define DEB_SAME_PR 0

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

    static i_file_entry * new_entry( char * _str, bool val ) {
        return new(_str) i_file_entry(_str,val);
    }
    static i_file_entry * new_entry( char * _str1, char * _str2 ) {
        return new(_str1,_str2) i_file_entry(_str1,_str2);
    }
    static void operator delete( void * ptr ) {
        free( ptr );
    }

private:
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
};

typedef LList <i_file_entry,0>  ListOfFiles;

int signature;

class treesync_regen_printinfo : public btree_printinfo {
    ListOfFiles * file_entries;
public:
    treesync_regen_printinfo( ListOfFiles * _file_entries ) :
        btree_printinfo( KEY_REC_PTR | DATA_REC_PTR ) {
        file_entries = _file_entries;
    }
    /*virtual*/ char * sprint_element( int noderec,
                                       int keyrec, void * key, int keylen,
                                       int datrec, void * dat, int datlen,
                                       bool * datdirty );
    /*virtual*/ void sprint_element_free( char * s ) { /* nothing */ }
    /*virtual*/ void print( char * format, ... ) { /* nothing */ }
};

char *
treesync_regen_printinfo :: sprint_element(
    int noderec,
    int keyrec, void * key, int keylen,
    int datrec, void * dat, int datlen,
    bool *datdirty )
{
    db_file_entry  * dfe = (db_file_entry *) dat;

    if ( dfe->random_signature != signature )
    {
        i_file_entry * entry;

        char  fname[ keylen + 1 ];
        memcpy( fname, key, keylen );
        fname[keylen] = 0;

#if DEB_PR
        printf( "deleted: %s\n", fname );
#endif

        entry = i_file_entry::new_entry(fname,false);
        file_entries->add( entry );
    }

    return "."; // return non-null so dumptree keeps going
}

void
regenerate_database( bool backup )
{
    Btree                * dbfile;
    FileBlockNumber      * fbn;
    ListOfFiles            file_entries;
    bool                   created = false;

    signature = random();

    {
        struct stat sb;
        if ( stat( TREESYNC_DB_FILE, &sb ) < 0 )
        {
            created = true;
        }
        else
        {
            if ( backup )
                // make backup file
                system( "cp "
                        TREESYNC_DB_FILE
                        " "
                        TREESYNC_DB_FILE ".bak" );
        }
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
    file_entries.add( i_file_entry::new_entry( ".", true ));

    // then begin parsing the list until its empty
    i_file_entry * entry, * ne;
    while ( entry = file_entries.dequeue_head() )
//    for ( entry = file_entries.get_head(); entry; entry = ne )
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
                        ne = i_file_entry::new_entry( entry->filename, nam );
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
                    FileEntry::new_entry( 
                        entry->filename,
                        FileEntry::NEW,
                        entry->sb.st_size ));

#if DEB_PR
                printf( "new: %s\n", entry->filename );
#endif
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

#if DEB_PR
                    printf( "changed: %s\n", entry->filename );
#endif

                    file_list.add( 
                        FileEntry::new_entry(
                            entry->filename,
                            FileEntry::CHANGED,
                            entry->sb.st_size ));
                }
#if DEB_SAME_PR
                else
                    printf( "same: %s\n", entry->filename );
#endif

                // signature update causes all entries to be dirty
                rec->data.dirty = true;
                dbfile->unlock_rec( rec );
            }
        }

//        ne = file_entries.get_next( entry );
//        file_entries.remove( entry );
        delete entry;
    }

    // now search bt file looking for entries with mismatching
    // signature; these files have been deleted.

    treesync_regen_printinfo pi( &file_entries );

    dbfile->dumptree( &pi );

    while ( entry = file_entries.dequeue_head() )
    {
        file_list.add( 
            FileEntry::new_entry(
                entry->filename,
                FileEntry::REMOVED,
                entry->sb.st_size ));

        dbfile->delete_rec( (UCHAR*) entry->filename,
                            strlen( entry->filename ));
        delete entry;
    }

    delete dbfile;

    printf( "database regeneration complete\n" );
}

void
clean_file_entry_list(void)
{
    FileEntry * fe;
    while ( fe = file_list.dequeue_head() )
        delete fe;
}
