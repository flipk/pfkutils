
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
#include <string>

#include <pk-md5.h>

#include "types.H"
#include "dll2.H"

enum TsFileEntryLists {
    FILE_ENTRY_LIST,
    FILE_ENTRY_QUEUE,
    FILE_ENTRY_HASH,
    FILE_ENTRY_NUM_LISTS
};

class TSFileEntry {
public:
    TSFileEntry(const std::string &_path)
        : path(_path) {}
    virtual ~TSFileEntry(void) {}
    LListLinks <TSFileEntry>  links[ FILE_ENTRY_NUM_LISTS ];
    enum { TYPE_FILE, TYPE_DIR, TYPE_LINK } type;
    const std::string path;  // relative to root dir
};

class TSFileEntryHashComparator {
public:
    static int hash_key( TSFileEntry * item ) {
        return hash_key(item->path);
    }
    static int hash_key( const std::string &path ) {
        // thank you Daniel Julius Bernstein.
        int hash = 5381;
        std::string::const_iterator it;
        int c;
        for (it = path.begin(); it != path.end(); it++)
        {
            c = *it;
            hash = ((hash << 5) + hash) + c;
        }
        return hash;
    }
    static bool hash_key_compare( const TSFileEntry * item,
                                  const std::string &path ) {
        return (item->path == path);
    }
};

typedef LList <TSFileEntry,FILE_ENTRY_LIST> TSFileEntryList;
typedef LListHash <TSFileEntry, const std::string, 
                   TSFileEntryHashComparator,
                   FILE_ENTRY_HASH> TSFileEntryHash;
typedef LList <TSFileEntry,FILE_ENTRY_QUEUE> TSFileEntryQueue;

class TSFileEntryFile : public TSFileEntry {
public:
    TSFileEntryFile(const std::string &_path)
        : TSFileEntry(_path)
    {
        type = TYPE_FILE;
        state = STATE_EXISTS;
    }
    enum State { STATE_EXISTS=1, STATE_DELETED=2 } state;
    UINT64  size;
    time_t  mtime;
    UINT16  mode;
    UINT8   md5[MD5_DIGEST_SIZE];
};

class TSFileEntryDir : public TSFileEntry {
public:
    TSFileEntryDir(const std::string &_path)
        : TSFileEntry(_path)
    {
        type = TYPE_DIR;
    }
    UINT16  mode;
};

class TSFileEntryLink : public TSFileEntry {
public:
    TSFileEntryLink(const std::string &_path) 
        : TSFileEntry(_path)
    {
        type = TYPE_LINK;
    }
    void set_target(const std::string &_target) {
        link_target = _target;
    }
    ~TSFileEntryLink(void) {}
    std::string link_target;
};

TSFileEntryList * treesync_generate_file_list(const std::string &root_dir);
