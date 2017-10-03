
/*
    This file is part of the "pkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
