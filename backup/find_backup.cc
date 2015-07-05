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

/** \file find_backup.cc
 * \brief Define utility function for finding a backup in a file.
 * \author Phillip F Knaack
 */

#include "database_elements.h"
#include "params.h"
#include "protos.h"
#include "FileList.h"

#include <stdlib.h>

/** lookup a backup in the database by name and return ID number.
 *
 * @param bakname the short text name of the database
 * @return the backup ID of the backup, if found; if not found, returns 0,
 *         which is an invalid backup ID. 
 */
UINT32
pfkbak_find_backup( char * bakname )
{
    PfkBackupDbInfo   info(pfkbak_meta);
    int i, j;

    if (!pfkbak_get_info( &info ))
        return 0;

    for (i=0; i < info.data.backups.num_items; i++)
    {
        UINT32 backup_number = info.data.backups.array[i]->v;

        PfkBackupInfo   binf(pfkbak_meta);

        binf.key.backup_number.v = backup_number;

        if (!binf.get())
        {
            fprintf(stderr, "bogus backup number %d\n", backup_number);
            return 0;
        }

        if (strcmp(bakname, binf.data.name.string) == 0)
        {
            return backup_number;
        }
    }

    return 0;
}
