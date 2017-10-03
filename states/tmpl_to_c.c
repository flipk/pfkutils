/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdio.h>

void
usage()
{
    fprintf( stderr,
             "usage:  tmpl_to_c [template_file var_name]...\n" );
    exit( 1 );
}

void process_template( char * file, char * var );

int
main( int argc, char ** argv )
{
    argc--; argv++;
    if ( argc == 0 || (( argc & 1 ) != 0 ))
        usage();

    do
    {
        fprintf( stderr, "processing template file %s\n", argv[0] );
        process_template( argv[0], argv[1] );
        argc -= 2;
        argv += 2;
    }
    while ( argc > 0 );

    return 0;
}

void
outline( char * s )
{
    while (1)
    {
        if ( *s == 10 || *s == 13 || *s == 0 )
            return;
        if ( *s == '"' )
            putchar( '\\' );
        if ( *s == '\\' )
            putchar( '\\' );
        putchar( *s );
        s++;
    }
}

void
process_template( char * file, char * var )
{
    char line[132];
    FILE * f;

    f = fopen( file, "r" );
    if ( !f )
    {
        fprintf( stderr, "could not open file %s\n", file );
        exit( 1 );
    }

    printf( "const char * %s = \n", var );

    while ( fgets( line, 131, f ))
    {
        if ( line[0] == '%' )  /* handle comments */
            continue;
        putchar( '"' );
        outline( line );
        printf( "\\n\"\n" );
    }

    printf( ";\n" );
}
