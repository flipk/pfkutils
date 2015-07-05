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

/** \file delete_gen.cc
 * \brief Defines method to delete a generation or a range of generations
 *        from a backup.
 * \author Phillip F Knaack
 */

#include "database_elements.h"
#include "params.h"
#include "protos.h"
#include "FileList.h"

#include <stdlib.h>
#include <time.h>

/** delete a generation or a range of generations from a backup.
 *
 * @param baknum  The backup ID number of the backup to change.
 * @param gen_num_s  The starting range of generations to delete.
 * @param gen_num_e  The ending range of generations to delete.
 */
void
pfkbak_delete_gen ( UINT32 baknum,
                    UINT32 gen_num_s, UINT32 gen_num_e )
{
    UINT32 in, out;

    printf("deleting generations %d thru %d\n", gen_num_s, gen_num_e);

    PfkBackupInfo backup_info(pfkbak_meta);

    backup_info.key.backup_number.v = baknum;

    if (!backup_info.get())
    {
        fprintf(stderr, "unable to fetch backup info\n");
        return;
    }

    {
        BST_ARRAY <PfkBakGenInfo> * genlist = &backup_info.data.generations;

        // delete generations from the backup_info
        for (in = out = 0; in < genlist->num_items; in++)
        {
            PfkBakGenInfo * gi = genlist->array[in];
            if (gi->generation_number.v < gen_num_s  ||
                gi->generation_number.v > gen_num_e)
            {
                if (in != out)
                {
                    genlist->array[in] = genlist->array[out];
                    genlist->array[out] = gi;
                }
                out++;
            }
        }
        genlist->alloc(out);
    }
    backup_info.put(true);

    UINT32 file_count = backup_info.data.file_count.v;
    UINT32 file_number;
    time_t now, last_progress;

    time( &last_progress );

    for (file_number = 0; file_number < file_count; file_number++)
    {
        PfkBackupFileInfo  file_info(pfkbak_meta);

        if (time( &now ) != last_progress)
        {
            UINT32 progress = file_number * 1000 / file_count;
            if (pfkbak_verb == VERB_1)
            {
                printf("\rprogress: %3d.%d%% ", progress/10, progress%10);
                fflush(stdout);
            }
            last_progress = now;
        }

        file_info.key.backup_number.v = baknum;
        file_info.key.file_number.v = file_number;

        if (!file_info.get())
            // skip the hole.
            continue;

        if (pfkbak_verb == VERB_2)
        {
            printf("%s", file_info.data.file_path.string);
            fflush(stdout);
        }

        {
            BST_ARRAY  <BST_UINT32_t> * genlist = &file_info.data.generations;
            for (in = out = 0; in < genlist->num_items; in++)
            {
                UINT32 g = genlist->array[in]->v;
                if (g < gen_num_s  ||  g > gen_num_e)
                {
                    if (in != out)
                    {
                        genlist->array[in]->v = genlist->array[out]->v;
                        genlist->array[out]->v = g;
                    }
                    out++;
                }
            }
            genlist->alloc(out);
        }
        if (out > 0)
            file_info.put(true);
        else
            file_info.del();

        UINT32 piece_number;
        for (piece_number = 0; ; piece_number++)
        {
            PfkBackupFilePieceInfo piece_info(pfkbak_meta);

            piece_info.key.backup_number.v = baknum;
            piece_info.key.file_number.v = file_number;
            piece_info.key.piece_number.v = piece_number;

            if (!piece_info.get())
                // probably end of file.
                break;

            BST_ARRAY <PfkBackupVersion> * verlist =
                &piece_info.data.versions;

            for (in = out = 0; in < verlist->num_items; in++)
            {
                PfkBackupVersion * ver = verlist->array[in];
                if (ver->gen_number.v < gen_num_s  ||
                    ver->gen_number.v > gen_num_e)
                {
                    if (in != out)
                    {
                        verlist->array[in] = verlist->array[out];
                        verlist->array[out] = ver;
                    }
                    out++;
                }
                else
                {
                    PfkBackupFilePieceData piece_data(pfkbak_meta);

                    piece_data.key.backup_number.v = baknum;
                    piece_data.key.file_number.v = file_number;
                    piece_data.key.piece_number.v = piece_number;
                    piece_data.key.md5hash.alloc(MD5_DIGEST_SIZE);
                    memcpy( piece_data.key.md5hash.binary,
                            ver->md5hash.binary,
                            MD5_DIGEST_SIZE );

                    if (!piece_data.get())
                    {
                        fprintf(stderr,
                                "%s:%s:%d: ERROR should not be here\n",
                                __FILE__, __FUNCTION__, __LINE__);
                    }
                    else
                    {
                        if (--piece_data.data.refcount.v == 0)
                        {
                            pfkbak_data->free( piece_data.data.data_fbn.v );
                            piece_data.del();
                        }
                        else
                        {
                            piece_data.put(true);
                        }
                    }
                }
            }
            verlist->alloc(out);
            if (out > 0)
                piece_info.put(true);
            else
                piece_info.del();
        }
        if (pfkbak_verb == VERB_2)
            printf("\n");
    }
    if (pfkbak_verb == VERB_1)
        printf("\rprogress: %3d.%d%%\n", 100, 0);
}
