/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "bakfile.h"
#include <iostream>
using namespace std;

bakFile::bakFile(const bkOptions &_opts)
    : opts(_opts)
{
    bt = NULL;
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
