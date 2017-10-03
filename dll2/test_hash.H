
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

#include "dll2.H"

enum { THING_LIST, THING_HASH, THING_NUM_LINKS };

struct thing {
    LListLinks <thing> links[THING_NUM_LINKS];
    int a;
};

struct thing_hash_1 {
    static int hash_key( thing * item ) {
        return item->a;
    }
    static int hash_key( int key ) {
        return key;
    }
    static bool hash_key_compare( thing * item, int key ) {
        return (item->a == key);
    }
};

typedef LListHash  <thing, int, thing_hash_1, THING_HASH>  thing_hash;
typedef LList      <thing,                    THING_LIST>  thing_list;

void add_entries( thing_hash * h, thing_list * l );
void del_entries( thing_hash * h, thing_list * l );
