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

#include "database_elements.H"
#include "params.H"
#include "protos.H"

#include <FileList.H>
#include <regex.h>

#include <stdlib.h>


static const
char * regexp_number = "^([0-9]+)$|^([0-9]+)-([0-9]+)$|^-([0-9]+)$|^(.+)$";

enum {
    MATCH_WHOLE      = 0,
    MATCH_N          = 1,
    MATCH_M_THRU_N_M = 2,
    MATCH_M_THRU_N_N = 3,
    MATCH_THRU_N     = 4,
    MATCH_NONE       = 5,
    MAX_MATCHES      = 6
};

void
pfkbak_delete_gens   ( Btree * bt, UINT32 baknum, int argc, char ** argv )
{
    int regerr;
    regex_t expr;
    regmatch_t matches[ MAX_MATCHES ];
    int gen_s, gen_e;

    regerr = regcomp( &expr, regexp_number, REG_EXTENDED );

    if (regerr != 0)
    {
        char error_buffer[80];
        regerror( regerr, &expr, error_buffer, 80 );
        printf("regcomp: %s\n", error_buffer);
        return;
    }

    for (; argc > 0; argc--, argv++)
    {
        regerr = regexec( &expr, argv[0], MAX_MATCHES, matches, 0 );
        if (regerr != 0)
        {
            char error_buffer[80];
            regerror( regerr, &expr, error_buffer, 80 );
            printf("regexec: %s\n", error_buffer);
            return;
        }

        if (matches[MATCH_N].rm_so != -1)
            gen_s = gen_e = atoi(argv[0]);
        else if (matches[MATCH_M_THRU_N_M].rm_so != -1)
        {
            gen_s = atoi(argv[0] + matches[MATCH_M_THRU_N_M].rm_so);
            gen_e = atoi(argv[0] + matches[MATCH_M_THRU_N_N].rm_so);
        }
        else if (matches[MATCH_THRU_N].rm_so != -1)
        {
            gen_s = 1;
            gen_e = atoi(argv[0] + matches[MATCH_THRU_N].rm_so);
        }
        else
        {
            printf("bogus gen range '%s'\n", argv[0]);
            continue;
        }
        if (gen_s > gen_e || gen_s <= 0 || gen_e <= 0)
        {
            printf("bogus gen range '%s'\n", argv[0]);
            continue;
        }

        pfkbak_delete_gen( bt, baknum, (UINT32)gen_s, (UINT32)gen_e );
    }
}
