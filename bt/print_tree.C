#if 0
g++ -I .. -I ../dll2 print_tree.C libbt.a -o print_tree
exit 0
#endif

#include "btree.H"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

void
print_hex( void * _buf, int len )
{
    unsigned char * buf = (unsigned char *) _buf;
    int pos;
    for ( pos = 0; pos < len; pos++ )
    {
        if (( pos % 24 ) == 0 )
            printf( "\n" );
        printf( " %02x", buf[pos] );
    }
    printf( "\n" );
}

class my_pi : public btree_printinfo {
public:
    my_pi( void ) :
        btree_printinfo( NODE_INFO | BTREE_INFO |
                         KEY_REC_PTR | DATA_REC_PTR ) { }
    /*virtual*/ ~my_pi( void ) { /* nothing */ }
    /*virtual*/ char * sprint_element( UINT32 noderec,
                                       UINT32 keyrec, void * key, int keylen,
                                       UINT32 datrec, void * dat, int datlen,
                                       bool * datdirty ) {

        printf( "key (%d): ", keyrec );
        print_hex( key, keylen );
        printf( "data (%d): ", datrec );
        print_hex( dat, datlen );
        printf( "\n" );

        return (char*) "";
    }
    /*virtual*/ void sprint_element_free( char * s ) {
        // nothing
    }
    virtual void print( char * format, ... )
    {
        va_list ap;
        va_start( ap, format );
        vprintf( format, ap );
        va_end( ap );
    }
};

extern "C" int
btpt_main( int argc, char ** argv )
{
    if ( argc != 3  ||  argv[1][0] != '-' )
    {
    usage:
        fprintf( stderr,
                 "usage: print_tree <flag> <Filename>\n"
                 "    flags:  -i  generic info only\n"
                 "            -r  dump recno records\n"
                 "            -b  dump btree records\n" );
        return 1;
    }

    enum { GENERIC, RECNO, BTREE } flag;

    switch ( argv[1][1] )
    {
    case 'i':  flag = GENERIC; break;
    case 'r':  flag = RECNO;   break;
    case 'b':  flag = BTREE;   break;
    default:   goto usage;
    }

    struct stat sb;
    if ( stat( argv[2], &sb ) < 0 )
    {
        fprintf( stderr, "stat '%s': %s\n", argv[2], strerror( errno ));
        return 1;
    }

    FileBlockNumber * fbn = new FileBlockNumber( argv[2], 100 );

    int num_segments, recs_in_use, recs_free;
    int * perseg_used, * perseg_free;
    fbn->file_info( &num_segments, &recs_in_use, &recs_free,
                    &perseg_used, &perseg_free );

    if ( flag == GENERIC )
    {
        int i;
        printf( "recno info:  "
                "recordsize = %d, pagesize = %d, segmentsize = %d\n"
                "num_segments = %d, recs_in_use = %d, recs_free = %d\n"
                "segments: ",
                fbn->get_recordsize(), fbn->get_pagesize(),
                fbn->get_segmentsize(),
                num_segments, recs_in_use, recs_free );
        for ( i = 0; i < num_segments; i++ )
            printf( "%d%% ", 
                    (perseg_used[i] * 100) /
                    (perseg_used[i] + perseg_free[i]) );
        printf( "\n" );
    }
    if ( flag == RECNO )
    {
        UINT32 bn;
        UINT32 maxbn = sb.st_size / 16;
        for ( bn = 0; bn < maxbn; bn++ )
        {
            int bsize;
            ULONG magic;
            UCHAR * bp = fbn->__get_block( bn, &bsize, &magic );
            if ( bp )
            {
                int pos;
                printf( "rec %d sz %d:", bn, bsize );
                for ( pos = 0; pos < bsize; pos++ )
                {
                    if (( pos % 24 ) == 0 )
                        printf( "\n" );
                    printf( " %02x", bp[pos] );
                }
                printf( "\n" );
                fbn->unlock_block( magic, false );
            }
        }

        delete fbn;
        return 0;
    }

    Btree * bt;
    try {
        bt = new Btree( fbn );
    }
    catch ( Btree::constructor_failed ) {
        printf( "failed to find a btree header\n" );
        delete fbn;
        return 0;
    }

    if ( flag == GENERIC )
    {
        Btree::btreeinfo  bti;
        bt->query( &bti );
        printf( "btree info:  depth = %d   order = %d\n"
                "root block = %d   nodes = %d   records = %d\n",
                bti.depth, bti.order, bti.rootblockno, bti.numnodes,
                bti.numrecords );
    }
    if ( flag == BTREE )
    {
        my_pi  pi;
        bt->dumptree( &pi );
    }

    delete bt;

    return 0;
}
