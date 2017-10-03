/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdlib.h>
#include <string.h>

#include "strip_chars.h"

int
strip_chars(char *buf, int len)
{
    char *inbuf = buf;
    char *obuf = (char*) malloc(len);
    int olen = 0;

    while (len > 0)
    {
        if (*buf != 10 && *buf != 13)
            obuf[olen++] = *buf;
        buf++;
        len--;
    }

    memcpy(inbuf,obuf,olen);
    free(obuf);
    return olen;
}
