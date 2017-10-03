
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "types.H"
#include "dll2.H"

class FileEntry {
public:
    FileEntry(const char * _path)
    {
        int plen = strlen(_path);
        path = new char[plen + 1];
        memcpy(path, _path, plen+1);
    }
    virtual ~FileEntry(void)
    {
        if (path) delete[] path;
    }
    LListLinks <FileEntry>  links[ 1 ];
    enum { TYPE_FILE, TYPE_DIR, TYPE_LINK } type;
    char * path; // relative to root dir
};

typedef LList <FileEntry,0> FileEntryList;

class FileEntryFile : public FileEntry {
public:
    FileEntryFile(const char * _path)
        : FileEntry(_path)
    {
        type = TYPE_FILE;
    }
    UINT16  mode; // as in chmod-kind of mode
    UINT64  size;
    time_t  atime;
};

class FileEntryDir : public FileEntry {
public:
    FileEntryDir(const char * _path)
        : FileEntry(_path)
    {
        type = TYPE_DIR;
    }
    UINT16  mode; // as in chmod-kind of mode
};

class FileEntryLink : public FileEntry {
public:
    FileEntryLink(const char * _path) 
        : FileEntry(_path)
    {
        link_target = NULL;
        type = TYPE_LINK;
    }
    void set_target(const char * _target) {
        int tlen = strlen(_target);
        link_target = new char[tlen+1];
        memcpy(link_target, _target, tlen+1);
    }
    ~FileEntryLink(void) { if (link_target) delete[] link_target; }
    char * link_target;
};

void generate_file_list(const char *root_dir);
