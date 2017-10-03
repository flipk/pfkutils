
/*
    This file is part of the "pkutils" tools written by Phil Knaack
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

static void
process_directory( FileEntryDir * dirlist, const char * dirpath )
{
    struct dirent * de;
    DIR * dir = opendir(dirpath);
    if (!dir)
    {
        fprintf(stderr, "unable to open directory %s: %s\n",
                dirpath, strerror(errno));
        return;
    }

    while ((de = readdir(dir)) != NULL)
    {
        if (strcmp(de->d_name,".") == 0 ||
            strcmp(de->d_name,"..") == 0)
        {
            continue;
        }
        char fullpath[ strlen(de->d_name) +
                       strlen(dirpath) + 2 ];
        struct stat sb;
        sprintf(fullpath,"%s/%s", dirpath, de->d_name);
        if (lstat(fullpath, &sb) < 0)
        {
            fprintf(stderr, "unable to stat %s: %s\n",
                    fullpath, strerror(errno));
        }
        else if (S_ISDIR(sb.st_mode))
        {
            FileEntryDir * newdir = new FileEntryDir;
            newdir->name = strdup(de->d_name);
            dirlist->files->add(newdir);
            process_directory(newdir, fullpath);
        }
        else if (S_ISREG(sb.st_mode))
        {
            FileEntryFile * fe = new FileEntryFile;
            fe->name = strdup(de->d_name);
            fe->size = sb.st_size;
            fe->atime = sb.st_atime;
            dirlist->files->add(fe);
        }
#if defined(S_ISLNK)
        else if (S_ISLNK(sb.st_mode))
        {
            FileEntryLink * fel = new FileEntryLink;
            char buf[1024];
            int cc = readlink(fullpath,buf,sizeof(buf));
            buf[cc] = 0;
            fel->name = strdup(de->d_name);
            fel->target = strdup(buf);
            dirlist->files->add(fel);
        }
#endif
        else
        {
            // ?? somthing else.
        }
    }

    closedir(dir);
}

FileEntryDir *
generate_file_list( const char * root_dir )
{
    FileEntryDir * dirlist = new FileEntryDir;
    dirlist->name = strdup(".");
    process_directory(dirlist, root_dir);
    return dirlist;
}

void
print_dir( FileEntryDir * dirlist, const char * dirpath )
{
    FileEntry * fe;
    for (fe = dirlist->files->get_head();
         fe;
         fe = dirlist->files->get_next(fe))
    {
        switch (fe->type)
        {
        case FileEntry::TYPE_FILE:
        {
            FileEntryFile * fef = (FileEntryFile *)fe;
            printf("File: %s/%s\n", dirpath, fef->name);
            break;
        }
        case FileEntry::TYPE_DIR:
        {
            FileEntryDir * fed = (FileEntryDir *)fe;
            char newdirpath[ strlen(dirpath) + strlen(fed->name) + 2 ];
            sprintf(newdirpath, "%s/%s", dirpath, fed->name);
            print_dir(fed, newdirpath);
            break;
        }
        case FileEntry::TYPE_LINK:
            FileEntryLink * fel = (FileEntryLink *)fe;
            printf("Link: %s/%s -> %s\n", dirpath, fel->name, fel->target);
            break;
        }
    }
}

int
main()
{
    FileEntryDir * fed;

    fed = generate_file_list( "/home/flipk/proj" );
    print_dir(fed, fed->name);
    delete fed;

    return 0;
}
