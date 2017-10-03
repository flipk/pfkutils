
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
#include <errno.h>

#include <Btree.H>
#include <bst.H>

#include "FileList.H"
#include "db.H"
#include "protos.H"
#include "macros.H"

Btree *
open_treesync_db(char *dir)
{
    Btree * ret;
    char dbpath[512];

    snprintf(dbpath, 511, "%s/" TREESYNC_DB, dir);
    dbpath[511]=0;

    ret = Btree::openFile(dbpath, DB_CACHE_SIZE);

    if (!ret)
    {
        ret = Btree::createFile(dbpath, DB_CACHE_SIZE, 0644, DB_ORDER);
        if (!ret)
        {
            fprintf(stderr, "database creation failed: %s\n",
                    strerror(errno));
            return NULL;
        }
        TreeSyncDbInfo  dbi(ret);
        dbi.key.info_key.set((char*)INFO_KEY);
        dbi.data.num_files.v = 0;
        dbi.data.tool_version.v = TOOL_VERSION;
        dbi.put(true);
    }

    TreeSyncDbInfo dbi(ret);
    dbi.key.info_key.set((char*)INFO_KEY);
    if (!dbi.get())
    {
        fprintf(stderr,"DB INFO key not found in database!!\n");
        delete ret;
        return NULL;
    }
    if (dbi.data.tool_version.v != TOOL_VERSION)
    {
        fprintf(stderr, "Tool version mismatch error!!\n");
        delete ret;
        return NULL;
    }

    return ret;
}
