
#include "btree.H"
#include "scan.H"
#include "scan_internal.H"

#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>


#if DEBUG_GET_PUT
char *
format_rec( void * _ptr, int len )
{
    UCHAR * ptr = (UCHAR*) _ptr;
    char * ret = new char[ len * 2 + 1 ];
    int i;
    for ( i = 0; i < len; i++ )
        sprintf( ret + (i*2), "%02x", ptr[i] );
    ret[len*2] = 0;
    return ret;
}
#endif

// the btkey for the record that contains the current
// touch count.

static const char TOUCH_REC_NAME[] = { 
    0, 0,  // prefix = 0
    0x74, 0x6f, 0x75, 0x63, 0x68, 0x20, 0x72, 0x65, 0x63
};

// the md5 hash of an empty file (optimization)

static const UCHAR zerofile_digest[16] = {
    0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
    0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e
};

// the current touch value

static UINT32 touch;

// the time at which this scan started; a magic number
// used to flag updated entries in the database.

static UINT32 scan_start_time;

// the list that we populate for our callers to use.

Final_File_List * scan_list;

class tsn_scan_printinfo : public btree_printinfo {
    File_List * file_entries;
public:
    tsn_scan_printinfo(File_List * _file_entries) :
        btree_printinfo( KEY_REC_PTR | DATA_REC_PTR ) {
        file_entries = _file_entries;
    }
    /*virtual*/ char * sprint_element(
        int noderec,
        int keyrec, void * key, int keylen,
        int datrec, void * dat, int datlen, bool * datdirty );
    /*virtual*/ void sprint_element_free( char * s ) { /* nothing */ }
    /*virtual*/ void print( char * format, ... ) { /* nothing */ }
};

//virtual
char *
tsn_scan_printinfo :: sprint_element(
    int noderec,
    int keyrec, void * key, int keylen,
    int datrec, void * dat, int datlen, bool * datdirty )
{
    Final_File_Entry * ffe;

    UINT16 prefix = ((UINT16_t*)key)->get();

    if ( prefix == 0 ) // hack: look for touch rec
    {
#if DEBUG_DUMP_DB
        printf( "touch = %d\n", ((UINT32_t *)dat)->get());
#endif
    }
    else if ( prefix == 1 )
    {
        db_file_entry * dfe = (db_file_entry *) dat;

#if DEBUG_DUMP_DB
        char filename[ keylen-2+1 ];
        memcpy( filename, (char*)key + 2, keylen - 2 );
        filename[keylen - 2] = 0;
        printf( "file '%s' len %d mtime %d %d touch %d ns %d\n",
                filename, dfe->length.get(), dfe->mtime.get(),
                dfe->touch.get(), dfe->num_segments.get() & 0x7fffffff  );
#endif

        if ( dfe->touch.get() != touch )
        {
            // files which no longer exist require deletion.
            file_entries->add( 
                file_entry::new_entry( key, keylen ));

            if ( scan_list )
            {
                ffe = new(keylen-2+1) Final_File_Entry;
                memcpy( ffe->filename, (char*)key+2, keylen-2 );
                ffe->filename[keylen-2] = 0;
                ffe->status = FFE_DELETED;
                scan_list->add( ffe );
            }
        }
        else
        {
            // all files which do exist should go on a list
            // for exchange / comparison with remote host.
            if ( scan_list )
            {
                Final_File_Entry * ffe = new(keylen-2+1) Final_File_Entry;
                memcpy( ffe->filename, (char*)key+2, keylen-2 );
                ffe->filename[keylen-2] = 0;
                ffe->length = dfe->length.get();
                ffe->num_segments = dfe->num_segments.get();

                if ( ffe->num_segments & 0x80000000 )
                {
                    ffe->status = FFE_NEW;
                    ffe->num_segments &= 0x7fffffff;
                    dfe->num_segments.set( ffe->num_segments );
                    *datdirty = true;
                }
                else if ( dfe->last_scan.get() == scan_start_time )
                    ffe->status = FFE_CHANGED;
                else
                    ffe->status = FFE_SAME;

                if ( ffe->num_segments > 0 )
                    ffe->digests = new MD5_DIGEST[ ffe->num_segments ];
                ffe->next_digest = 0;

                scan_list->add( ffe );
            }
        }
    }
    else /* by implication, prefix >= 2 */
    {
        MD5_DIGEST * digs = (MD5_DIGEST *) dat;
#if DEBUG_DUMP_DB
        char filename[ keylen-2+1 ];
        memcpy( filename, (char*)key + 2, keylen - 2 );
        filename[keylen - 2] = 0;
        MD5_DIGEST zerodigest;

        memset( &zerodigest, 0, sizeof(zerodigest) );

        for ( int i = 0; i < DIGESTS_PER_KEY; i++ )
        {
            if ( memcmp( digs[i].digest, &zerodigest, 
                         sizeof( zerodigest )))
            {
                printf( "digest '%s' digkey %d dignum %d md5 ",
                        filename, prefix - 2, i );
                for ( int j = 0; j < 16; j++ )
                    printf( "%02x", digs[i].digest[j] );
                printf( "\n" );
            }
        }
#endif
        char fname[ keylen-2+1 ];
        memcpy( fname, (char*)key+2, keylen-2 );
        fname[ keylen-2 ] = 0;

        ffe = scan_list ? scan_list->find( fname ) : NULL;
        if ( ffe )
        {
            int digkey = prefix-2;
            int dignum;
            int digsleft = ffe->num_segments - (digkey*DIGESTS_PER_KEY) + 1;

            MD5_DIGEST * digs = (MD5_DIGEST *) dat;
            if ( digsleft > DIGESTS_PER_KEY )
                digsleft = DIGESTS_PER_KEY;

            for ( dignum = 0; dignum < digsleft; dignum++ )
            {
                if ( digkey == 0 && dignum == 0 )
                    ffe->whole_digest = digs[dignum];
                else
                    ffe->digests[ ffe->next_digest++ ] = digs[dignum];
            }
        }
    }

    // return non-null so bt dumptree does not terminate early.
    return ".";
}

static void
scan_free( void * arg, char * s )
{
    // nothing
}

static void
scan_print( void * arg, char * format, ... )
{
    // nothing
}

static void
calculate_digests( char * name,
                   MD5_DIGEST * whole_digest,
                   MD5_DIGEST * digests,
                   UINT32 num_segments )
{
    FILE * f;
    UINT32 i;
    f = fopen( name, "r" );
    if ( !f )
    {
        printf( "error opening '%s'\n", name );
        return;
    }
    char * seg = new char[ SEGMENT_SIZE ];
    MD5_CTX  whole_ctx;
    MD5Init( &whole_ctx );

    for ( i = 0; i < num_segments; i++ )
    {
        int cc;
        cc = fread( seg, 1, SEGMENT_SIZE, f );
        if ( cc < 0 )
        {
            printf( "error reading from '%s'\n", name );
            break;
        }
        MD5_CTX  ctx;
        MD5Init( &ctx );
        MD5Update( &ctx,       (unsigned char *)seg, cc );
        MD5Update( &whole_ctx, (unsigned char *)seg, cc );
        MD5Final( &digests[i], &ctx );
    }

    fclose( f );
    delete[] seg;
    MD5Final( whole_digest, &whole_ctx );

#if DEBUG_CALC_DIGESTS
    for ( i = 0; i < num_segments; i++ )
    {
        int j;
        printf( "file '%s' segment %d md5 ", name, i );
        for ( j = 0; j < 16; j++ )
            printf( "%02x", digests[i].digest[j] );
        printf( "\n" );
    }
#endif
}

static int count_total_files;
static int count_dirs;
static int count_new;
static int count_updated;
static int count_deleted;
static int count_bytes;

// chdir to root of scanned tree before starting
static void
_do_scan( Btree * tree_db, bool first_pass )
{
    File_List  lst;
    time_t last_count, now;
    struct stat sb;
    Btree::rec  * rec;

    count_total_files = 0;
    count_dirs = 0;
    count_new = 0;
    count_updated = 0;
    count_deleted = 0;
    count_bytes = 0;

    rec = tree_db->get_rec( (UCHAR*)TOUCH_REC_NAME,
                            sizeof( TOUCH_REC_NAME ));
    if ( !rec )
    {
        printf( "error retrieving touch record\n" );
        return;
    }

    UINT32_t * ptouch = (UINT32_t *) rec->data.ptr;

    touch = ptouch->get();
    touch++;
    ptouch->set( touch );
    rec->data.dirty = true;
    tree_db->unlock_rec( rec );

    lst.add( file_entry::new_entry( "." ));

    file_entry * fe;

    time( &last_count );
    scan_start_time = last_count;
    while ( fe = lst.dequeue_head())
    {
        if ( lstat( fe->name, &sb ) < 0 )
        {
            printf( "error stat '%s': %s\n", fe->name, strerror( errno ));
            goto next_file;
        }

        time( &now );
        if ( last_count != now )
        {
            fprintf( stderr, "\r  files: %d  bytes: %d  ",
                     count_total_files, count_bytes );
            fflush( stderr );
            last_count = now;
        }

        if ( S_ISDIR( sb.st_mode ))
        {
            DIR * dirp = opendir( fe->name );

            if ( !dirp )
                goto next_file;

#if DEBUG_PROC_DIRS
            printf( "processing dir '%s'\n", fe->name );
#endif
            count_dirs ++;
            struct dirent * de;

            while ( de = readdir( dirp ))
            {
                char * nam = de->d_name;

                if ( strcmp( nam, "." ) == 0    ||
                     strcmp( nam, ".." ) == 0 )
                    continue;

                if ( strncmp( nam, DBFILEPREFIX,
                              sizeof(DBFILEPREFIX)-1 ) == 0 )
                    continue;

                lst.add( file_entry::new_entry( fe->name, nam ) );
            }

            closedir( dirp );

        }
        else if ( S_ISREG( sb.st_mode ))
        {
            int i;
            // 2 for prefix
            int keylen = strlen( fe->name ) + 2;
            // NB: 1 extra for NUL, but NUL is not included to db file
            UCHAR btkey[ keylen+1 ];
            UINT16_t *prefix = (UINT16_t*) btkey;
            bool newent = false;

            count_total_files ++;

#if DEBUG_PROC_FILES
            printf( "processing file '%s'\n", fe->name );
#endif

            // add 01 prefix for key
            prefix->set( 1 );
            // NB: this writes 1 byte beyond 'keylen' but 
            //     that is OK because we allocated an extra byte.
            sprintf( (char*)btkey+2, "%s", fe->name );
            rec = tree_db->get_rec( btkey, keylen );
#if DEBUG_GET_PUT
            {
                char * d1 = format_rec( btkey, keylen );
                printf( "1 fetch rec key: %s\n", d1 );
                delete[] d1; if ( rec ) {
                    char * d2 = format_rec( rec->data.ptr, rec->data.len );
                    printf( "            dat: %s\n", d2 );
                    delete[] d2;
                } else
                    printf( "            dat: NOT FOUND\n" );
            }
#endif
            if ( !rec )
            {
                rec = tree_db->alloc_rec( keylen, sizeof( db_file_entry ));
                if ( !rec )
                {
                    printf( "error in tree_db->alloc_rec\n" );
                    exit( 1 );
                }
#if DEBUG_NEW_FILES
                printf( "new file '%s'\n", fe->name );
#endif
                count_new ++;
                memcpy( rec->key.ptr, btkey, keylen );
                rec->key.dirty = true;
                memset( rec->data.ptr, 0, sizeof( db_file_entry ));
                newent = true;
            }

            db_file_entry * dfe = (db_file_entry *) rec->data.ptr;

            if ( dfe->length.get() != sb.st_size    ||
                 dfe->mtime .get() != sb.st_mtime   )
            {
                UINT32 num_segments;
                int digkey = 0; // which bt key for digests?
                int dignum = 0; // which digest within this bt key?

                num_segments = CALC_NUM_SEGMENTS( sb.st_size );
                if ( num_segments > MAX_SEGMENTS )
                {
                    fprintf( stderr,
                             "WARNING: ignoring file '%s' because "
                             "it is too large!\n", fe->name );
                    // actually we're not ignoring for now :$
                }

                MD5_DIGEST digests[ num_segments+1 ];

                if ( sb.st_size == 0 )
                {
                    memcpy( digests[0].digest,
                            &zerofile_digest,
                            sizeof( digests[0].digest ));
                }
                else
                {
                    calculate_digests( fe->name, &digests[0],
                                       digests+1, num_segments );
                }

                // write digests
                Btree::rec  * digrec;
                MD5_DIGEST * rec_digests;
                bool rec_allocd = false;

                for ( i = 0; i < (num_segments+1); i++ )
                {
                    if ( dignum == 0 )
                    {
                        prefix->set( digkey+2 );
                        digrec = tree_db->get_rec( btkey, keylen );
#if DEBUG_GET_PUT
                        {
                            char * d1 = format_rec( btkey, keylen );
                            printf( "1 fetch rec key: %s\n", d1 );
                            delete[] d1; if ( digrec ) {
                                char * d2 = format_rec( digrec->data.ptr,
                                                        digrec->data.len );
                                printf( "            dat: %s\n", d2 );
                                delete[] d2;
                            } else
                                printf( "            dat: NOT FOUND\n" );
                        }
#endif
                        if ( !digrec )
                        {
                            rec_allocd = true;
                            digrec = tree_db->alloc_rec( 
                                keylen,
                                sizeof( MD5_DIGEST ) * DIGESTS_PER_KEY );

                            if ( !digrec )
                            {
                                printf( "error 0x59b84e2b\n" );
                                exit( 1 );
                            }
                            memcpy( digrec->key.ptr, btkey, keylen );
                            digrec->key.dirty = true;
                            // NB: data marked dirty only if
                            //     brand new record or if 
                            //     memcmp below indicates digest changed.
                            digrec->data.dirty = true;
                        }
                        else
                        {
                            rec_allocd = false;
                        }
                        rec_digests = (MD5_DIGEST*) digrec->data.ptr;
                    }
                    if ( memcmp( rec_digests[dignum].digest,
                                 digests[ i ].digest, 
                                 sizeof( MD5_DIGEST )))
                    {
                        rec_digests[dignum] = digests[ i ];
                        digrec->data.dirty = true;
#if DEBUG_CHANGED_DIGEST
                        printf( "digest seg %d changed on '%s'\n",
                                i, fe->name );
#endif
                    }
                    if ( ++dignum == DIGESTS_PER_KEY )
                    {
                        if ( rec_allocd )
                        {
#if DEBUG_GET_PUT
                            char * d1, * d2;
                            d1 = format_rec( digrec->key.ptr,
                                             digrec->key.len );
                            d2 = format_rec( digrec->data.ptr,
                                             digrec->data.len );
                            printf( "3 put   rec key: %s\n"
                                    "            dat: %s\n", d1, d2 );
                            delete[] d1; delete[] d2;
#endif
                            tree_db->put_rec( digrec );
                        }
                        else
                        {
                            tree_db->unlock_rec( digrec );
                        }
                        dignum = 0;
                        digkey++;
                    }
                }

                if ( dignum > 0 )
                {
                    memset( rec_digests[dignum].digest,
                            0,
                            sizeof(MD5_DIGEST) * (
                                DIGESTS_PER_KEY - dignum ));
                    if ( rec_allocd )
                    {
#if DEBUG_GET_PUT
                        char * d1, * d2;
                        d1 = format_rec( digrec->key.ptr,
                                         digrec->key.len );
                        d2 = format_rec( digrec->data.ptr,
                                         digrec->data.len );
                        printf( "4 put   rec key: %s\n"
                                "            dat: %s\n", d1, d2 );
                        delete[] d1; delete[] d2;
#endif
                        if ( tree_db->put_rec( digrec ) != Btree::PUT_NEW )
                            printf( "error 0x199f1dba\n" );
                    }
                    else
                    {
                        tree_db->unlock_rec( digrec );
                    }
                    digkey++;
                }

                // delete any digest entries higher than num_segments

                while ( digkey < 65530 )
                {
                    prefix->set( digkey+2 );
#if DEBUG_TRIM
                    printf( "TRIM file '%s' : attempting delete on "
                            "digest entry %d\n",
                            fe->name, digkey );
#endif
                    if ( tree_db->delete_rec( btkey, keylen )
                         != Btree::DELETE_OK )
                        break;
                    digkey++;
                }

                dfe->length.set( sb.st_size );
                dfe->mtime.set( sb.st_mtime );
                if ( newent && first_pass )
                    dfe->num_segments.set( num_segments | 0x80000000UL );
                else
                    dfe->num_segments.set( num_segments );
                dfe->last_scan.set( scan_start_time );

//              unnecessary since it is done down below too
//              rec->data.dirty = true;

                if ( !newent )
                {
#if DEBUG_UPDATES
                    printf( "updated file '%s'\n", fe->name );
#endif
                    count_updated ++;
                }
            }

            dfe->touch.set( touch );
            rec->data.dirty = true;

            count_bytes += dfe->length.get();

            if ( newent )
            {
#if DEBUG_GET_PUT
                char * d1, * d2;
                d1 = format_rec( rec->key.ptr,
                                 rec->key.len );
                d2 = format_rec( rec->data.ptr,
                                 rec->data.len );
                printf( "5 put   rec key: %s\n"
                        "            dat: %s\n", d1, d2 );
                delete[] d1; delete[] d2;
#endif
                if ( tree_db->put_rec( rec ) != Btree::PUT_NEW )
                {
                    printf( "error 0xbbd9d7b1\n" );
                }
            }
            else
            {
                tree_db->unlock_rec( rec );
            }
        }

    next_file:
        delete fe;
    }
    fprintf( stderr, "\r  files: %d  bytes: %d  \n",
             count_total_files, count_bytes );

    tsn_scan_printinfo  * pi;

    pi = new tsn_scan_printinfo( &lst );
    tree_db->dumptree( pi );
    delete pi;

    while ( fe = lst.dequeue_head() )
    {
#if DEBUG_DELETES
        printf( "deleted file '%s'\n", fe->name );
#endif
        count_deleted ++;
        // 2 for prefix
        int l = strlen( fe->name ) + 2;
        // 1 to store NUL, but NB: NUL is not part of db file entries
        UCHAR key[ l+1 ];
        UINT16_t * prefix = (UINT16_t *)key;
        UINT16 count;

        sprintf( (char*)key+2, "%s", fe->name );

        for ( count = 1; count < 65530; count++ )
        {
            prefix->set( count );
            if ( tree_db->delete_rec( key, l ) == Btree::DELETE_KEY_NOT_FOUND )
                break;
        }

        delete fe;
    }
}

static bool created_tree;

void
scan_start( void )
{
    Btree * tree_db;
    Btree::rec  * rec;
    struct stat sb;

    created_tree = false;
    if ( stat( DBFILE, &sb ) < 0 )
    {
        created_tree = true;
    }
    else
    {
        if ( system( "cp " DBFILE " " DBFILENEW ) < 0 )
        {
            printf( "error in cp command\n" );
            exit( 1 );
        }
    }

    FileBlockNumber * fbn;
    fbn = new FileBlockNumber( DBFILENEW, DBCACHEPARMS );
    if ( ! fbn )
    {
        printf( "FileBlockNumber construction failure\n" );
        return;
    }

    if ( created_tree )
        Btree::new_file( fbn, 31 );

    tree_db = new Btree( fbn );
    if ( ! tree_db )
    {
        printf( "Btree construction failure\n" );
        return;
    }

    if ( created_tree )
    {
        touch = 1;

        rec = tree_db->alloc_rec( sizeof( TOUCH_REC_NAME ), sizeof( touch ) );
        if ( !rec )
        {
            printf( "error creating touch record\n" );
            return;
        }

        memcpy( rec->key.ptr, TOUCH_REC_NAME, sizeof(TOUCH_REC_NAME));
        rec->key.dirty = true;

        UINT32_t * ptouch = (UINT32_t *) rec->data.ptr;
        ptouch->set( touch );
        rec->data.dirty = true;

        if ( tree_db->put_rec( rec ) != Btree::PUT_NEW )
        {
            printf( "error 0x6614026e\n" );
        }
    }

    scan_list = new Final_File_List;

    _do_scan(tree_db,true);
    delete tree_db;

#if DEBUG_PRINT_FFE_LIST
    Final_File_Entry * ffe;
    for ( ffe = scan_list->get_head();
          ffe;
          ffe = scan_list->get_next(ffe) )
    {
        char hash[ 33 ];
        int i, j;
        if ( ffe->status == FFE_SAME )
            continue;
        for ( i = 0; i < 16; i++ )
            sprintf( hash+(i*2), "%02x", ffe->whole_digest.digest[i] );
        printf( "FILE: %s%s\n", ffe->filename,
                (ffe->status == FFE_SAME) ? "" :
                (ffe->status == FFE_CHANGED) ? " (changed)" :
                (ffe->status == FFE_NEW) ? " (new)" :
                (ffe->status == FFE_NEW) ? " (deleted)" : "(unknown)" );
#if DEBUG_PRINT_FFE_LIST==2
        printf( "size: %d\n", (UINT32) ffe->length );
        printf( "hash: %s\n", hash );
        for ( j = 0; j < ffe->num_segments; j++ )
        {
            for ( i = 0; i < 16; i++ )
                sprintf( hash+(i*2), "%02x", ffe->digests[j].digest[i] );
            printf( "hash%d: %s\n", j, hash );
        }
#endif
    }
#endif

    printf( "  total files: %d\n", count_total_files );
    printf( "  directories: %d\n", count_dirs );
    printf( "  new files  : %d\n", count_new );
    printf( "  upd files  : %d\n", count_updated );
    printf( "  del files  : %d\n", count_deleted );

}


void
scan_finish( bool success )
{
    // clean scan_list, no longer needed.
    Final_File_Entry * ffe;
    while ( ffe = scan_list->dequeue_head())
    {
        delete ffe;
    }
    delete scan_list;
    scan_list = NULL;

    if ( success )
    {
        Btree * tree_db;
        FileBlockNumber * fbn;

        fbn = new FileBlockNumber( DBFILENEW, DBCACHEPARMS );
        if ( ! fbn )
        {
            printf( "FileBlockNumber construction failure\n" );
            return;
        }
        tree_db = new Btree( fbn );
        if ( ! tree_db )
        {
            printf( "Btree construction failure\n" );
            return;
        }

        _do_scan(tree_db,false);
        delete tree_db;

        unlink( DBFILEBAK );
        if ( !created_tree )
            system( "mv " DBFILE " " DBFILEBAK );
        system( "mv " DBFILENEW " " DBFILE );
    }
    else
    {
        unlink( DBFILENEW );
    }
}
