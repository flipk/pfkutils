/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
 */

#include "bakfile.h"
#include <iostream>
using namespace std;

bakFile::bakFile(const bkOptions &_opts)
    : opts(_opts)
{
    bt = NULL;
}

bool
bakFile::openFiles(void)
{
#if SINGLE_FILE_BACKUP
    bt = Btree::openFile(opts.backupfile_data.c_str(),
                         CACHE_SIZE_INDEX);
#else
    bt = Btree::openFile(opts.backupfile_index.c_str(),
                         CACHE_SIZE_INDEX);
#endif
    if (bt == NULL)
    {
        cerr << "unable to open btree database\n";
        return false;
    }
    fbi = bt->get_fbi();

#if SINGLE_FILE_BACKUP
    fbi_data = fbi;
#else
    fbi_data = FileBlockInterface::openFile(
        opts.backupfile_data.c_str(),  CACHE_SIZE_DATA);
    if (fbi_data == NULL)
    {
        cerr << "unable to open btree database\n";
        return false;
    }
#endif

    return true;
}

bool
bakFile::createFiles(void)
{
#if SINGLE_FILE_BACKUP
    bt = Btree::createFile(
        opts.backupfile_data.c_str(), CACHE_SIZE_INDEX,
        /*file mode*/ 0600, BTREE_ORDER);
#else
    bt = Btree::createFile(
        opts.backupfile_index.c_str(),
        CACHE_SIZE_INDEX,
        /*file mode*/ 0600, BTREE_ORDER);
#endif
    if (bt == NULL)
    {
        cerr << "unable to create database index\n";
        return false;
    }
    fbi = bt->get_fbi();

#if SINGLE_FILE_BACKUP
    fbi_data = fbi;
#else
    fbi_data = FileBlockInterface::createFile(
        opts.backupfile_data.c_str(), CACHE_SIZE_DATA,
        /*file mode*/ 0600);
    if (fbi_data == NULL)
    {
        cerr << "unable to create database\n";
        delete bt;
        bt = NULL;
        return false;
    }
#endif

    return true;
}

static bool compactionStatus(FileBlockStats *stats, void *arg)
{
    if (stats->num_aus < 1000)
        // don't bother compacting a small file.
        return false;
    uint32_t max_free = stats->num_aus / 100;
    if (stats->free_aus <= max_free)
        return false;
    return true;
}

bakFile::~bakFile(void)
{
    if (bt)
    {
        bt->get_fbi()->compact(&compactionStatus, NULL);
        delete bt;
    }
#if SINGLE_FILE_BACKUP == 0
    if (fbi_data)
    {
        fbi_data->compact(&compactionStatus, NULL);
        delete fbi_data;
    }
#endif
}

void
bakFile :: operate(void)
{
    switch (opts.op)
    {
    case OP_CREATE:   create     ();  break;
    case OP_UPDATE:   update     ();  break;
    case OP_LIST:     listdb     ();  break;
    case OP_DELETE:   deletevers ();  break;
    case OP_EXTRACT:  extract    ();  break;
    case OP_EXPORT:   export_tar ();  break;
    default:
        ;
    }
}
