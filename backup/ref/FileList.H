
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
#include "pk-md5.h"

enum { FILE_ENTRY_LIST, NUM_FILE_ENTRY_LISTS };

class FileEntry {
public:
    FileEntry(void) { name = NULL; }
    virtual ~FileEntry(void) { if (name) delete[] name; }
    LListLinks <FileEntry>  links[ NUM_FILE_ENTRY_LISTS ];
    enum { TYPE_FILE, TYPE_DIR, TYPE_LINK } type;
    char * name; // partial only 
};

typedef LList <FileEntry,FILE_ENTRY_LIST> FileEntryList;

class FileEntryFile : public FileEntry {
public:
    FileEntryFile(void) { type = TYPE_FILE; }
    /*virtual*/ ~FileEntryFile(void) { }
    UINT64  size;
    time_t  atime;
};

class FileEntryDir : public FileEntry {
public:
    FileEntryDir(void) { type = TYPE_DIR; files = new FileEntryList; }
    /*virtual*/ ~FileEntryDir(void) {
        FileEntry * fe;
        while ((fe = files->dequeue_head()) != NULL)
            delete fe;
        delete files;
    }
    FileEntryList * files;
};

class FileEntryLink : public FileEntry {
public:
    FileEntryLink(void) { type = TYPE_LINK; target = NULL; }
    /*virtual*/ ~FileEntryLink(void) { if (target) delete[] target; }
    char * target;
};

FileEntryDir * generate_file_list( const char * root_dir );
