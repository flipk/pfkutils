
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <Btree.H>
#include <bst.H>

#include "FileList.H"
#include "db.H"
#include "protos.H"

void
treesync_copy_file(char *fromroot, char *fromfile,
          char *toroot,   char *tofile    )
{
    char from_full_path[512];
    char   to_full_path[512];

    snprintf(from_full_path, sizeof(from_full_path),
             "%s/%s", fromroot, fromfile);
    snprintf(  to_full_path, sizeof(  to_full_path),
               "%s/%s", toroot,   tofile);

    printf("copy %s\n", from_full_path);

    int fd1, fd2;

    fd1 = open(from_full_path, O_RDONLY);
    if (fd1 < 0)
    {
        fprintf(stderr, "error opening source file '%s'\n",
                from_full_path);
        return;
    }
    (void) unlink(to_full_path);
    if (treesync_create_dirs(to_full_path) < 0)
    {
        close(fd1);
        return;
    }        
    fd2 = open(to_full_path, O_CREAT | O_WRONLY, 0644);
    if (fd2 < 0)
    {
        fprintf(stderr, "error opening destination file '%s': %s\n",
                to_full_path, strerror(errno));
        close(fd1);
        return;
    }

    int cc;
    char buffer[65536];

    while (1)
    {
        cc = read(fd1, buffer, sizeof(buffer));
        if (cc <= 0)
            break;
        write(fd2, buffer, cc);
    }

    close(fd1);
    close(fd2);
}
