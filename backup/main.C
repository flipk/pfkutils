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
#include "protos.H"

void
usage(void)
{
    fprintf(stderr, "usage: \n"
"create file:    pfkbak -C[Vv] BACKUP-FILE\n"
"\n"
"list backups:   pfkbak -L[Vv] BACKUP-FILE\n"
"create backup:  pfkbak -c[Vv] BACKUP-FILE:BACKUP-NAME DIR\n"
"delete backup:  pfkbak -D[Vv] BACKUP-FILE:BACKUP-NAME\n"
"update backup:  pfkbak -u[Vv] BACKUP-FILE:BACKUP-NAME DIR\n"
"\n"
"delete gens:    pfkbak -d[Vv] BACKUP-FILE:BACKUP-NAME GenN A-B -B\n"
"\n"
"list files:     pfkbak -l[Vv] BACKUP-FILE:BACKUP-NAME GenN\n"
"\n"
"extract:        pfkbak -e[Vv] BACKUP-FILE:BACKUP-NAME GenN [FILE...]\n"
"extract list:   pfkbak -E[Vv] BACKUP-FILE:BACKUP-NAME GenN LIST-FILE\n"
            "\n"
        );
    exit(1);
}

backop     pfkbak_op;
verblevel  pfkbak_verb;
char *     pfkbak_file;
char *     pfkbak_name;
int        pfkbak_gen;

static void
parse_filename(char * arg)
{
    pfkbak_file = strdup(arg);

    char * colon = strchr(pfkbak_file, ':');
    if (colon)
    {
        *colon = 0;
        pfkbak_name = colon+1;
    }
    else
        pfkbak_name = NULL;

    fprintf(stderr,"backup file: %s\n", pfkbak_file);
    fprintf(stderr,"backup name: %s\n",
            pfkbak_name ? pfkbak_name : "(not specified)");
    fprintf(stderr,"verbose level: %s\n",
            pfkbak_verb == VERB_QUIET ? "quiet" :
            pfkbak_verb == VERB_1 ? "print filenames" :
            pfkbak_verb == VERB_2 ? "print filenames and blocks" : 
            "(unknown)");
    fprintf(stderr,"\n");
}

int
main(int argc, char ** argv)
{
    pfkbak_op = BAK_NONE;
    pfkbak_verb = VERB_QUIET;

    if (argc < 3 || argv[1][0] != '-')
        usage();

    char * cmd = &argv[1][1];
    if (cmd[1] == 'v')
        pfkbak_verb = VERB_1;
    else if (cmd[1] == 'V')
        pfkbak_verb = VERB_2;

    parse_filename(argv[2]);

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
    case BAK_DELETE_BACKUP:
        if (argc != 3) usage(); break;

    case BAK_CREATE_BACKUP:
    case BAK_UPDATE_BACKUP:
    case BAK_LIST_FILES:
        if (argc != 4) usage(); break;

    case BAK_DELETE_GENS:
        if (argc < 4)  usage(); break;

    case BAK_EXTRACT:
        if (argc < 5)  usage(); break;

    case BAK_EXTRACT_LIST:
        if (argc != 5) usage(); break;

    case BAK_NONE:
    default:
        // shouldn't be reached but it satisifies compiler.
        usage();
    }

    // verify presence or absence of backup name
    switch (pfkbak_op)
    {
    case BAK_CREATE_FILE:
    case BAK_LIST_BACKUPS:
        if (pfkbak_name != NULL) usage(); break;

    case BAK_CREATE_BACKUP:
    case BAK_DELETE_BACKUP:
    case BAK_UPDATE_BACKUP:
    case BAK_LIST_FILES:
    case BAK_DELETE_GENS:
    case BAK_EXTRACT:
    case BAK_EXTRACT_LIST:
        if (pfkbak_name == NULL) usage(); break;

    case BAK_NONE:
    default:
        // shouldn't be reached but it satisifies compiler.
        usage();
    }

    Btree * bt = NULL;

    // create or open btree file
    if (pfkbak_op == BAK_CREATE_FILE)
        bt = Btree::createFile( pfkbak_file, CACHE_SIZE,
                                0600, BTREE_ORDER );
    else
        bt = Btree::openFile( pfkbak_file, CACHE_SIZE );

    if (!bt)
    {
        fprintf(stderr, "unable to %s file: %s: %s\n",
                pfkbak_op == BAK_CREATE_FILE ? "create" : "open",
                pfkbak_file, strerror(errno));
        return 1;
    }

    // now call the relevant function to handle the op.
    switch (pfkbak_op)
    {
    case BAK_CREATE_FILE:
        pfkbak_create_file( bt );
        break;
    case BAK_LIST_BACKUPS:
        pfkbak_list_backups( bt );
        break;
    case BAK_CREATE_BACKUP:
        pfkbak_create_backup( bt, argv[3] );
        break;
    case BAK_DELETE_BACKUP:
        pfkbak_delete_backup( bt );
        break;
    case BAK_UPDATE_BACKUP:
        pfkbak_update_backup( bt, argv[3] );
        break;
    case BAK_DELETE_GENS:
        pfkbak_delete_gens( bt, argc-3, argv+3 );
        break;
    case BAK_LIST_FILES:
        pfkbak_list_files( bt, argv[3] );
        break;
    case BAK_EXTRACT:
        pfkbak_extract( bt, argv[3], argc-4, argv+4 );
        break;
    case BAK_EXTRACT_LIST:
        pfkbak_extract_list( bt, argv[3], argv[4] );
        break;
    case BAK_NONE:
    default:
        // shouldn't be reached but it satisifies compiler.
        usage();
    }


    delete bt;

    return 0;
}
