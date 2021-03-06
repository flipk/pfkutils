/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
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

/** \file list_backups.cc
 * \brief list all the backups found in a backup database.
 * \author Phillip F Knaack
 */

#include "database_elements.h"
#include "params.h"
#include "protos.h"
#include "FileList.h"

#include <stdlib.h>

/** list all backups found in a database. */
void
pfkbak_list_backups  ( void )
{
    PfkBackupDbInfo   info(pfkbak_meta);
    int i, j;

    if (!pfkbak_get_info( &info ))
        return;

    if (pfkbak_verb > VERB_QUIET)
        printf("tool version: %d\n"
               "number of backups: %d\n",
               info.data.tool_version.v,
               info.data.backups.num_items);

    for (i=0; i < info.data.backups.num_items; i++)
    {
        uint32_t   backup_number = info.data.backups.array[i]->v;

        PfkBackupInfo   binf(pfkbak_meta);

        binf.key.backup_number.v = backup_number;

        if (!binf.get())
        {
            fprintf(stderr, "bogus backup number %d\n", backup_number);
            return;
        }

        printf("name: %s\n", binf.data.name.string.c_str());

        if (pfkbak_verb > VERB_QUIET)
        {
            printf("   backup number %d:\n"
                   "   root dir: %s\n"
                   "   comment: %s\n"
                   "   number of files: %d\n"
                   "   next generation number: %d\n",
                   backup_number,
                   binf.data.root_dir.string.c_str(),
                   binf.data.comment.string.c_str(),
                   binf.data.file_count.v,
                   binf.data.next_generation_number.v);
        }


        for (j=0; j < binf.data.generations.num_items; j++)
        {
            PfkBakGenInfo * gen = binf.data.generations.array[j];

            printf("   %d: date/time: %s\n",
                   gen->generation_number.v,
                   gen->date_time.string.c_str());
        }
    }
}
