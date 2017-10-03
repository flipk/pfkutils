
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

#include <Btree.H>
#include <bst.H>
#include <time.h>

#include "FileList.H"
#include "db.H"
#include "protos.H"

static bool
compaction_status_function(FileBlockStats *stats, void *arg)
{
    static time_t last = 0;
    if (stats->free_regions < 10)
        return false;
    time_t now = time(NULL);
    if (now != last)
    {
        printf("free aus: %d\n", stats->free_aus);
        last = now;
    }
    return true;
}

int treesync_verbose = 0;

#define DELETE_LIST(list)                       \
    while ((fe = list->dequeue_head()) != NULL) \
        delete fe;                              \
    delete list

extern "C" int
treesync_main( int argc, char ** argv )
{
    TSFileEntryList * fel1, * fel2;
    Btree * db1, * db2;
    char * dir1, * dir2;
    TSFileEntry * fe;

    if (argc > 1)
    {
        if (strcmp(argv[1], "-v") == 0)
        {
            treesync_verbose = 1;
            argc--;
            argv++;
        }
    }

    if (argc == 2)
    {
        dir1 = argv[1];
        db1 = open_treesync_db(dir1);
        if (!db1)
            return 1;
        fel1 = treesync_generate_file_list(dir1);
        treesync_update_db(dir1, db1, fel1);
        DELETE_LIST(fel1);
        db1->get_fbi()->compact(compaction_status_function, NULL);
        delete db1;
        return 0;
    }

    if (argc != 3)
    {
        fprintf(stderr, "usage: pfktreesync [-v] dir1 [dir2]\n");
        return 1;
    }

    dir1 = argv[1];
    dir2 = argv[2];

    db1 = open_treesync_db(dir1);
    if (!db1)
        return 1;
    db2 = open_treesync_db(dir2);
    if (!db2)
    {
        delete db1;
        return 1;
    }

    fel1 = treesync_generate_file_list(dir1);
    fel2 = treesync_generate_file_list(dir2);

    if (!fel1 || !fel2)
    {
        delete db1;
        delete db2;
        return 1;
    }

    treesync_update_db( dir1, db1, fel1 );
    treesync_update_db( dir2, db2, fel2 );

    treesync_analyze( dir1, fel1, dir2, fel2 );

    DELETE_LIST(fel1);
    DELETE_LIST(fel2);

    fel1 = treesync_generate_file_list(dir1);
    fel2 = treesync_generate_file_list(dir2);

    treesync_update_db( dir1, db1, fel1 );
    treesync_update_db( dir2, db2, fel2 );

    DELETE_LIST(fel1);
    DELETE_LIST(fel2);

    db1->get_fbi()->compact(compaction_status_function, NULL);
    db2->get_fbi()->compact(compaction_status_function, NULL);

    delete db1;
    delete db2;

    return 0;
}
