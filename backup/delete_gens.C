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

/** \file delete_gens.C
 * \brief Method to delete generations from a backup.
 * \author Phillip F Knaack
 */

#include "database_elements.H"
#include "params.H"
#include "protos.H"
#include "FileList.H"
#include <regex.h>

#include <stdlib.h>

/** a regular expression used to recognize what form the user has
 * specified the generation argument. It can either be a single generation
 * specified as a single integer, a range of A-B with a dash separating
 * two numbers, or -B implying that every generation number up thru and
 * including B should be deleted.
 */
static const
char * regexp_number = "^([0-9]+)$|^([0-9]+)-([0-9]+)$|^-([0-9]+)$|^(.+)$";

/** used for indexing the regmatch_t produced by regexec when matching
 * the regexp_number pattern. The values 1-5 correspond to the parenthesized
 * subexpressions above.
 */ 
enum {
    MATCH_WHOLE      = 0,   /**< regmatch_t[0] is always the whole pattern */
    MATCH_N          = 1,   /**< for matching the "N" case                 */
    MATCH_M_THRU_N_M = 2,   /**< for matching the "M" in the "M-N" case    */
    MATCH_M_THRU_N_N = 3,   /**< for matching the "N" in the "M-N" case    */
    MATCH_THRU_N     = 4,   /**< for matching the "N" in the "-N" case     */
    MATCH_NONE       = 5,   /**< only invalid syntax will trigger this one */
    MAX_MATCHES      = 6    /**< how big to dimension the regmatch_t array */
};

/** parse command line options for deleting generations and call the
 * function to delete generation appropriately.
 */
void
pfkbak_delete_gens   ( UINT32 baknum, int argc, char ** argv )
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

        pfkbak_delete_gen( baknum, (UINT32)gen_s, (UINT32)gen_e );
    }
}
