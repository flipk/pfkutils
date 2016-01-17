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

/** \file create_backup.cc
 * \brief Implementation of method to create a backup in a backup database.
 * \author Phillip F Knaack
 */

#include "database_elements.h"
#include "params.h"
#include "protos.h"
#include "FileList.h"

#include <stdlib.h>
#include <strings.h>
#include <errno.h>

/**
 * create a backup in a backup database file.
 * This function sets up a new backup, creating a PfkBackupInfo and
 * populating it with an initial state. It does not add any user data
 * to the backup.
 *
 * @param bakname   A short string name for this backup.
 * @param root_dir  The starting directory for this backup.
 * @param comment   A text string to be displayed in the 'comment' field.
 */
void
pfkbak_create_backup ( char * bakname,
                       char * root_dir, char * comment )
{
    PfkBackupDbInfo   info(pfkbak_meta);
    uint32_t  baknum;

    if (root_dir[0] != '/')
    {
        fprintf(stderr, "ERROR: root directory of a backup must be an "
                "absolute path.\n");
        return;
    }

    if (chdir(root_dir) < 0)
    {
        fprintf(stderr, "chdir to '%s': %s\n",
                root_dir, strerror(errno));
        return;
    }

    if (!pfkbak_get_info( &info ))
        return;

    PfkBackupInfo   binf(pfkbak_meta);

    do {
        baknum = random();

        binf.key.backup_number.v = baknum;
        if (binf.get())
            baknum = 0;
    } while (baknum == 0);

    binf.data.root_dir.set(root_dir);
    binf.data.name.set(bakname);
    binf.data.comment.set(comment);
    binf.data.file_count.v = 0;
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

    printf("created backup '%s' using backup number %d\n",
           bakname, baknum);
}
