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

#include "database_elements.h"
#include "params.h"
#include "protos.h"
#include "FileList.h"

#include <stdlib.h>

void
pfkbak_list_files    ( UINT32 baknum, UINT32 gen_num )
{
    fprintf(stderr,
            "listing generation %d in backup number %d\n", gen_num, baknum);

    PfkBackupInfo   bakinfo(pfkbak_meta);

    bakinfo.key.backup_number.v = baknum;

    if (!bakinfo.get())
    {
        fprintf(stderr, "unable to fetch backup info\n");
        return;
    }

    UINT32 number_files = bakinfo.data.file_count.v;

    fprintf(stderr,
            "backup name: %s\n"
            "comment: %s\n"
            "files: %d\n",
            bakinfo.data.name.string,
            bakinfo.data.comment.string,
            number_files);

    UINT32 file_number;
    int idx, piece_number;

    for (file_number = 0; file_number < number_files; file_number++)
    {
        PfkBackupFileInfo  file_info(pfkbak_meta);

        file_info.key.backup_number.v = baknum;
        file_info.key.file_number.v = file_number;

        if (!file_info.get())
        {
            if (pfkbak_verb > VERB_QUIET)
                printf("unused file number %d\n", file_number);
            continue;
        }

        if (pfkbak_verb == VERB_QUIET)
        {
            for (idx = 0; idx < file_info.data.generations.num_items; idx++)
                if (gen_num == file_info.data.generations.array[idx]->v)
                    printf("%s\n", file_info.data.file_path.string);
            continue;
        }

        printf("%lld\t%s\n",
               file_info.data.size.v,
               file_info.data.file_path.string);

        printf("  gens: ");
        for (idx = 0; idx < file_info.data.generations.num_items; idx++)
        {
            gen_num = file_info.data.generations.array[idx]->v;
            printf("%d ", gen_num);
        }
        printf("\n");

        if (pfkbak_verb != VERB_2)
            continue;

        for (piece_number = 0; ; piece_number++)
        {
            PfkBackupFilePieceInfo piece_info(pfkbak_meta);

            piece_info.key.backup_number.v = baknum;
            piece_info.key.file_number.v = file_number;
            piece_info.key.piece_number.v = piece_number;

            if (!piece_info.get())
                break;

            printf("    piece %d\n", piece_number);

            BST_ARRAY <PfkBackupVersion> * v = &piece_info.data.versions;

            for (idx = 0; idx < v->num_items; idx++)
            {
                char md5string[MD5_STRING_LEN];

                pfkbak_sprint_md5( v->array[idx]->md5hash.binary, md5string );
                printf( "      gen %d hash %s ",
                        v->array[idx]->gen_number.v, md5string );

                PfkBackupFilePieceData piece_data(pfkbak_meta);

                piece_data.key.backup_number.v = baknum;
                piece_data.key.file_number.v = file_number;
                piece_data.key.piece_number.v = piece_number;
                piece_data.key.md5hash.alloc(MD5_DIGEST_SIZE);
                memcpy( piece_data.key.md5hash.binary,
                        v->array[idx]->md5hash.binary,
                        MD5_DIGEST_SIZE );

                if (!piece_data.get())
                {
                    fprintf(stderr, "unable to fetch piece data\n");
                }
                else
                {
                    printf(" ref %d cs %d us %d fbn %d",
                           piece_data.data.refcount.v,
                           piece_data.data.csize.v,
                           piece_data.data.usize.v,
                           piece_data.data.data_fbn.v);
                }

                printf("\n");
            }
        }
    }
}
