/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "main.h"

static struct _dont_log_tags {
    int len;
    char * string;
} dont_log_tags[] = {
    { 8, "includes" },
    { 7, "include" },
    { 5, "ifdef" },
    { 6, "ifndef" },

    { 3, "int" },
    { 4, "void" },
    { 4, "long" },
    { 5, "short" },
    { 4, "char" },

    { 8, "__inline" },
    { 3, "__P" },

    { 4, "enum" },
    { 5, "union" },
    { 6, "struct" },
    { 6, "extern" },
    { 6, "static" },
    { 5, "const" },
    { 8, "volatile" },
    { 7, "typedef" },

    { 2, "if" },
    { 4, "else" },
    { 3, "for" },
    { 6, "return" },
    { 4, "case" },
    { 6, "switch" },
    { 6, "sizeof" },

    { 3, "the" },
    { 1, "a" },
    { 3, "and" },
    { 0, 0 }
};
    
int
maketags_do_logit( char * tagname, int taglen )
{
    struct _dont_log_tags * dltp;

    if ( isdigit( tagname[0] ))
        return 0;

    for ( dltp = dont_log_tags; dltp->len != 0; dltp++ )
        if ( taglen == dltp->len )
            if ( strncmp( dltp->string, tagname, taglen ) == 0 )
                return 0;

    return 1;
}
