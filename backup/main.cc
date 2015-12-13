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

/** \file main.cc
 * \brief the "main" entry point for pfkbak tool.
 * \author Phillip F Knaack
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "pfkutils_config.h"

#include "Btree.h"
#include "params.h"
#include "database_elements.h"
#include "protos.h"

#define SEPARATE_DATA_FILE 0
#define CACHE_SIZE (512*1024*1024)
#define BTREE_ORDER  25

/** the btree key for the database info uses this string constant.
 */
const char * PfkBackupDbInfoKey::INFO_KEY = "PfkBakDbInfo";

/** display command line help for this tool, and exit.
 */
static void
usage(void)
{
    fprintf(stderr, "version: %s\n", PACKAGE_STRING);
    fprintf(stderr, "usage: \n"
"create file:    pfkbak -C[Vv] BACKUP-FILE\n"
"\n"
"list backups:   pfkbak -L[Vv] BACKUP-FILE\n"
"create backup:  pfkbak -c[Vv] BACKUP-FILE BACKUP-NAME DIR COMMENT\n"
"delete backup:  (unimplemented) pfkbak -D[Vv] BACKUP-FILE BACKUP-NAME\n"
"update backup:  pfkbak -u[Vv] BACKUP-FILE BACKUP-NAME\n"
"\n"
"delete gens:    pfkbak -d[Vv] BACKUP-FILE BACKUP-NAME GenN A-B -B\n"
"\n"
"list files:     pfkbak -l[Vv] BACKUP-FILE BACKUP-NAME GenN\n"
"\n"
"extract:        pfkbak -e[Vv] BACKUP-FILE BACKUP-NAME GenN   (unimplemented [FILE...])\n"
"extract list:   (unimplemented) pfkbak -E[Vv] BACKUP-FILE BACKUP-NAME GenN LIST-FILE\n"
            "\n"
        );
    exit(1);
}

/** describe what -v or -V command line options were specified. 
 */
verblevel  pfkbak_verb;

Btree              * pfkbak_meta;
FileBlockInterface * pfkbak_data;
char               * pfkbak_file;
char               * pfkbak_data_file;
char               * pfkbak_name;

/** the main function of this tool.
 * it is not called 'main' because it is referenced by the pfkutils
 * tool's main function.
 */
extern "C" int
pfkbak_main(int argc, char ** argv)
{
    backop     pfkbak_op;
    UINT32     baknum = 0;

    pfkbak_op = BAK_NONE;
    pfkbak_verb = VERB_QUIET;

    if (argc < 3 || argv[1][0] != '-')
        usage();

    char * cmd = &argv[1][1];
    if (cmd[1] == 'v')
        pfkbak_verb = VERB_1;
    else if (cmd[1] == 'V')
        pfkbak_verb = VERB_2;

    // in every command format, argv[2] is the backup file.
    // in every command which has argv[3], it is the backup name.
    pfkbak_file = argv[2];
    if (argc > 3)
        pfkbak_name = argv[3];
    else
        pfkbak_name = NULL;

    pfkbak_data_file = (char*) malloc(strlen(pfkbak_file)+5);
    sprintf(pfkbak_data_file, "%s.data", pfkbak_file);

    // assign pfkbak_op
#define OP(c,op) case c: pfkbak_op = BAK_##op; break;
    switch (*cmd)
    {
        OP('C',CREATE_FILE);
        OP('L',LIST_BACKUPS);
        OP('c',CREATE_BACKUP);
        OP('D',DELETE_BACKUP);
        OP('u',UPDATE_BACKUP);
        OP('d',DELETE_GENS);
        OP('l',LIST_FILES);
        OP('e',EXTRACT);
        OP('E',EXTRACT_LIST);
    default:
        usage();
    }
#undef OP

    // verify number of command line arguments
    switch (pfkbak_op)
    {
    case BAK_CREATE_FILE:
    case BAK_LIST_BACKUPS:
        if (argc != 3) usage(); break;

    case BAK_DELETE_BACKUP:
    case BAK_UPDATE_BACKUP:
        if (argc != 4) usage(); break;

    case BAK_LIST_FILES:
        if (argc != 5) usage(); break;

    case BAK_DELETE_GENS:
        if (argc < 5)  usage(); break;

    case BAK_EXTRACT:
        if (argc < 5)  usage(); break;

    case BAK_CREATE_BACKUP:
    case BAK_EXTRACT_LIST:
        if (argc != 6) usage(); break;

    case BAK_NONE:
    default:
        // shouldn't be reached but it satisifies compiler.
        usage();
    }

    Btree * bt = NULL;
    FileBlockInterface * fbi = NULL;

    int bt_cache_size = CACHE_SIZE;

    // create or open btree file
    if (pfkbak_op == BAK_CREATE_FILE)
    {
        bt = Btree::createFile( pfkbak_file, bt_cache_size,
                                0600, BTREE_ORDER );

        if (!bt)
        {
            fprintf(stderr, "unable to create file: %s: %s\n",
                    pfkbak_file, strerror(errno));
            return 1;
        }

#if SEPARATE_DATA_FILE
        fbi = FileBlockInterface::createFile( pfkbak_data_file,
                                              DATA_CACHE_SIZE, 0600 );

        if (!fbi)
        {
            fprintf(stderr, "unable to create file: %s: %s\n",
                    pfkbak_data_file, strerror(errno));
            return 1;
        }
#else
        fbi = bt->get_fbi();
#endif
    }
    else
    {
        bt = Btree::openFile( pfkbak_file, bt_cache_size );

        if (!bt)
        {
            fprintf(stderr, "Unable to open backup file: %s: %s\n",
                    pfkbak_file, strerror(errno));
            return 1;
        }

        PfkBackupDbInfo   info(bt);
        if (!pfkbak_get_info( &info ))
        {
            delete bt;
            fprintf(stderr, "unable to validate database!\n");
            return 1;
        }

#if SEPARATE_DATA_FILE
        fbi = FileBlockInterface::openFile( pfkbak_data_file,
                                            DATA_CACHE_SIZE );

        if (!fbi)
        {
            fprintf(stderr, "Unable to open backup data file: %s: %s\n",
                    pfkbak_data_file, strerror(errno));
            return 1;
        }
#else
        fbi = bt->get_fbi();
#endif
    }

    pfkbak_meta = bt;
    pfkbak_data = fbi;

    switch (pfkbak_op)
    {
    case BAK_CREATE_BACKUP:
        baknum = pfkbak_find_backup(pfkbak_name);
        if (baknum != 0)
        {
            delete bt;
            fprintf(stderr, "a backup named '%s' already exists "
                    "in this database.\n", pfkbak_name);
            return 1;
        }
        break;

    case BAK_DELETE_BACKUP:
    case BAK_UPDATE_BACKUP:
    case BAK_DELETE_GENS:
    case BAK_LIST_FILES:
    case BAK_EXTRACT:
    case BAK_EXTRACT_LIST:
        baknum = pfkbak_find_backup(pfkbak_name);
        if (baknum == 0)
        {
            delete bt;
            fprintf(stderr, "a backup named '%s' is not found "
                    "in this database.\n", pfkbak_name);
            return 1;
        }
        break;
    }

    // now call the relevant function to handle the op.
    switch (pfkbak_op)
    {
    case BAK_CREATE_FILE:
        pfkbak_create_file( pfkbak_file );
        break;
    case BAK_LIST_BACKUPS:
        pfkbak_list_backups();
        break;
    case BAK_CREATE_BACKUP:
        pfkbak_create_backup( pfkbak_name, argv[4], argv[5] );
        break;
    case BAK_DELETE_BACKUP:
        pfkbak_delete_backup( baknum );
        break;
    case BAK_UPDATE_BACKUP:
        pfkbak_update_backup( baknum );
        break;
    case BAK_DELETE_GENS:
        pfkbak_delete_gens( baknum, argc-4, argv+4 );
        break;
    case BAK_LIST_FILES:
        pfkbak_list_files( baknum, atoi(argv[4]) );
        break;
    case BAK_EXTRACT:
        pfkbak_extract( baknum, atoi(argv[4]), argc-5, argv+5 );
        break;
    case BAK_EXTRACT_LIST:
        pfkbak_extract_list( baknum, atoi(argv[4]), argv[5] );
        break;
    case BAK_NONE:
    default:
        // shouldn't be reached but it satisifies compiler.
        usage();
    }

    delete bt;
#if SEPARATE_DATA_FILE
    delete fbi;
#endif
    return 0;
}
