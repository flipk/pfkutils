
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

#include <Btree.H>
#include <bst.H>

#include "FileList.H"
#include "db.H"
#include "protos.H"
#include "macros.H"

static void
make_trash_dir(char *root)
{
    char trashpath[512];
    snprintf(trashpath, 511, "%s/" TRASH_DIR, root);
    trashpath[511] = 0;
    (void) mkdir( trashpath, 0755 );
}

void
treesync_analyze( char * dir1, TSFileEntryList * fel1,
                  char * dir2, TSFileEntryList * fel2 )
{
    TSFileEntryHash fh1;
    TSFileEntryHash fh2;
    TSFileEntryQueue q1;  // work queue for 1
    TSFileEntryQueue q2;  // work queue for 2
    TSFileEntryQueue c12; // copy 1 to 2
    TSFileEntryQueue c21; // copy 2 to 1
    TSFileEntryQueue d1;  // delete from 1
    TSFileEntryQueue d2;  // delete from 2
    union {
        TSFileEntry * fe;
        TSFileEntryFile * fef;
    } u1;
    union {
        TSFileEntry * fe;
        TSFileEntryFile * fef;
    } u2;

    for (u1.fe = fel1->get_head(); u1.fe; u1.fe = fel1->get_next(u1.fe))
    {
        fh1.add(u1.fe);
        q1.add(u1.fe);
    }
    for (u2.fe = fel2->get_head(); u2.fe; u2.fe = fel2->get_next(u2.fe))
    {
        fh2.add(u2.fe);
        // add to work queue and strip them off as matches are found.
        q2.add(u2.fe);
    }

    while ((u1.fe = q1.dequeue_head()) != NULL)
    {
        // for each entry in dir 1, try to find it in fh2.
        u2.fe = fh2.find(u1.fe->path);
        if (u2.fe)
        {
            q2.remove(u2.fe);

            // determine if something must be deleted from dir 1 or dir 2
            // (added to d1 or d2)

            if (u1.fef->state == TSFileEntryFile::STATE_DELETED)
            {
                if (u2.fef->state == TSFileEntryFile::STATE_EXISTS)
                {
                    // exists in 2 but not in 1, do we delete from 2
                    // or copy to 1?
                    if (u1.fef->mtime > u2.fef->mtime)
                    {
                        // the deletion date in 1 is later than the last
                        // modification date in 2, so delete from 2.
                        d2.add(u2.fe);
                    }
                    else
                    {
                        // the modification date of 2 is later than the
                        // deletion date of 1, so copy the file to 1.
                        c21.add(u2.fe);
                    }
                }
            }
            else if (u2.fef->state == TSFileEntryFile::STATE_DELETED)
            {
                if (u1.fef->state == TSFileEntryFile::STATE_EXISTS)
                {
                    // exists in 1 but not in 2, do we delete from 1
                    // or copy to 2?
                    if (u1.fef->mtime > u2.fef->mtime)
                    {
                        // the modification date of 1 is later than the
                        // deletion date in 2, so copy to 2.
                        c12.add(u1.fe);
                    }
                    else
                    {
                        // the deletion date of 2 is later than the
                        // modification date of 1, so delete from 1.
                        d1.add(u1.fe);
                    }
                }
            }
            else if (memcmp(u1.fef->md5, u2.fef->md5, MD5_DIGEST_SIZE) != 0)
            {
                // the file has been modified. determine which
                // way to copy.
                if (u1.fef->mtime > u2.fef->mtime)
                {
                    // dir 1 is the newer copy.
                    c12.add(u1.fe);
                }
                else
                {
                    // dir 2 is the newer copy.
                    c21.add(u2.fe);
                }
            }
        }
        else
        {
            // the file exists in dir 1 but not dir 2.
            c12.add(u1.fe);
        }
    }

    // whatever has not been stripped off q2 by the above loop,
    // is a file in 2 but not in 1, which needs to be copied.
    while ((u2.fe = q2.dequeue_head()) != NULL)
        c21.add(u2.fe);

    // clean up the hashes, not needed anymore.
    for (u1.fe = fel1->get_head(); u1.fe; u1.fe = fel1->get_next(u1.fe))
        fh1.remove(u1.fe);
    for (u2.fe = fel2->get_head(); u2.fe; u2.fe = fel2->get_next(u2.fe))
        fh2.remove(u2.fe);

    // what's left on c12 and c21 are the files which need to be
    // copied.

    while ((u1.fe = c12.dequeue_head()) != NULL)
        if (u1.fef->mtime != 0)
            treesync_copy_file(dir1, u1.fe->path, dir2, u1.fe->path);

    while ((u2.fe = c21.dequeue_head()) != NULL)
        if (u2.fef->mtime != 0)
            treesync_copy_file(dir2, u2.fe->path, dir1, u2.fe->path);

    if (d1.get_cnt() > 0)
        make_trash_dir(dir1);
    if (d2.get_cnt() > 0)
        make_trash_dir(dir2);

    while ((u1.fe = d1.dequeue_head()) != NULL)
        treesync_delete_file(dir1, u1.fe->path);

    while ((u2.fe = d2.dequeue_head()) != NULL)
        treesync_delete_file(dir2, u2.fe->path);
}
