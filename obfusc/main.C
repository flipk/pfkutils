/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

extern "C" {
#include "tokens.h"
#include "myputs.h"
int yylex(void);
};

#include "btree.H"
#include "translate.h"

void produce_header( Btree * bt );

#define DB_FILE  "obfuscated_symbols.db"
extern char *yytext;
extern int yyleng;
#define MY_ORDER    15
#define CACHE_INFO  10,100,1000

#if 0
#define PRINTF(x)   printf( " " x ": " )
#else
#define PRINTF(x)
#endif

int
main( int argc, char ** argv )
{
    FileBlockNumber      * fbn;
    Btree * bt;
    char token[ 5000 ];
    bool created = false;

    srandom( time(NULL) * getpid() );

    {
        struct stat sb;
        if ( stat( DB_FILE, &sb ) < 0 )
            created = true;
    }

    fbn = new FileBlockNumber( DB_FILE, CACHE_INFO );
    if ( !fbn )
    {
        fprintf( stderr, 
                 "could not open db file %s\n", DB_FILE );
        return 1;
    }

    if ( created )
        Btree::new_file( fbn, MY_ORDER );

    bt = new Btree( fbn );

    if ( argc == 2 )
    {
        produce_header(bt);
        delete bt;
        return 0;
    }

    translate_token_init( bt );
    printf( "#include \"transtags.h\"\n");
    while ( 1 )
    {
        int c = yylex();
        memcpy( token, yytext, yyleng );
        token[yyleng] = 0;
        switch ( c )
        {
        case EOF:
        case 0:
            myputs( "\n" );
            translate_token_close( bt );
            delete bt;
            return 0;

        case TOKEN_LITERAL:
            PRINTF("LITERAL");
            myputs( token );
            break;

        case TOKEN_IDENTIFIER:
            PRINTF("IDENT");
            translate_token( bt, token );
            break;

        case TOKEN_WHITESPACE:
//            PRINTF("WHITE");
            myputwhitespace();
            break;

        case TOKEN_PREPROCLINE:
            PRINTF("PREPROC");
            myputpreproc( token );
            break;

        case TOKEN_COMMENT:
            PRINTF("COMMENT");
            break;

        case TOKEN_CHAR:
            PRINTF("CHAR");
            myputs( token );
            break;

        default:
            printf( "DEFAULT(%d)\n", c );
        }
    }
    /*NOTREACHED*/
}
