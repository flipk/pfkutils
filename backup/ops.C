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
    BackupInfoList   bil(bt);

    bil.data.backup_ids.alloc(0);
    if (bil.put() == false)
    {
        fprintf(stderr,"error writing backup info list\n");
    }
}

void
pfkbak_list_backups  ( Btree * bt )
{
    BackupInfoList  bil(bt);

    if (bil.get() == false)
    {
        fprintf(stderr, "could not retrieve backup info list\n");
        return;
    }
    printf("file contains %d backups.\n",
           bil.data.backup_ids.num_items);
    for (int i = 0; i < bil.data.backup_ids.num_items; i++)
    {
        BackupInfo   bi(bt);
        UINT32 id = bil.data.backup_ids.array[i]->v;
        bi.key.backup_id.v = id;
        if (bi.get() == false)
        {
            fprintf(stderr, "could not retrieve backup id %d\n",  id);
            continue;
        }
        printf("backup id %d name: %s\n"
               "    gen count: %d  next gen: %d\n",
               id,
               bi.data.backup_name.string,
               bi.data.generations.num_items,
               bi.data.next_generation_id.v);
        for (int j = 0; j < bi.data.generations.num_items; j++)
        {
            GenerationInfo * gen;
            gen = bi.data.generations.array[j];
            printf("    generation id: %d (taken: %s)\n"
                   "        num files: %d   num bytes: %lld\n",
                   gen->generation_id.v,
                   gen->date_time.string,
                   gen->num_files.v,
                   gen->num_bytes.v );
        }
    }
}

void
pfkbak_create_backup ( Btree * bt, char * root_dir )
{
    BackupInfoList  bil(bt);

    if (bil.get() == false)
    {
        fprintf(stderr, "could not retrieve backup info list\n");
        return;
    }
    printf("backup currently contains %d backups.\n",
           bil.data.backup_ids.num_items);
    int idx = bil.data.backup_ids.num_items;
    bil.data.backup_ids.alloc(idx+1);
    UINT32 new_id = 0;
    int i;
    do {
        new_id = random();
        for (i=0; i < idx; i++)
            if (new_id == bil.data.backup_ids.array[i]->v)
            {
                new_id = 0;
                break;
            }
    } while (new_id == 0);
    printf("allocating new backup id %d for backup name %s\n",
           new_id, pfkbak_name);
    bil.data.backup_ids.array[idx]->v = new_id;
    if (bil.put(true) == false)
    {
        fprintf(stderr, "unable to put new backup info list\n");
        return;
    }

    BackupInfo   bi(bt);
    bi.key.backup_id.v = new_id;
    bi.data.backup_name.strdup(pfkbak_name);
    bi.data.next_generation_id.v = 2;
    bi.data.generations.alloc(1);
    GenerationInfo * gen = bi.data.generations.array[0];
    gen->generation_id.v = 1;
    gen->num_files.v = 0;
    gen->num_bytes.v = 0;
    gen->date_time.strdup((char*) "** TODAY **");
    gen->root_dir_id.v = 0;
    if (bi.put() == false)
    {
        fprintf(stderr, "unable to put new backup info\n");
        return;
    }
    printf("starting in root dir %s\n", root_dir);
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

void
pfkbak_extract       ( Btree * bt, char * gen, int argc, char ** argv )
{
}

void
pfkbak_extract_list  ( Btree * bt, char * gen, char * list_file )
{
}
