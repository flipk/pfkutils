/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "ipipe_rollover.h"

ipipe_rollover :: ipipe_rollover( int _max_size, char * _filename, int _flags )
{
    current_size = 0;
    max_size = _max_size;
    filename = strdup(_filename);
    ret = (char*)malloc( strlen(filename) + 8 );
    counter = 1;
    flags = _flags;
}

ipipe_rollover :: ~ipipe_rollover( void )
{
    free(filename);
    free(ret);
}

void
ipipe_rollover :: check_rollover( int fd, int added )
{
    int newfd;
    current_size += added;
    if (current_size < max_size )
        return;
    current_size = 0;
    char * fn = get_next_filename();
    newfd = open( fn, flags, 0644 );
    if (newfd < 0)
    {
        fprintf(stderr, "open rollover file '%s': %s\n",
                fn, strerror(errno));
        return;
    }
    close(fd);
    dup2(newfd, fd);
    close(newfd);
}

char *
ipipe_rollover :: get_next_filename(void)
{
    sprintf(ret, "%s.%05d", filename, counter++);
    return ret;
}
