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
pfkbak_list_backups  ( Btree * bt )
{
    PfkBackupDbInfo   info(bt);
    int i, j;

    if (!pfkbak_get_info( &info ))
        return;

    printf("tool version: %d\n"
           "number of backups: %d\n",
           info.data.tool_version.v,
           info.data.backups.num_items);

    for (i=0; i < info.data.backups.num_items; i++)
    {
        UINT32   backup_number = info.data.backups.array[i]->v;

        PfkBackupInfo   binf(bt);

        binf.key.backup_number.v = backup_number;

        if (!binf.get())
        {
            fprintf(stderr, "bogus backup number %d\n", backup_number);
            return;
        }

        printf("backup number %d:\n"
               "   root dir: %s\n"
               "   name: %s\n"
               "   comment: %s\n"
               "   number of files: %d\n"
               "   next generation number: %d\n"
               "   generations:\n",
               backup_number,
               binf.data.root_dir.string,
               binf.data.name.string,
               binf.data.comment.string,
               binf.data.file_count.v,
               binf.data.next_generation_number.v);

        for (j=0; j < binf.data.generations.num_items; j++)
        {
            PfkBakGenInfo * gen = binf.data.generations.array[j];

            printf("      %d: date/time: %s\n",
                   gen->generation_number.v,
                   gen->date_time.string);
        }
    }
}
