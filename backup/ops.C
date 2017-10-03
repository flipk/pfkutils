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

#include <stdlib.h>

void
pfkbak_create_file   ( Btree * bt )
{
    PfkBackupDbInfo   info(bt);

    info.key.info_key.set((char*)INFO_KEY);
    info.data.tool_version.v = TOOL_VERSION;
    info.data.backups.alloc(0);
    if (info.put() == false)
    {
        fprintf(stderr,"error writing backup info list\n");
    }
}

bool
pfkbak_validate_file ( Btree * bt )
{
    PfkBackupDbInfo   info(bt);

    info.key.info_key.set((char*)INFO_KEY);

    if (!info.get())
        return false;

    if (info.data.tool_version.v != TOOL_VERSION)
    {
        fprintf(stderr, "tool version mismatch!!\n");
        return false;
    }

    return true;
}

void
pfkbak_list_backups  ( Btree * bt )
{
}

void
pfkbak_create_backup ( Btree * bt, char * root_dir )
{
}

void
pfkbak_update_backup ( Btree * bt, char * root_dir )
{
}

void
pfkbak_delete_backup ( Btree * bt )
{
}

void
pfkbak_delete_gens   ( Btree * bt, int argc, char ** argv )
{
}

void
pfkbak_list_files    ( Btree * bt, char * gen )
{
}

// recall if argc==0, it means extract everything.
void
pfkbak_extract       ( Btree * bt, char * gen, int argc, char ** argv )
{
}

void
pfkbak_extract_list  ( Btree * bt, char * gen, char * list_file )
{
}
