
#include <stdio.h>
#include <stdlib.h>

#include <Btree.H>
#include <bst.H>

#include "FileList.H"
#include "db.H"
#include "protos.H"

void
update_db( char *root_dir, Btree * db, FileEntryList * fel )
{
    union {
        FileEntry * fe;
        FileEntryFile * fef;
        FileEntryDir * fed;
    } fe;
    FileEntry * nfe;
    DbInfo  dbinf(db);
    UINT32  index;

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
        }
        else
        {
            fel->remove(fe.fe);
            delete fe.fe;
        }
    }

    FileEntryList   files_processed;

    FileSeq  fiseq(db);
    int i;

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
            fel->remove(fe.fe);
            // unchanged or modified.
            if ((fe.fef->size != fiseq.data.size.v)   ||
                (fe.fef->mtime != fiseq.data.mtime.v)  )
            {
//                printf("modified: %s/%s\n", root_dir, fe.fe->path);
                // file is modified.
                // recalculate the md5 of the file and update
                // both the list entry and the database.
                calc_md5( root_dir, fe.fe->path, fe.fef->md5 );
                for (i=0; i < 16; i++)
                    fiseq.data.md5hash.array[i].v = fe.fef->md5[i];
                fiseq.data.size.v = fe.fef->size;
                fiseq.data.mtime.v = fe.fef->mtime;
                fiseq.put(true);
                files_processed.add(fe.fe);
            }
            else
            {
                // file is not modified.  optimize out the md5 
                // calculation by copying out of the database.
                for (i=0; i < 16; i++)
                    fe.fef->md5[i] = fiseq.data.md5hash.array[i].v;
                files_processed.add(fe.fe);
            }
        }
        else
        {
            // deleted file.
//            printf("deleted: %s/%s\n", root_dir, fiseq.data.file_path.string);

            // delete it from the database as well.
            fiseq.del();

            // if we are looking at the last item, 
            // just delete it. else pull last item down to this slot.
            UINT32 last_ind = dbinf.data.num_files.v-1;
            if (index != last_ind)
            {
                fiseq.key.index.v = last_ind;
                if (!fiseq.get())
                    fprintf(stderr, "error fetching last\n");
                fiseq.del();
                fiseq.key.index.v = index;
                fiseq.put(true);
            }
            // decrement the total number of files in the database.
            dbinf.data.num_files.v--;
            // force the loop to look at this same index number
            // again so that we don't end up skipping the one
            // that we just pulled down.
            index--;
        }
    }

    // now identify the files created: whatever is left on fel
    // after the above, is a new file.

    while ((fe.fe = fel->dequeue_head()) != NULL)
    {
//        printf("created: %s/%s\n", root_dir, fe.fe->path);
        calc_md5(root_dir, fe.fe->path, fe.fef->md5);
        files_processed.add(fe.fe);

        // add a new entry to the database for this item.
        fiseq.key.index.v = dbinf.data.num_files.v;
        dbinf.data.num_files.v++;
        fiseq.data.file_path.set(fe.fe->path);
        fiseq.data.size.v = fe.fef->size;
        fiseq.data.mtime.v = fe.fef->mtime;
        for (i=0; i < 16; i++)
            fiseq.data.md5hash.array[i].v = fe.fef->md5[i];
        if (!fiseq.put())
            fprintf(stderr, "error putting created entry\n");
    }

    dbinf.put(true);

    // time to clean up the mess.

    while ((fe.fe = files_processed.dequeue_head()) != NULL)
    {
        fh.remove(fe.fe);
        fel->add(fe.fe);
    }
}
