/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include <stdio.h>
#include <stdlib.h>

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

    f = fopen( file, "re" );
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
