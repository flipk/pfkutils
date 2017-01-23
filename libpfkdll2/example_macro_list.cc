
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
    Macro_List   macro_list;
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
