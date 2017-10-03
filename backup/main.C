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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "Btree.H"
#include "params.H"
#include "database_elements.H"
#include "protos.H"

const char * PfkBackupDbInfoKey::INFO_KEY = "PfkBakDbInfo";

static void
usage(void)
{
    fprintf(stderr, "usage: \n"
"create file:    pfkbak -C[Vv] BACKUP-FILE\n"
"\n"
"list backups:   pfkbak -L[Vv] BACKUP-FILE\n"
"create backup:  pfkbak -c[Vv] BACKUP-FILE BACKUP-NAME DIR COMMENT\n"
"delete backup:  pfkbak -D[Vv] BACKUP-FILE BACKUP-NAME\n"
"update backup:  pfkbak -u[Vv] BACKUP-FILE BACKUP-NAME\n"
"\n"
"delete gens:    pfkbak -d[Vv] BACKUP-FILE BACKUP-NAME GenN A-B -B\n"
"\n"
"list files:     pfkbak -l[Vv] BACKUP-FILE BACKUP-NAME GenN\n"
"\n"
"extract:        pfkbak -e[Vv] BACKUP-FILE BACKUP-NAME GenN [FILE...]\n"
"extract list:   pfkbak -E[Vv] BACKUP-FILE BACKUP-NAME GenN LIST-FILE\n"
            "\n"
        );
    exit(1);
}

verblevel  pfkbak_verb;

extern "C" int
pfkbak_main(int argc, char ** argv)
{
    backop     pfkbak_op;
    char *     pfkbak_file;
    char *     pfkbak_name;
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

    // create or open btree file
    if (pfkbak_op == BAK_CREATE_FILE)
    {
        bt = Btree::createFile( pfkbak_file, CACHE_SIZE,
                                0600, BTREE_ORDER );
    }
    else
    {
        bt = Btree::openFile( pfkbak_file, CACHE_SIZE );

        PfkBackupDbInfo   info(bt);
        if (!pfkbak_get_info( &info ))
        {
            delete bt;
            fprintf(stderr, "unable to validate database!\n");
            return 1;
        }
    }

    if (!bt)
    {
        fprintf(stderr, "unable to %s file: %s: %s\n",
                pfkbak_op == BAK_CREATE_FILE ? "create" : "open",
                pfkbak_file, strerror(errno));
        return 1;
    }

    switch (pfkbak_op)
    {
    case BAK_CREATE_BACKUP:
        baknum = pfkbak_find_backup(bt, pfkbak_name);
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
        baknum = pfkbak_find_backup(bt, pfkbak_name);
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
        pfkbak_create_file( pfkbak_file, bt );
        break;
    case BAK_LIST_BACKUPS:
        pfkbak_list_backups( bt );
        break;
    case BAK_CREATE_BACKUP:
        pfkbak_create_backup( bt, pfkbak_name, argv[4], argv[5] );
        break;
    case BAK_DELETE_BACKUP:
        pfkbak_delete_backup( bt, baknum );
        break;
    case BAK_UPDATE_BACKUP:
        pfkbak_update_backup( bt, baknum );
        break;
    case BAK_DELETE_GENS:
        pfkbak_delete_gens( bt, baknum, argc-4, argv+4 );
        break;
    case BAK_LIST_FILES:
        pfkbak_list_files( bt, baknum, atoi(argv[4]) );
        break;
    case BAK_EXTRACT:
        pfkbak_extract( bt, baknum, atoi(argv[4]), argc-5, argv+5 );
        break;
    case BAK_EXTRACT_LIST:
        pfkbak_extract_list( bt, baknum, atoi(argv[4]), argv[5] );
        break;
    case BAK_NONE:
    default:
        // shouldn't be reached but it satisifies compiler.
        usage();
    }

    delete bt;
    return 0;
}
