
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

#if _FILE_OFFSET_BITS != 64
#error you forgot -D_FILE_OFFSET_BITS=64
#endif

static int compare_iterations;

static int
qsortFileEntryCompare(const void * _a, const void * _b)
{
    FileEntry * a = *(FileEntry**)_a;
    FileEntry * b = *(FileEntry**)_b;

    compare_iterations++;

    return -strcmp(a->path, b->path);
}

void
generate_file_list(const char *root_dir,
                   FileEntryList * file_list,
                   FileEntryList * dir_list)
{
    FileEntryList   work_list;
    FileEntry     * current;
    struct dirent * de;
    DIR           * dir;
    struct stat     sb;
    char            temp_path[1024];
    char            rel_path[1024];
    int             cc;

    current = new FileEntryDir(".");
    work_list.add(current);

    while ((current = work_list.dequeue_head()) != NULL)
    {
        cc = snprintf(temp_path, sizeof(temp_path)-1,
                      "%s/%s", root_dir, current->path);
        temp_path[cc] = 0;

        dir = opendir(temp_path);
        if (!dir)
        {
            fprintf(stderr, "unable to open directory: %s: %s\n",
                    temp_path, strerror(errno));
            delete current;
            continue;
        }

        if (stat(temp_path, &sb) < 0)
        {
            fprintf(stderr, "unable to stat %s: %s\n",
                    temp_path, strerror(errno));
            closedir(dir);
            delete current;
            continue;
        }

        ((FileEntryDir*)current)->mode = sb.st_mode & 0777;
        dir_list->add(current);

        while ((de = readdir(dir)) != NULL)
        {
            if (strcmp(de->d_name,".") == 0 ||
                strcmp(de->d_name,"..") == 0)
            {
                continue;
            }

            cc = snprintf(temp_path, sizeof(temp_path)-1,
                          "%s/%s/%s", root_dir, current->path, de->d_name);
            temp_path[cc] = 0;
            cc = snprintf(rel_path, sizeof(rel_path)-1,
                          "%s/%s", current->path, de->d_name);
            rel_path[cc] = 0;

            if (lstat(temp_path, &sb) < 0)
            {
                fprintf(stderr, "unable to stat %s: %s\n",
                        temp_path, strerror(errno));
            }
            else if (S_ISDIR(sb.st_mode))
            {
                FileEntryDir * fed = new FileEntryDir(rel_path);
                fed->mode = sb.st_mode & 0777;
                work_list.add(fed);
            }
            else if (S_ISREG(sb.st_mode))
            {
                FileEntryFile * fef = new FileEntryFile(rel_path);
                fef->mode = sb.st_mode & 0777;
                fef->size = sb.st_size;
                fef->atime = sb.st_atime;
                file_list->add(fef);
            }
            else if (S_ISLNK(sb.st_mode))
            {
                FileEntryLink * fel = new FileEntryLink(rel_path);
                cc = readlink(temp_path, rel_path, sizeof(rel_path)-1);
                rel_path[cc] = 0;
                fel->set_target(rel_path);
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

    FileEntry ** flat =
        new FileEntry*[file_list->get_cnt() + dir_list->get_cnt()];
    int counter = 0;

    while ((current = file_list->dequeue_head()) != NULL)
        flat[counter++] = current;
    while ((current = dir_list->dequeue_head()) != NULL)
        flat[counter++] = current;

    qsort(flat, counter, sizeof(FileEntry*), &qsortFileEntryCompare);

    while (counter-- > 0)
    {
        current = flat[counter];
        if (current->type == FileEntry::TYPE_DIR)
            dir_list->add(current);
        else
            file_list->add(current);
    }
}


int
main()
{
    FileEntryList  files, dirs;
    union {
        FileEntry     * fe;
        FileEntryFile * fef;
        FileEntryDir  * fed;
        FileEntryLink * fel;
    } fe;
    UINT64  total_bytes = 0;

    compare_iterations = 0;
    generate_file_list("/home/flipk", &files, &dirs);

    fprintf(stderr, "scan found %d dirs and %d files\n",
            dirs.get_cnt(), files.get_cnt());

    fprintf(stderr, "compare iterations: %d\n", compare_iterations);

    while ((fe.fe = dirs.dequeue_head()) != NULL)
    {
        switch (fe.fe->type)
        {
        case FileEntry::TYPE_FILE:
            printf("File: %s\n"
                   "Mode: %o  Size: %lld   Atime: %d\n",
                   fe.fef->path, fe.fef->mode, fe.fef->size, (int)fe.fef->atime);
            break;
        case FileEntry::TYPE_LINK:
            printf("Link: %s\n"
                   "Target: %s\n",
                   fe.fel->path, fe.fel->link_target);
            break;
        case FileEntry::TYPE_DIR:
            printf("Dir: %s\n"
                   "Mode: %o\n", fe.fed->path, fe.fed->mode);
            break;
        }
        delete fe.fe;
    }

    while ((fe.fe = files.dequeue_head()) != NULL)
    {
        switch (fe.fe->type)
        {
        case FileEntry::TYPE_FILE:
            printf("File: %s\n"
                   "Mode: %o  Size: %lld   Atime: %d\n",
                   fe.fef->path, fe.fef->mode, fe.fef->size, (int)fe.fef->atime);
            total_bytes += fe.fef->size;
            break;
        case FileEntry::TYPE_LINK:
            printf("Link: %s\n"
                   "Target: %s\n",
                   fe.fel->path, fe.fel->link_target);
            break;
        case FileEntry::TYPE_DIR:
            printf("Dir: %s\n"
                   "Mode: %o\n", fe.fed->path, fe.fed->mode);
            break;
        }
        delete fe.fe;
    }

    fprintf(stderr, "total bytes in this backup: %lld\n", total_bytes);

    return 0;
}
