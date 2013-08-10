
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
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
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

extern "C" {
#include "tokens.h"
#include "myputs.h"
};

#include "btree.H"
#include "translate.h"

int * next_token_number = 0;
int define_symbol = 0;

Btree::rec  * next_token_record;
#define NEXT_TOK_REC_KEY  "__phil_k_next_token_record_key"

void
translate_token_init( Btree * bt )
{
    // attempt to fetch counter
    next_token_record = bt->get_rec( (UCHAR*) NEXT_TOK_REC_KEY,
                                     sizeof(NEXT_TOK_REC_KEY)-1 );
    if ( next_token_record == NULL )
    {
        next_token_record = bt->alloc_rec( sizeof(NEXT_TOK_REC_KEY)-1,
                                           sizeof(int) );
        if ( next_token_record == NULL )
        {
            fprintf( stderr, "%s:%d: should not happen!\n",
                     __FILE__, __LINE__ );
            exit(1);
        }
        next_token_number = (int*) next_token_record->data.ptr;
        *next_token_number = 0x684d38c9;
        memcpy( next_token_record->key.ptr,
                NEXT_TOK_REC_KEY,
                sizeof(NEXT_TOK_REC_KEY)-1 );
        next_token_record->key.dirty = true;
        next_token_record->data.dirty = true;
        bt->put_rec( next_token_record );
        next_token_record = bt->get_rec( (UCHAR*) NEXT_TOK_REC_KEY,
                                         sizeof(NEXT_TOK_REC_KEY)-1 );
    }

    next_token_number = (int*) next_token_record->data.ptr;
    next_token_record->data.dirty = true;
}

void
translate_token_close( Btree * bt )
{
    bt->unlock_rec( next_token_record );
}

void
translate_token( Btree * bt, char * tok )
{
    Btree::rec * rec;
    int tok_len = strlen(tok)+1;
    bool created = false;

    rec = bt->get_rec( (UCHAR*) tok, tok_len );
    if ( rec == NULL )
    {
        // alloc

        char temp_dat[20];

        if ( define_symbol )
        {
            define_symbol = 0;
            temp_dat[0] = 1;
        }
        else
            temp_dat[0] = 0;

#if 0
        sprintf( temp_dat+1, "%c%x", 
                 (random() % 26)  + 'a', *next_token_number );
        (*next_token_number)++;
#else
        sprintf( temp_dat+1, "%c%x", 
                 (random() % 26)  + 'a', random() );
#endif
        rec = bt->alloc_rec( tok_len, strlen(temp_dat+1)+2 );
        memcpy( rec->key.ptr, tok, tok_len );
        rec->key.dirty = true;
        memcpy( rec->data.ptr, temp_dat, strlen(temp_dat+1)+2 );
        rec->data.dirty = true;

        created = true;
    }
    else
    {
        // reuse
        if ( define_symbol )
        {
            rec->data.ptr[0] = 1;
            define_symbol=0;
            rec->data.dirty = true;
        }
        created = false;
    }

    myputs( (char*) rec->data.ptr+1 );

    if ( created )
        bt->put_rec( rec );
    else
        bt->unlock_rec( rec );
}
