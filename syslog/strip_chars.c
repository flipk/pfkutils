
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
