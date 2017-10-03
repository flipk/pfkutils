
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

#include "FileBlock_iface.H"
#include "FileBlockLocal.C"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define MAX_BYTES (16*1024*1024)

int
main(int argc, char ** argv)
{
    int fd, options;

    if (argc != 2)
    {
        fprintf(stderr, "usage: t3 <file>\n");
        return 1;
    }

    options = O_RDWR;
#ifdef O_LARGEFILE
    options |= O_LARGEFILE;
#endif
    fd = open(argv[1], options);
    if ( fd < 0 )
    {
        fprintf(stderr, "open: %s\n", strerror(errno));
        return 1;
    }

    PageIO * pageio = new PageIOFileDescriptor(fd);
    BlockCache * bc = new BlockCache( pageio, MAX_BYTES );
    FileBlockLocal * fbi = new FileBlockLocal(bc);

    fbi->dump_extents();

    delete fbi;
    delete bc;
    delete pageio;

    return 0;
}
