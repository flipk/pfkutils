
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "wordentry.h"
#include "parse_actions.h"
#include "machine.h"

extern char * strerror( int );
extern int yyparse( void );

extern char * baseclassH;
extern char * baseclassC;

/*
 * usage:
 *   stategen <file.st> -h headerfile -c implfile [-s skelfile]
 */

static int parse_args( int argc, char ** argv, struct args * );

int
pkstgen_main( int argc, char ** argv )
{
    struct args args;
    int fd;

    if ( parse_args( argc, argv, &args ) < 0 )
    {
        fprintf( stderr, "usage:\n"
                 "stategen file.st -b -h file.H -c file.C [-s file2.C]\n"
                 "    file.st  : input file\n"
                 "    -b       : emit base class\n"
                 "    file.H   : header file\n"
                 "    file.C   : implementation\n"
                 "    file2.C  : skeleton implementation\n" );
        return( 1 );
    }

    close( 0 );
    close( 1 );

    if ( open( args.inputfile, O_RDONLY ) < 0 )
    {
        fprintf( stderr, "unable to open input file: %s\n",
                 strerror( errno ));
        exit( 1 );
    }

    init_machine( &args );
    yyparse();

    fprintf( stderr,
             "*** used %d bytes of memory in %d parse entries\n",
             machine.bytes_allocated, machine.entries );

    if ( args.headerfile )
    {
        fprintf( stderr, "*** producing header file %s\n", args.headerfile );
        unlink( args.headerfile );
        open( args.headerfile, O_CREAT | O_WRONLY, 0644 );
        dump_machine( DUMP_HEADER );
        fflush( stdout );
        close( 1 );
    }

    if ( args.implfile )
    {
        fprintf( stderr, "*** producing implementation file %s\n",
                 args.implfile );
        unlink( args.implfile );
        open( args.implfile, O_CREAT | O_WRONLY, 0644 );
        dump_machine( DUMP_CODE );
        fflush( stdout );
        close( 1 );
    }

    if ( args.skelfile )
    {
        fprintf( stderr, "*** producing skeleton file %s\n", args.skelfile );
        unlink( args.skelfile );
        open( args.skelfile, O_CREAT | O_WRONLY, 0644 );
        dump_machine( DUMP_SKELETON );
        fflush( stdout );
        close( 1 );
    }

    destroy_machine();

    if ( args.emit_base_class )
    {
        fprintf( stderr, "*** producing state machine base header\n" );
        unlink( "pk_state_machine_base.H" );
        fd = open( "pk_state_machine_base.H", O_CREAT | O_WRONLY, 0644 );
        write( fd, baseclassH, strlen( baseclassH ));
        close( fd );

        fprintf( stderr, "*** producing state machine base impl\n" );
        unlink( "pk_state_machine_base.C" );
        fd = open( "pk_state_machine_base.C", O_CREAT | O_WRONLY, 0644 );
        write( fd, baseclassC, strlen( baseclassC ));
        close( fd );
    }

    return 0;
}

static int
parse_args( int argc, char ** argv, struct args *a )
{
    int opt;

    a->inputfile = a->headerfile = a->implfile = a->skelfile = NULL;
    if ( argc < 4 )
        return -1;

    argc--; argv++;
    a->inputfile = argv[0];
    a->emit_base_class = 0;

    while (( opt = getopt( argc, argv, "bh:c:s:" )) != -1 )
    {
        switch ( opt )
        {
        case 'b':
            a->emit_base_class = 1;
            break;
        case 'h':
            a->headerfile = optarg;
            break;
        case 'c':
            a->implfile = optarg;
            break;
        case 's':
            a->skelfile = optarg;
            break;
        default:
            return -1;
        }
    }

    if ( !a->headerfile )
    {
        return -1;
    }

    return 0;
}
