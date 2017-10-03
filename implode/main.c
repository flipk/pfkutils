
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "implode.h"

struct lzw_assist {
    FILE * hook_in;
    FILE * hook_out;
};

static int
read_hook( void * arg, unsigned char * buf, int size )
{
    struct lzw_assist * assist = (struct lzw_assist *)arg;
    return fread( buf, 1, size, assist->hook_in );
}

static int
write_hook( void * arg, unsigned char * buf, int size )
{
    struct lzw_assist * assist = (struct lzw_assist *)arg;
    return fwrite( buf, 1, size, assist->hook_out );
}

static int
_lzw_main( int do_compress, int argc, char ** argv )
{
    int retcode;
    char * progname;
    struct lzw_assist assist;

    if ( argc != 3 )
    {
        printf( "usage: [lzwcomp | lzwuncomp] infile outfile\n" );
        printf( "if infile or outfile are '-', stdin or stdout are used\n" );
        return 1;
    }

    progname = ((progname = strrchr( argv[0], '/' )) == NULL) ?
        argv[0] : (progname+1);

    if ( strcmp( argv[1], "-" ) == 0 )
        assist.hook_in = stdin;
    else
        assist.hook_in = fopen( argv[1], "r" );

    if ( assist.hook_in == NULL )
    {
        perror( "open input" );
        return 1;
    }

    if ( strcmp( argv[2], "-" ) == 0 )
        assist.hook_out = stdout;
    else
        assist.hook_out = fopen( argv[2], "w" );

    if ( assist.hook_out == NULL )
    {
        perror( "open output" );
        return 1;
    }

    setvbuf( assist.hook_in,  NULL, _IOFBF, 512 * 1024 );
    setvbuf( assist.hook_out, NULL, _IOFBF, 512 * 1024 );

    retcode = 1;

    if ( do_compress == 1 )
        retcode = lzw_Implode( read_hook, write_hook, &assist,
                               CMP_BINARY, 4096, 9 );
    else if ( strcmp( progname, "lzwuncomp" ) == 0 )
        retcode = lzw_Explode( read_hook, write_hook, &assist );
    else
        fprintf( stderr, "unknown program %s\n", progname );

    fclose( assist.hook_in );
    fclose( assist.hook_out );

    return retcode;
}

int
lzwcomp_main( int argc, char ** argv )
{
    return _lzw_main( 1, argc, argv );
}

int
lzwuncomp_main( int argc, char ** argv )
{
    return _lzw_main( 0, argc, argv );
}
