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

#include "database_elements.H"
#include "params.H"
#include "protos.H"

#include <FileList.H>

#include <stdlib.h>

void
pfkbak_create_backup ( Btree * bt, char * bakname,
                       char * root_dir, char * comment )
{
    PfkBackupDbInfo   info(bt);
    UINT32  baknum;

    if (!pfkbak_get_info( &info ))
        return;

    PfkBackupInfo   binf(bt);

    do {
        baknum = random();

        binf.key.backup_number.v = baknum;
        if (binf.get())
            baknum = 0;
    } while (baknum == 0);

    printf("new backup number %d allocated\n", baknum);

    binf.data.root_dir.set(root_dir);
    binf.data.name.set(bakname);
    binf.data.comment.set(comment);
    binf.data.next_generation_number.v = 1;
    binf.data.generations.alloc(0);

    if (!binf.put())
    {
        printf("error putting new backup info\n");
    }

    int index = info.data.backups.num_items;
    info.data.backups.alloc(index+1);
    info.data.backups.array[index]->v = baknum;

    if (!info.put(true))
    {
        printf("error putting new info\n");
    }
}
