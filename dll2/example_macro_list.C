
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "dll2.H"

enum {
    MACRO_HASH,
    NUM_LISTS
};

#define MAX_LEN 64

struct macro_item {
    LListLinks <macro_item>  links[ NUM_LISTS ];
    char macro[ MAX_LEN ];
    int val;

    macro_item( char * str ) {
        strncpy( macro, str, MAX_LEN-1 );
        macro[ MAX_LEN - 1 ] = 0;
    }
};

class macro_item_hash_1 {
public:
    static int hash_key( macro_item * mi ) {
        return hash_key( mi->macro );
    }
    static int hash_key( char * str ) {
        int ret = 5, mult = 1; char * p = str;
        for ( ; *p; p++ )
        {
            ret += ( (int)(*p) * mult );
            mult++;
        }
        printf( "str '%s' hashes to key %d\n", str, ret );
        return ret;
    }
    static bool hash_key_compare( macro_item * mi, char * str ) {
        return ( strcmp( mi->macro, str ) == 0 );
    }
};

typedef LListHash < macro_item, char *,
                    macro_item_hash_1, MACRO_HASH > Macro_List;

int
main()
{
    Macro_List   macro_list(100);
    macro_item   * mi;

    mi = new macro_item( "one" );
    macro_list.add( mi );

    mi = new macro_item( "two" );
    macro_list.add( mi );

    mi = new macro_item( "three" );
    macro_list.add( mi );

    mi = macro_list.find( "one" );
    macro_list.remove( mi );
    delete mi;

    mi = macro_list.find( "two" );
    macro_list.remove( mi );
    delete mi;

    mi = macro_list.find( "three" );
    macro_list.remove( mi );
    delete mi;

}
