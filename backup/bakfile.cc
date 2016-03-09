/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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
    bt = Btree::openFile(opts.backupfile_index.c_str(),
                         CACHE_SIZE_INDEX);
    if (bt == NULL)
    {
        cerr << "unable to open btree database\n";
        return false;
    }
    fbi = bt->get_fbi();

    fbi_data = FileBlockInterface::openFile(
        opts.backupfile_data.c_str(),  CACHE_SIZE_DATA);
    if (fbi_data == NULL)
    {
        cerr << "unable to open btree database\n";
        return false;
    }

    return true;
}

bool
bakFile::createFiles(void)
{
    bt = Btree::createFile(
        opts.backupfile_index.c_str(),
        CACHE_SIZE_INDEX,
        /*file mode*/ 0600, BTREE_ORDER);
    if (bt == NULL)
    {
        cerr << "unable to create database index\n";
        return false;
    }

    fbi_data = FileBlockInterface::createFile(
        opts.backupfile_data.c_str(),
        CACHE_SIZE_DATA,
        /*file mode*/ 0600);
    if (fbi_data == NULL)
    {
        cerr << "unable to create database\n";
        delete bt;
        bt = NULL;
        return false;
    }

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
    if (fbi_data)
    {
        fbi_data->compact(&compactionStatus, NULL);
        delete fbi_data;
    }
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
