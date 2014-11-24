
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

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

#include "FileList.H"

#define TRASH_DIR    "TREESYNC_TRASH"

using namespace std;

static int
qsortTSFileEntryCompare(const void * _a, const void * _b)
{
    TSFileEntry * a = *(TSFileEntry**)_a;
    TSFileEntry * b = *(TSFileEntry**)_b;

// xxx is this < or > ?
    return a->path < b->path;
}

TSFileEntryList * 
treesync_generate_file_list(const string &root_dir)
{
    TSFileEntryList * file_list;
    TSFileEntryList   work_list;
    TSFileEntry     * current;
    struct dirent * de;
    DIR           * dir;
    struct stat     sb;
    string          temp_path;
    string          _rel_path;
    string          rel_path;
    int             cc;
    bool            first_dir = true;

    file_list = new TSFileEntryList;

    current = new TSFileEntryDir(".");
    work_list.add(current);

    while ((current = work_list.dequeue_head()) != NULL)
    {
        if (current->path == TRASH_DIR)
        {
            delete current;
            continue;
        }
        temp_path = root_dir + "/" + current->path;

        dir = opendir(temp_path.c_str());
        if (!dir)
        {
            fprintf(stderr, "unable to open directory: %s: %s\n",
                    temp_path.c_str(), strerror(errno));
            delete current;
            if (first_dir)
            {
                delete file_list;
                return NULL;
            }
            // else
            continue;
        }

        first_dir = false;

        if (stat(temp_path.c_str(), &sb) < 0)
        {
            fprintf(stderr, "unable to stat %s: %s\n",
                    temp_path.c_str(), strerror(errno));
            closedir(dir);
            delete current;
            continue;
        }

        ((TSFileEntryDir*)current)->mode = sb.st_mode & 0777;
        file_list->add(current);

        while ((de = readdir(dir)) != NULL)
        {
            if (strcmp(de->d_name,".") == 0 ||
                strcmp(de->d_name,"..") == 0)
            {
                continue;
            }

            string d_name(de->d_name);

            temp_path = root_dir + "/" + current->path + "/" + d_name;
            _rel_path = current->path + "/" + d_name;
            if (_rel_path[0] == '.' && _rel_path[1] == '/')
                rel_path = _rel_path.substr(2);
            else
                rel_path = _rel_path;

            if (lstat(temp_path.c_str(), &sb) < 0)
            {
                fprintf(stderr, "unable to stat %s: %s\n",
                        temp_path.c_str(), strerror(errno));
            }
            else if (S_ISDIR(sb.st_mode))
            {
                TSFileEntryDir * fed = new TSFileEntryDir(rel_path);
                fed->mode = sb.st_mode & 0777;
                work_list.add(fed);
            }
            else if (S_ISREG(sb.st_mode))
            {
                TSFileEntryFile * fef = new TSFileEntryFile(rel_path);
                fef->mode = sb.st_mode & 0777;
                fef->size = sb.st_size;
                fef->mtime = sb.st_mtime;
                file_list->add(fef);
            }
            else if (S_ISLNK(sb.st_mode))
            {
                TSFileEntryLink * fel = new TSFileEntryLink(rel_path);
                fel->link_target.resize(4096);
                cc = readlink(temp_path.c_str(),
                              const_cast<char*>(fel->link_target.c_str()),
                              fel->link_target.length());
                fel->link_target.resize(cc);
                file_list->add(fel);
            }
            else
            {
                // something else! nothing to do.
            }
        }

        closedir(dir);
    }

    // work_list should now be empty.

    TSFileEntry ** flat = new TSFileEntry*[file_list->get_cnt()];
    int counter = 0;

    while ((current = file_list->dequeue_head()) != NULL)
        flat[counter++] = current;

    qsort(flat, counter, sizeof(TSFileEntry*), &qsortTSFileEntryCompare);

    while (counter-- > 0)
    {
        current = flat[counter];
        file_list->add(current);
    }

    return file_list;
}
