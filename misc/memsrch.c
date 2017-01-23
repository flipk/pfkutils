
#include "memsrch.h"

/* search a big buffer for occurrances of bytes
   specified in little buffer */
int
memsrch( uchar * bigbuf, uint biglen,
         uchar * littlebuf, uint littlelen )
{
    int i, j, ret = -1;
    if ( biglen == 0 || littlelen > biglen )
        return -1;
    for ( j = i = 0; i < biglen; i++ )
    {
        while ( littlebuf[j] == bigbuf[i] )
        {
            if ( j == 0 )
                ret = i;
            j++; i++;
            if ( j == littlelen )
                return ret;
        }
        if ( ret >= 0 )
        {
            i = ret; j = 0;
        }
        ret = -1;
    }
    return ret;
}
