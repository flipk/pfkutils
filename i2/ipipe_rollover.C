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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "ipipe_rollover.H"

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
