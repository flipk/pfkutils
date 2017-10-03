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

/** \file extract.C
 * \brief Extract files from a backup.
 * \author Phillip F Knaack
 */

#include "database_elements.H"
#include "params.H"
#include "protos.H"
#include "FileList.H"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <zlib.h>

#define BAIL() \
    do { \
    fprintf(stderr, "%s:%s:%d: error\n", __FILE__, __FUNCTION__, __LINE__); \
    return; \
    } while (0)

/** a helper function which ensures directories exist when opening a file.
 * if the component directories don't exist, this function creates them.
 * it's sort of like "mkdir -p <path>".
 *
 * @param in_path   the path (relative to current working dir) to file to open.
 * @param mode      the file mode to open the file with.
 * @return a file descriptor or negative if there was a failure. errno is 
 *         set in the event of a failure.
 */
static int
openfile( char *in_path, UINT16 mode )
{
    int len = strlen(in_path)+1;
    int num_comps = 1;
    int i, j;
    char * p;

    // count the path components.
    for (p = in_path; *p; p++)
        if (*p == '/')
            num_comps++;

    // copy the path into all of them
    char comps[num_comps][len];
    for (i = 0; i < num_comps; i++)
        memcpy(comps[i], in_path, len);

    // search for slashes and null them out
    for (i = j = 0; j < len; j++)
        if (in_path[j] == '/')
            comps[i++][j] = 0;

    // create component directories if they don't exist
    for (i=0; i < (num_comps-1); i++)
        if (mkdir( comps[i], 0700 ) < 0)
            if (errno != EEXIST)
                fprintf(stderr, "mkdir '%s': %s\n",
                        comps[i], strerror(errno));

    // and open the final file.
    return open( comps[i], O_WRONLY | O_CREAT, mode );
}

/** extract files from a backup. 
 *  
 * \todo currently argc and argv are ignored, this should be implemented.
 *
 * @param baknum the backup ID number.
 * @param gen_num which generation number to extract from the backup.
 * @param argc  count of arguments following on the command line. if argc is 
 *              zero, it means extract everything.
 * @param argv  array of arguments on the command line; each arg represents
 *              a file to be extracted.
 */
void
pfkbak_extract       ( UINT32 baknum,
                       UINT32 gen_num, int argc, char ** argv )
{
    PfkBackupInfo back_info(pfkbak_meta);

    back_info.key.backup_number.v = baknum;

    if (!back_info.get())
        BAIL();

    int idx;
    {
        // validate that this gen# actually exists in this backup.

        BST_ARRAY <PfkBakGenInfo> * gens = &back_info.data.generations;
        for (idx = 0; idx < gens->num_items; idx++)
            if (gens->array[idx]->generation_number.v == gen_num)
                break;

        if (idx == gens->num_items)
        {
            fprintf(stderr, "generation %d not found in backup\n", gen_num);
            return;
        }
    }

    (void) umask( 000 );

    if (mkdir( back_info.data.name.string, 0700 ) < 0)
    {
        fprintf(stderr, "unable to create directory '%s': %s\n",
                back_info.data.name.string,
                strerror(errno));
        return;
    }
    (void) chdir( back_info.data.name.string );

    UINT32 num_files = back_info.data.file_count.v;
    UINT32 file_number;
    for (file_number = 0; file_number < num_files; file_number++)
    {
        PfkBackupFileInfo file_info(pfkbak_meta);

        file_info.key.backup_number.v = baknum;
        file_info.key.file_number.v = file_number;

        if (!file_info.get())
            // deleted files will leave holes, that's okay.
            continue;

        BST_ARRAY <BST_UINT32_t> * gens = &file_info.data.generations;

        // validate this file is actually part of this generation.

        for (idx = 0; idx < gens->num_items; idx++)
            if (gens->array[idx]->v == gen_num)
                break;

        if (idx == gens->num_items)
            // this file is not part of this generation.
            continue;

        char * file_path = file_info.data.file_path.string;

        // do a tar-like display, show the file name without
        // newline; don't display the newline until extraction
        // of the file is complete.

        /** \todo support extracting individual files */

        if (pfkbak_verb > VERB_QUIET)
        {
            printf("%s", file_path);
            fflush(stdout);
        }

        int fd = openfile(file_path, file_info.data.mode.v);

        if (fd < 0)
        {
            printf(": unable to create: %s\n", strerror(errno));
            continue;
        }

        PfkBackupFilePieceInfo piece_info(pfkbak_meta);
        PfkBackupFilePieceData piece_data(pfkbak_meta);
        UINT32 piece_number;

        for (piece_number = 0; ; piece_number++)
        {
            piece_info.key.backup_number.v = baknum;
            piece_info.key.file_number.v = file_number;
            piece_info.key.piece_number.v = piece_number;

            if (!piece_info.get())
                // end of file? probably.
                break;

            BST_ARRAY <PfkBackupVersion> * v = &piece_info.data.versions;

            for (idx = 0; idx < v->num_items; idx++)
                if (v->array[idx]->gen_number.v == gen_num)
                    break;

            if (idx == v->num_items)
            {
                // this is not really a bug; if a file gets shorter
                // from one gen to the next, later pieces will not be
                // tagged with this generation#, and so this is an
                // expected result.
                if (pfkbak_verb > VERB_QUIET)
                    printf(": piece %d info not found\n", piece_number);
            }
            else
            {
                piece_data.key.backup_number.v = baknum;
                piece_data.key.file_number.v = file_number;
                piece_data.key.piece_number.v = piece_number;
                piece_data.key.md5hash.alloc(MD5_DIGEST_SIZE);
                memcpy( piece_data.key.md5hash.binary,
                        v->array[idx]->md5hash.binary,
                        MD5_DIGEST_SIZE );

                if (!piece_data.get())
                {
                    printf(": piece %d data not found\n", piece_number);
                }
                else
                {
                    UINT32 fbn = piece_data.data.data_fbn.v;
                    UINT16 usize = piece_data.data.usize.v;
                    UINT16 csize = piece_data.data.csize.v;

                    FileBlock * fb = pfkbak_data->get( fbn );

                    if (!fb)
                    {
                        printf(": unable to fetch data for piece %d\n",
                               piece_number);
                    }
                    else
                    {
                        UCHAR ubuf[ usize ];
                        uLongf ulen = usize;

                        if (usize == csize)
                        {
                            (void) write( fd, fb->get_ptr(), csize );
                        }
                        else
                        {
                            (void) uncompress( (Bytef*)ubuf, &ulen,
                                               fb->get_ptr(), csize );
                            (void) write( fd, ubuf, ulen );
                        }
                        pfkbak_data->release( fb );
                    }
                }
            }
        }

        close(fd);
        if (pfkbak_verb > VERB_QUIET)
            printf("\n");
    }
}
