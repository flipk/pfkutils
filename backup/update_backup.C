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
#include "protos.H"

#include <FileList.H>

#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

enum file_state { STATE_NEW, STATE_MODIFIED, STATE_UNMODIFIED };

bool
walk_file( file_state state, Btree * bt,
           UINT32 baknum, UINT32 file_number, UINT32 gen_num,
           PfkBackupFileInfo * file_info, TSFileEntryFile * fef )
{
    UINT32 piece_number;
    int fd = -1;
    int idx;

    if (state != STATE_UNMODIFIED)
    {
        file_info->data.size. v = fef->size;
        file_info->data.mtime.v = fef->mtime;

        fd = open(fef->path, O_RDONLY);
        if ( fd < 0 )
        {
            fprintf(stderr, "ERROR : modified but cannot open: %s\n",
                    fef->path);
            fd = -1;
        }
    }

    // read in all the pieces of this file, comparing md5
    // hashes, and either adding new pieces, or bumping the
    // refcounts of existing pieces.

    int piece_len;
    int PIECE_SIZE = PfkBackupFilePieceDataData::PIECE_SIZE;
    UCHAR buffer[PIECE_SIZE];
    UCHAR  md5hash[MD5_DIGEST_SIZE];
    PfkBackupFilePieceInfo   piece_info(bt);
    PfkBackupFilePieceData   piece_data(bt);

    // walk all the pieces.
    for (piece_number = 0; ; piece_number++)
    {
        if (state != STATE_UNMODIFIED)
        {
            piece_len = read(fd, buffer, PIECE_SIZE);
            if (piece_len < 0)
                fprintf(stderr, "ERROR : reading %s: %s\n",
                        fef->path, strerror(errno));
            if (piece_len == 0)
                break;

            pfkbak_md5_buffer(buffer, piece_len, md5hash);
        }

        piece_info.key.backup_number.v = baknum;
        piece_info.key.file_number.v = file_number;
        piece_info.key.piece_number.v = piece_number;

        if (!piece_info.get())
        {
            if (state == STATE_UNMODIFIED)
                // done.
                break;

            // otherwise, file grew in size and added pieces,
            // or was new and pieces never existed at all.
            piece_info.data.versions.alloc(1);
            piece_info.data.versions.array[0]->gen_number.v = gen_num;
            memcpy( piece_info.data.versions.array[0]->md5hash.binary,
                    md5hash, MD5_DIGEST_SIZE);
        }
        else
        {
            // xxx

            // should not ever get here if state is new.
            // unmodified won't have opened the file, so we have to
            // find the most recent generation's info and copy the
            // md5sum.
            // modified will have opened the file, so we should search
            // the list to see if this md5 exists already, and if it does,
            // copy it, if not, make a new entry.

            // must also fetch the PfkBackupFilePieceData and bump
            // the reference count, if an existing md5 was just copied.
        }

        piece_info.put(true);
    }

    if (fd != -1)
        close(fd);
}

void
pfkbak_update_backup ( Btree * bt, UINT32 baknum )
{
    PfkBackupInfo   bakinfo(bt);
    UINT32  gen_num;

    bakinfo.key.backup_number.v = baknum;

    if (!bakinfo.get())
    {
        fprintf(stderr, "unable to fetch backup info!!\n");
        return;
    }

    if (chdir(bakinfo.data.root_dir.string) < 0)
    {
        fprintf(stderr, "changing to backup root directory: %s\n",
                strerror(errno));
        return;
    }

    gen_num = bakinfo.data.next_generation_number.v++;
    printf("creating generation %d\n", gen_num);

    int gen_index = bakinfo.data.generations.num_items;
    bakinfo.data.generations.alloc(gen_index+1);

    PfkBakGenInfo * gen_info = bakinfo.data.generations.array[gen_index];

    char current_time[32];
    time_t  now;

    time(&now);
    ctime_r(&now, current_time);
    current_time[strlen(current_time)-1]=0;

    printf("current date and time: %s\n", current_time);

    gen_info->date_time.set(current_time);
    gen_info->generation_number.v = gen_num;

    // generate a file list.

    TSFileEntryList * fel;
    fel = treesync_generate_file_list(".");

    // put all the found file names in a hash for quick searches.

    TSFileEntryHash   hash;
    union {
        TSFileEntry     * fe;
        TSFileEntryFile * fef;
    } fe;
    TSFileEntry     * nfe;

    for (fe.fe = fel->get_head(); fe.fe; fe.fe = nfe)
    {
        nfe = fel->get_next(fe.fe);
        if (fe.fe->type == TSFileEntry::TYPE_FILE)
            hash.add(fe.fe);
        else
        {
            fel->remove(fe.fe);
            delete fe.fe;
        }
    }

    UINT32 file_number;

    // walk through all file entries in the database, looking
    // to see if a file we found has changed.

    struct unused_file_number {
        struct unused_file_number * next;
        UINT32  file_number;
    };
    unused_file_number * unused_head = NULL;
    PfkBackupFileInfo  file_info(bt);

    for (file_number = 0;
         file_number < bakinfo.data.file_count.v;
         file_number++)
    {
        file_info.key.backup_number.v = baknum;
        file_info.key.file_number.v = file_number;

        if (!file_info.get())
        {
            // this file number no longer exists. skip it.
            // and record it just in case we want to use it
            // later for a new file.
            unused_file_number * ufn = new unused_file_number;
            ufn->file_number = file_number;
            ufn->next = unused_head;
            unused_head = ufn;
            continue;
        }

        // look for file in hash.

        fe.fe = hash.find(file_info.data.file_path.string);
        if (!fe.fe)
        {
            // file was deleted!
            // do nothing with the database, which will cause this
            // file entry in the database to reflect that this file
            // is not a part of this generation.
        }
        else
        {
            fel->remove(fe.fe);
            hash.remove(fe.fe);

            // file found!
            if ( (fe.fef->size  != file_info.data.size.v )  ||
                 (fe.fef->mtime != file_info.data.mtime.v)  )
            {
                // file was modified!

                walk_file( STATE_MODIFIED, bt,
                           baknum, file_number, gen_num,
                           &file_info, fe.fef );
            }
            else
            {
                // file unchanged!
                // walk through all the pieces in the database, adding
                // references to this generation number and bumping all
                // the refcounts.

                walk_file( STATE_UNMODIFIED, bt, 
                           baknum, file_number, gen_num,
                           &file_info, fe.fef );
            }

            // update the FileInfo to indicate it is a member of
            // this generation.
            gen_index = file_info.data.generations.num_items;
            file_info.data.generations.alloc(gen_index+1);
            file_info.data.generations.array[gen_index]->v = gen_num;
            
            file_info.put(true);

            delete fe.fe;
        }
    }

    // anything left on fel, is a new file.

    for (fe.fe = fel->get_head(); fe.fe; fe.fe = nfe)
    {
        nfe = fel->get_next(fe.fe);
        hash.remove(fe.fe);
        fel->remove(fe.fe);

        // file created!

        if (unused_head != NULL)
        {
            // re-use an unused file number, if one exists.
            unused_file_number * ufn = unused_head;
            unused_head = ufn->next;
            file_number = ufn->file_number;
            delete ufn;
        }
        else
        {
            // allocate a new file number if none are unused.
            file_number = bakinfo.data.file_count.v++;
        }

        file_info.key.backup_number.v = baknum;
        file_info.key.file_number.v = file_number;

        file_info.data.file_path.set(fe.fe->path);
        file_info.data.generations.alloc(1);
        file_info.data.generations.array[0]->v = gen_num;

        walk_file( STATE_NEW, bt,
                   baknum, file_number, gen_num,
                   &file_info, fe.fef );

        file_info.put(true);

        delete fe.fe;
    }
    delete fel;

    while (unused_head != NULL)
    {
        unused_file_number * ufn;

        ufn = unused_head;
        unused_head = ufn->next;

        delete ufn;
    }

    // write back into the database that a new generation
    // exists.

    bakinfo.put(true);
}
