
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <Btree.H>
#include <bst.H>

#include "FileList.H"
#include "db.H"
#include "protos.H"
#include "macros.H"

void
update_db( char *root_dir, Btree * db, FileEntryList * fel )
{
    union {
        FileEntry * fe;
        FileEntryFile * fef;
        FileEntryDir * fed;
    } fe;
    FileEntry * nfe = NULL;
    DbInfo  dbinf(db);
    UINT32  index;
    FileEntryQueue   workq;

    dbinf.key.info_key.set((char*)INFO_KEY);
    if (!dbinf.get())
    {
        fprintf(stderr, "failure fetching dbinfo\n");
        exit(1);
    }

    FileEntryHash   fh;

    for (fe.fe = fel->get_head(); fe.fe; fe.fe = nfe)
    {
        nfe = fel->get_next(fe.fe);
        if ((fe.fe->type == FileEntry::TYPE_FILE)   &&
            (strcmp(fe.fe->path, TREESYNC_DB) != 0))
        {
            fh.add(fe.fe);
            workq.add(fe.fe);
        }
        else
        {
            fel->remove(fe.fe);
            delete fe.fe;
        }
    }

    FileSeq  fiseq(db);

    // first identify the files deleted, modified, and unchanged.

    for (index=0; index < dbinf.data.num_files.v; index++)
    {
        fiseq.key.index.v = index;
        if (!fiseq.get())
        {
            fprintf(stderr, "error fetching fiinf %d\n", index);
            continue;
        }
        fe.fe = fh.find(fiseq.data.file_path.string);
        if (fe.fe)
        {
            workq.remove(fe.fe);
            // unchanged or modified.
            if ((        fe.fef->size  != fiseq.data.size.v)   ||
                ((UINT32)fe.fef->mtime != fiseq.data.mtime.v)  )
            {
                // file is modified.
                // recalculate the md5 of the file and update
                // both the list entry and the database.
                calc_md5( root_dir, fe.fe->path, fe.fef->md5 );
                memcpy(fiseq.data.md5hash.binary, fe.fef->md5, MD5_DIGEST_SIZE);
                fiseq.data.state.v = FileSeqData::STATE_EXISTS;
                fiseq.data.size.v = fe.fef->size;
                fiseq.data.mtime.v = fe.fef->mtime;
                fiseq.put(true);
            }
            else
            {
                // file is not modified.  optimize out the md5 
                // calculation by copying out of the database.
                memcpy(fe.fef->md5, fiseq.data.md5hash.binary, MD5_DIGEST_SIZE);
            }
        }
        else
        {
            // deleted file.
            // add a deleted marker to the fel so that we will know
            // to delete it from the other tree if it is found to still
            // exist there.

            fe.fef = new FileEntryFile(fiseq.data.file_path.string);
            fe.fef->state = FileEntryFile::STATE_DELETED;
            fe.fef->size = 0;
            memset(fe.fef->md5, 0, MD5_DIGEST_SIZE);

            if (fiseq.data.state.v == FileSeqData::STATE_EXISTS)
            {
                // indicate in the database that the file has been
                // deleted.  technically we don't know the moment the
                // file was deleted, so just record the deletion time
                // as right now.

                fiseq.data.state.v = FileSeqData::STATE_DELETED;
                fiseq.data.size.v = 0;
                fiseq.data.mtime.v = time(0);
                memset(fiseq.data.md5hash.binary, 0, MD5_DIGEST_SIZE);
                fiseq.put(true);
            }

            // extract the time of deletion from the database.
            fe.fef->mtime = fiseq.data.mtime.v;

            fel->add(fe.fe);
        }
    }

    // now identify the files created: whatever is left on workq
    // after the above, is a new file.

    while ((fe.fe = workq.dequeue_head()) != NULL)
    {
        calc_md5(root_dir, fe.fe->path, fe.fef->md5);

        // add a new entry to the database for this item.
        fiseq.key.index.v = dbinf.data.num_files.v;
        dbinf.data.num_files.v++;
        fiseq.data.state.v = FileSeqData::STATE_EXISTS;
        fiseq.data.file_path.set(fe.fe->path);
        fiseq.data.size.v = fe.fef->size;
        fiseq.data.mtime.v = fe.fef->mtime;
        memcpy(fiseq.data.md5hash.binary, fe.fef->md5, MD5_DIGEST_SIZE);
        if (!fiseq.put())
            fprintf(stderr, "error putting created entry\n");
    }

    dbinf.put(true);

    // time to clean up the mess.

    for (fe.fe = fel->get_head(); fe.fe; fe.fe = fel->get_next(fe.fe))
        if (fh.onthislist(fe.fe))
            fh.remove(fe.fe);
}
