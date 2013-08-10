
#include <stdio.h>

char *
urlify( char * input )
{
    static char output[600];
    char *i, *o;
    static char hex[] = "0123456789abcdef";

    memset( output, 0, 600 );

    for ( i=input, o=output; *i; i++ )
    {
        if ( isspace( *i ) )
        {
            *o++ = '+';
        }
        else if ( isalpha( *i ) || isdigit( *i ) )
        {
            *o++ = *i;
        }
        else
        {
            /* hexify it */
            *o++ = '%';
            *o++ = hex[ (*i) / 16 ];
            *o++ = hex[ (*i) % 16 ];
        }
    }

    return output;
}
