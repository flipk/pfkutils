
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

enum FileEntryLists {
    FILE_ENTRY_LIST,
    FILE_ENTRY_QUEUE,
    FILE_ENTRY_HASH,
    FILE_ENTRY_NUM_LISTS
};

class FileEntry {
public:
    FileEntry(const char * _path) {
        int plen = strlen(_path);
        path = new char[plen + 1];
        memcpy(path, _path, plen+1);
        deleted = false;
        modified = false;
    }
    virtual ~FileEntry(void) {
        if (path)
            delete[] path;
    }
    LListLinks <FileEntry>  links[ FILE_ENTRY_NUM_LISTS ];
    enum { TYPE_FILE, TYPE_DIR, TYPE_LINK } type;
    char * path; // relative to root dir
    bool deleted;
    bool modified;
};

class FileEntryHashComparator {
public:
    static int hash_key( FileEntry * item ) {
        return hash_key(item->path);
    }
    static int hash_key( char * path ) {
        // thank you Daniel Julius Bernstein.
        int hash = 5381;
        int c;
        while ((c = *path++) != 0)
            hash = ((hash << 5) + hash) + c;
        return hash;
    }
    static bool hash_key_compare( FileEntry * item, char *path ) {
        if (strcmp(item->path, path) == 0)
            return true;
        return false;
    }
};

typedef LList <FileEntry,FILE_ENTRY_LIST> FileEntryList;
typedef LListHash <FileEntry, char*, 
                   FileEntryHashComparator,
                   FILE_ENTRY_HASH> FileEntryHash;
typedef LList <FileEntry,FILE_ENTRY_QUEUE> FileEntryQueue;

class FileEntryFile : public FileEntry {
public:
    FileEntryFile(const char * _path)
        : FileEntry(_path)
    {
        type = TYPE_FILE;
        deleted = false;
        created = false;
        modified = false;
    }
    UINT64  size;
    time_t  mtime;    // 0 means 'whitespace'
    bool    deleted;
    bool    created;
    bool    modified;
    UINT8   md5[16];
};

class FileEntryDir : public FileEntry {
public:
    FileEntryDir(const char * _path)
        : FileEntry(_path)
    {
        type = TYPE_DIR;
    }
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

FileEntryList * generate_file_list(const char *root_dir);
