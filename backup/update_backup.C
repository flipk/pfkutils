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

// NOTE: BST_FIXED_BINARY type was removed, thus possibly breaking
//       all of the 'md5hash' fields in this program. if you see weird
//       bus faults in this code around that field, that is probably why.
//       i never got around to fixing it.

/** \file update_backup.C
 * \brief update or freshen a backup: add a generation.
 * \author Phillip F Knaack
 */

#include "database_elements.H"
#include "params.H"
#include "protos.H"
#include "FileList.H"

#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <zlib.h>

/** identifier for the detected state of the file, according to
 * its timestamp and size field. */
enum file_state {
    STATE_NEW,         /**< the file appears to be new, no previous record
                          of it in the database. */
    STATE_MODIFIED,    /**< the file appears to be modified, its timestamp
                          and/or size does not match the database. */
    STATE_UNMODIFIED   /**< the file appears to have exactly the same 
                          timestamp and size as recorded in the database. */
};

static UINT64 bytes_written;

/** add the data for a file piece to the database.
 * this function will also compress the data before adding it,
 * if compression seems relevant; if compression results in expansion,
 * (because file appears already compressed) just skip compression for
 * the remainder of the file to save time.
 * 
 * @param baknum  the backup ID
 * @param file_number the file being worked on in the backup.
 * @param piece_number the piece number of this file.
 * @param md5hash the calculated m5 hash of this piece
 * @param buffer a pointer to the data for the piece
 * @param usize the (uncompressed) size of this buffer
 *
 * @return true if data added okay, false if there was an error.
 */
static bool
put_piece_data( UINT32 baknum, UINT32 file_number,
                UINT32 piece_number, UCHAR * md5hash,
                UCHAR * buffer, int usize )
{
    PfkBackupFilePieceData   piece_data(pfkbak_meta);
    UINT32 data_fbn = 0;
    UCHAR * final_buffer;
    UINT16  final_size;

    // unfortunately, we have to compress to a temp buffer
    // and then copy it to the FileBlock, because we don't know
    // how big to make the FileBlock until the compression is
    // complete! bummer.

    uLongf csize = compressBound( usize );
    UCHAR cbuf[ csize ];

    bytes_written += usize;

    (void) compress2( cbuf, &csize,
                      (const Bytef*)buffer, usize,
                      Z_BEST_SPEED );

    if (csize < usize)
    {
        final_buffer = cbuf;
        final_size = csize;
    }
    else
    {
        final_buffer = buffer;
        final_size = usize;
    }

    data_fbn = pfkbak_data->alloc( final_size );
    if (data_fbn == 0)
    {
        fprintf(stderr, "ERROR: %s:%s:%d shouldn't be here\n",
                __FILE__, __FUNCTION__, __LINE__);
        return false;
    }
    else
    {
        FileBlock * data_fb = pfkbak_data->get( data_fbn, true );
        if (!data_fb)
        {
            fprintf(stderr, "ERROR: %s:%s:%d shouldn't be here\n",
                    __FILE__, __FUNCTION__, __LINE__);
            return false;
        }
        else
        {
            memcpy( data_fb->get_ptr(), final_buffer, final_size );
            data_fb->mark_dirty();
            pfkbak_data->release( data_fb );
        }
    }

    piece_data.key.backup_number.v = baknum;
    piece_data.key.file_number.v = file_number;
    piece_data.key.piece_number.v = piece_number;
    memcpy( piece_data.key.md5hash.binary, md5hash, MD5_DIGEST_SIZE );

    piece_data.data.refcount.v = 1;
    piece_data.data.csize.v = final_size;
    piece_data.data.usize.v = usize;
    piece_data.data.data_fbn.v = data_fbn;

    piece_data.put(true);

    return true;
}

/** do all work required for updating one file in the database.
 * This function will open the file if required, read in all the
 * pieces, calculate md5 hashes, and update all pieces if required.
 * If not required, it will just reference the last copy of each
 * piece in the database.
 *
 * @param state   the state of the file according to timestamps
 * @param baknum  the backup ID
 * @param file_number the file being worked on in the backup.
 * @param gen_num  the generation number being created.
 * @param file_info  the information from the database about this file
 * @param fef     the information from treescan about this file.
 */
static void
walk_file( file_state state, 
           UINT32 baknum, UINT32 file_number, UINT32 gen_num,
           PfkBackupFileInfo * file_info, TSFileEntryFile * fef )
{
    UINT32 piece_number;
    int fd = -1;
    int idx, newidx;

    if (state != STATE_UNMODIFIED)
    {
        file_info->data.size. v = fef->size;
        file_info->data.mtime.v = fef->mtime;

        fd = open(fef->path, O_RDONLY);
        if ( fd < 0 )
        {
            fprintf(stderr, "ERROR : modified but cannot open: %s: %s\n",
                    fef->path, strerror(errno));
            fd = -1;
            // pretend the file was not modified in this case.
            state = STATE_UNMODIFIED;
        }
    }

    // read in all the pieces of this file, comparing md5
    // hashes, and either adding new pieces, or bumping the
    // refcounts of existing pieces.

    int piece_len;
    int PIECE_SIZE = PfkBackupFilePieceDataData::PIECE_SIZE;
    UCHAR buffer[PIECE_SIZE];
    UCHAR  md5hash[MD5_DIGEST_SIZE];
    PfkBackupFilePieceInfo   piece_info(pfkbak_meta);

    // walk all the pieces.
    for (piece_number = 0; ; piece_number++)
    {
        if (state != STATE_UNMODIFIED)
        {
            piece_len = read(fd, buffer, PIECE_SIZE);
            if (piece_len < 0)
            {
                int err = errno;
                fprintf(stderr, "ERROR : reading %s: %s\n",
                        fef->path, strerror(err));
                // stupid windows, or cygwin, or whatever. if you don't
                // have permission to READ the file, you should never
                // have been able to OPEN it in the first place. sigh.
                if (err == EACCES)
                {
                    // pretend the file couldn't be opened.
                    close(fd);
                    fd = -1;
                    state = STATE_UNMODIFIED;
                }
            }
            else if (piece_len == 0)
                break;
            else
                pfkbak_md5_buffer(buffer, piece_len, md5hash);
        }

        piece_info.key.backup_number.v = baknum;
        piece_info.key.file_number.v = file_number;
        piece_info.key.piece_number.v = piece_number;

        BST_ARRAY <PfkBackupVersion> * versions = & piece_info.data.versions;

        if (!piece_info.get())
        {
            if (state == STATE_UNMODIFIED)
                // end of file, most likely.
                break;

            // otherwise, file grew in size and added pieces,
            // or was a new file and pieces never existed at all.
            versions->alloc(1);
            versions->array[0]->gen_number.v = gen_num;
            memcpy( versions->array[0]->md5hash.binary,
                    md5hash, MD5_DIGEST_SIZE);

            // put data fbn.

            put_piece_data( baknum, file_number, piece_number,
                            md5hash, buffer, piece_len );
        }
        else
        {
            int res;

            switch (state)
            {
            case STATE_NEW:
                fprintf(stderr, "ERROR: %s:%s:%d shouldn't be here\n",
                        __FILE__, __FUNCTION__, __LINE__);
                break;

            case STATE_MODIFIED:

                // search the list for this md5.
                res = -1;
                for (idx = 0; idx < versions->num_items; idx++)
                {
                    res = memcmp( md5hash,
                                  versions->array[idx]->md5hash.binary,
                                  MD5_DIGEST_SIZE );
                    if (res == 0)
                        break;
                }

                // always put new gen# entries at the end of the
                // array. this way, the unmodified leg can always
                // assume the latest version is the last version.

                if (res == 0)
                {
                    // this md5 already exists. copy to new
                    // generation #

                    newidx = versions->num_items;

                    versions->alloc(newidx+1);
                    versions->array[newidx]->gen_number.v = gen_num;
                    memcpy( versions->array[newidx]->md5hash.binary,
                            versions->array[   idx]->md5hash.binary,
                            MD5_DIGEST_SIZE );

                    piece_info.put(true);

                    // then bump refcount on piecedata.

                    PfkBackupFilePieceData   piece_data(pfkbak_meta);

                    piece_data.key.backup_number.v = baknum;
                    piece_data.key.file_number.v = file_number;
                    piece_data.key.piece_number.v = piece_number;
                    memcpy( piece_data.key.md5hash.binary, md5hash,
                            MD5_DIGEST_SIZE );

                    if (!piece_data.get())
                    {
                        fprintf(stderr, "ERROR: %s:%s:%d shouldn't be here\n",
                                __FILE__, __FUNCTION__, __LINE__);
                    }
                    else
                    {
                        piece_data.data.refcount.v++;
                        piece_data.put(true);
                    }
                }
                else
                {
                    // this md5 does not exist. put new data fbn,
                    // then put new piecedata.

                    newidx = versions->num_items;

                    versions->alloc(newidx+1);
                    versions->array[newidx]->gen_number.v = gen_num;
                    memcpy( versions->array[newidx]->md5hash.binary,
                            md5hash, MD5_DIGEST_SIZE );

                    piece_info.put(true);

                    put_piece_data( baknum, file_number, piece_number,
                                    md5hash, buffer, piece_len );
                }

                break;

            case STATE_UNMODIFIED:
                // file wasn't opened; so just search the list
                // and clone the last entry. then bump refcount
                // in the piecedata.

                idx = versions->num_items-1;
                newidx = versions->num_items;

                versions->alloc(newidx+1);
                versions->array[newidx]->gen_number.v = gen_num;
                memcpy( versions->array[newidx]->md5hash.binary,
                        versions->array[   idx]->md5hash.binary,
                        MD5_DIGEST_SIZE );

                piece_info.put(true);

                // then bump refcount on piecedata.

                PfkBackupFilePieceData   piece_data(pfkbak_meta);

                piece_data.key.backup_number.v = baknum;
                piece_data.key.file_number.v = file_number;
                piece_data.key.piece_number.v = piece_number;
                memcpy( piece_data.key.md5hash.binary,
                        versions->array[   idx]->md5hash.binary,
                        MD5_DIGEST_SIZE );

                if (!piece_data.get())
                {
                    fprintf(stderr, "ERROR: %s:%s:%d shouldn't be here\n",
                            __FILE__, __FUNCTION__, __LINE__);
                }
                else
                {
                    piece_data.data.refcount.v++;
                    piece_data.put(true);
                }

                break;
            }
        }

        piece_info.put(true);
    }

    if (fd != -1)
        close(fd);
}

/**
 * Update a backup in a database.
 * This function gets into the directory for a backup, walks the
 * tree collecting timestamps on all files in the tree, and then
 * looks for updates since the last run. It adds any new data not
 * in the backup.
 *
 * @param baknum  The backup number in the database to update.
 */
void
pfkbak_update_backup ( UINT32 baknum )
{
    PfkBackupInfo   bakinfo(pfkbak_meta);
    UINT32  gen_num;

    bytes_written = 0;

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
    printf("creating generation %d for backup '%s'\n",
           gen_num, bakinfo.data.name.string);

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

    if (pfkbak_verb > VERB_QUIET)
    {
        printf("scanning... ");
        fflush(stdout);
    }

    TSFileEntryList * fel;
    fel = treesync_generate_file_list(".");

    // put all the found file names in a hash for quick searches.

    TSFileEntryHash   hash;
    union {
        TSFileEntry     * fe;
        TSFileEntryFile * fef;
    } fe;
    TSFileEntry     * nfe;
    UINT64            total_bytes = 0;
    UINT64            processed_bytes = 0;

    for (fe.fe = fel->get_head(); fe.fe; fe.fe = nfe)
    {
        nfe = fel->get_next(fe.fe);
        if (fe.fe->type == TSFileEntry::TYPE_FILE)
        {
            hash.add(fe.fe);
            total_bytes += fe.fef->size;
        }
        else
        {
            fel->remove(fe.fe);
            delete fe.fe;
        }
    }

    if (pfkbak_verb > VERB_QUIET)
    {
        printf("found %d files, %lld bytes\n",
               hash.get_cnt(), total_bytes);
        fflush(stdout);
    }

    UINT32 file_number;

    // walk through all file entries in the database, looking
    // to see if a file we found has changed.

    struct unused_file_number {
        struct unused_file_number * next;
        UINT32  file_number;
    };
    unused_file_number * unused_head = NULL;
    PfkBackupFileInfo  file_info(pfkbak_meta);
    time_t last_progress;

    time( &last_progress );

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

            processed_bytes += fe.fef->size;
            if (time( &now ) != last_progress )
            {
                UINT32 progress = processed_bytes * 1000 / total_bytes;
                if (pfkbak_verb == VERB_1)
                {
                    printf("\rprogress: %3d.%d%%   bytes: %lld  written: %lld ",
                           progress/10, progress%10,
                           processed_bytes, bytes_written);
                    fflush(stdout);
                }
                last_progress = now;
            }

            // file found!
            if ( (fe.fef->size  != file_info.data.size.v )  ||
                 (fe.fef->mtime != file_info.data.mtime.v)  )
            {
                // file was modified!

                if (pfkbak_verb == VERB_2)
                {
                    printf("%s", fe.fef->path);
                    fflush(stdout);
                }

                walk_file( STATE_MODIFIED, 
                           baknum, file_number, gen_num,
                           &file_info, fe.fef );

                if (pfkbak_verb == VERB_2)
                    printf("\n");
            }
            else
            {
                // file unchanged!
                // walk through all the pieces in the database, adding
                // references to this generation number and bumping all
                // the refcounts.

                walk_file( STATE_UNMODIFIED,  
                           baknum, file_number, gen_num,
                           &file_info, fe.fef );
            }

            // update the FileInfo to indicate it is a member of
            // this generation.
            gen_index = file_info.data.generations.num_items;
            file_info.data.mode.v = fe.fef->mode;
            file_info.data.size.v = fe.fef->size;
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

        processed_bytes += fe.fef->size;
        if (time( &now ) != last_progress )
        {
            UINT32 progress = processed_bytes * 1000 / total_bytes;
            if (pfkbak_verb == VERB_1)
            {
                printf("\rprogress: %3d.%d%%   bytes: %lld  written: %lld ",
                       progress/10, progress%10,
                       processed_bytes, bytes_written);
                fflush(stdout);
            }
            last_progress = now;
        }

        if (pfkbak_verb == VERB_2)
        {
            printf("%s", fe.fef->path);
            fflush(stdout);
        }

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
        file_info.data.mode.v = fe.fef->mode;
        file_info.data.size.v = fe.fef->size;
        file_info.data.generations.alloc(1);
        file_info.data.generations.array[0]->v = gen_num;

        walk_file( STATE_NEW, 
                   baknum, file_number, gen_num,
                   &file_info, fe.fef );

        file_info.put(true);

        if (pfkbak_verb == VERB_2)
            printf("\n");

        delete fe.fe;
    }
    delete fel;

    if (pfkbak_verb == VERB_1)
        printf("\rprogress: %3d.%d%%   bytes: %lld  written: %lld\n",
               100, 0,
               processed_bytes, bytes_written);

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
