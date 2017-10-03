/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <string.h>
#include <stdio.h>

#include "main.h"

/**/   char viewtags_input_line[ MAXLINE ];
static char input_line_parsed  [ MAXLINE ];
/**/   char * viewtags_lineargs[ MAXARGS ];

int
viewtags_get_line( FILE * f )
{
    int i;
    char * line;

    if ( fgets( viewtags_input_line, MAXLINE-1, f ) == NULL )
        return -1;

    for ( line = viewtags_input_line; *line; line++ )
        if ( *line == '\n' || *line == '\r' )
        {
            *line = 0;
            break;
        }

    strcpy( input_line_parsed, viewtags_input_line );

    line = input_line_parsed;
    for ( i = 0; i < MAXARGS; i++ )
    {
        viewtags_lineargs[i] = line;

        /* walk forward until space found */
        while ( *line != ' ' && *line != 0 )
            line++;

        if ( *line == 0 )
            return i+1;

        *line++ = 0;

        /* walk forward until nonspace found */
        while ( *line == ' ' )
            line++;

        if ( *line == 0 )
            return i+1;
    }

    return i;
}
