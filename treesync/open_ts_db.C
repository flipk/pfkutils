
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <Btree.H>
#include <bst.H>

#include "FileList.H"
#include "db.H"
#include "protos.H"
#include "macros.H"

Btree *
open_ts_db(char *dir)
{
    Btree * ret;
    char dbpath[512];

    snprintf(dbpath, 511, "%s/" TREESYNC_DB, dir);
    dbpath[511]=0;

    ret = Btree::openFile(dbpath, DB_CACHE_SIZE);

    if (!ret)
    {
        ret = Btree::createFile(dbpath, DB_CACHE_SIZE, 0644, DB_ORDER);
        if (!ret)
        {
            fprintf(stderr, "database creation failed: %s\n",
                    strerror(errno));
            return NULL;
        }
        DbInfo  dbi(ret);
        dbi.key.info_key.set((char*)INFO_KEY);
        dbi.data.num_files.v = 0;
        dbi.data.tool_version.v = TOOL_VERSION;
        dbi.put(true);
    }

    DbInfo  dbi(ret);
    dbi.key.info_key.set((char*)INFO_KEY);
    if (!dbi.get())
    {
        fprintf(stderr,"DB INFO key not found in database!!\n");
        delete ret;
        return NULL;
    }
    if (dbi.data.tool_version.v != TOOL_VERSION)
    {
        fprintf(stderr, "Tool version mismatch error!!\n");
        delete ret;
        return NULL;
    }

    return ret;
}
