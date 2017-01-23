
#ifndef __ID_NAME_DB_H_
#define __ID_NAME_DB_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mytypes.h"
#include "ino_types.h"
#include "../bt/btree.H"

class id_name_db {
    Btree * bt;
    char fname[ 64 ];
    bool purge_active;
    int purging_id;

    static char * btdump_sprint( void * arg, int noderec,
                                 int keyrec, void * key, int keylen,
                                 int datrec, void * dat, int datlen,
                                 bool *datdirty );
    static void   btdump_sprintfree( void * arg, char * s );
    static void   btdump_print( void * arg, char * format, ... );

    static char * btdump_real_sprint( void * arg, int noderec,
                                      int keyrec, void * key, int keylen,
                                      int datrec, void * dat, int datlen,
                                      bool *datdirty );
    static void   btdump_real_sprintfree( void * arg, char * s );
    static void   btdump_real_print( void * arg, char * format, ... );
    void _periodic_purge( bool all, int id );

public:
    id_name_db( void );
    ~id_name_db( void );

    void add( int mount_id, uchar * path, inode_file_type ftype, int id );
    void del( int id );
    void purge_mount( int mount_id );
    void dump_btree( void );
    // return null if not present
    uchar * fetch( int id, inode_file_type &ftype );
    // return -1 if not present
    int fetch( int mount_id, uchar * path, inode_file_type &ftype );
    int alloc_id( void )
        {
            int id;
            inode_file_type dummy;
            while ( 1 )
            {
                uchar * p;
                id = random();
                p = fetch( id, dummy );
                if ( p == NULL )
                    return id;
                delete[] p;
            }
        }

    void flush( void ) { bt->flush(); }
    void periodic_purge( void ) { _periodic_purge( false, purging_id ); }
    bool need_periodic_purge( void ) { return purge_active; }
};

#endif /* __ID_NAME_DB_H_ */
