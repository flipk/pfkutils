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
    while ( len-- > 0 )
    {
        printf( "%02x ", *buf++ );
    }
    printf( "\n" );
}

class my_pi : public btree_printinfo {
public:
    my_pi( void ) :
        btree_printinfo( NODE_INFO | BTREE_INFO |
                         KEY_REC_PTR | DATA_REC_PTR ) { }
    /*virtual*/ ~my_pi( void ) { /* nothing */ }
    /*virtual*/ char * sprint_element( int noderec,
                                       int keyrec, void * key, int keylen,
                                       int datrec, void * dat, int datlen,
                                       bool * datdirty ) {

        printf( "key : " );
        print_hex( key, keylen );
        printf( "data: " );
        print_hex( dat, datlen );
        printf( "\n" );

        return (char*) 1;
    }
    /*virtual*/ void sprint_element_free( char * s ) {
        // nothing
    }
    virtual void print( char * format, ... )
        __attribute__ ((format( printf, 2, 3 ))) {
        // nothing
    }
};

int
main( int argc, char ** argv )
{
    if ( argc != 2 )
    {
        fprintf( stderr, "usage: print_tree <Filename>\n" );
        return 1;
    }

    struct stat sb;
    if ( stat( argv[1], &sb ) < 0 )
    {
        fprintf( stderr, "stat '%s': %s\n", argv[1], strerror( errno ));
        return 1;
    }

    FileBlockNumber * fbn = new FileBlockNumber( argv[1], 100, 100, 100 );
    Btree * bt = new Btree( fbn );

    my_pi  pi;

    bt->dumptree( &pi );

    delete bt;

    return 0;
}
