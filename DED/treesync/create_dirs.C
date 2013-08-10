
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

int
treesync_create_dirs(char * path)
{
    // search for every slash. zero it out.
    // attempt a stat. if it exists (and is a dir) skip it.
    // if it does not exist, mkdir it. unzero it out.
    // move to next slash, until there are no more slashes.

    char * p = path;
    struct stat sb;

    if (*p == '/')
        p++;

    while (*p)
    {
        if (*p == '/')
        {
            *p = 0;
            if (stat(path, &sb) < 0)
            {
                if (errno == ENOENT)
                {
                    // we can create it and move on.
                    if (mkdir(path, 0755) < 0)
                    {
                        fprintf(stderr, "mkdir %s: %s\n",
                                path, strerror(errno));
                        *p = '/';
                        return -1;
                    }
                }
                else
                {
                    fprintf(stderr, "error stat dir %s: %s\n",
                            path, strerror(errno));
                    *p = '/';
                    return -1;
                }
            }
            *p = '/';
        }
        p++;
    }

    return 0; 
}
