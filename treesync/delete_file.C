
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
#include "macros.H"

void
treesync_delete_file(char *root, char *file)
{
    char from_full_path[512];
    char   to_full_path[512];
    char   to_file_name[512];
    char * p, * q;

    for (p = file, q = to_file_name; *p; p++)
    {
        if (*p != '/')
            *q++ = *p;
        else
            *q++ = '.';
    }
    *q++ = 0;

    snprintf(from_full_path, sizeof(from_full_path),
             "%s/%s", root, file);
    snprintf(  to_full_path, sizeof(  to_full_path),
             "%s/" TRASH_DIR "/%s", root, to_file_name);

    printf("delete %s\n", from_full_path);
    (void)unlink(to_full_path);
    if (rename(from_full_path, to_full_path) < 0)
        fprintf(stderr, "rename: %s\n", strerror(errno));
}
