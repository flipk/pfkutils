#include <Btree.H>
#include <bst.H>

#include "FileList.H"
#include "db.H"
#include "protos.H"

void
analyze( char * dir1, FileEntryList * fel1,
         char * dir2, FileEntryList * fel2 )
{
    FileEntryHash fh1, fh2;
    FileEntryQueue c12; // copy 1 to 2
    FileEntryQueue c21; // copy 2 to 1
    union {
        FileEntry * fe;
        FileEntryFile * fef;
    } u1;
    union {
        FileEntry * fe;
        FileEntryFile * fef;
    } u2;

    for (u1.fe = fel1->get_head(); u1.fe; u1.fe = fel1->get_next(u1.fe))
        fh1.add(u1.fe);
    for (u2.fe = fel2->get_head(); u2.fe; u2.fe = fel2->get_next(u2.fe))
    {
        fh2.add(u2.fe);
        // add to work queue and strip them off as matches are found.
        c21.add(u2.fe);
    }

    for (u1.fe = fel1->get_head(); u1.fe; u1.fe = fel1->get_next(u1.fe))
    {
        // for each entry in dir 1, try to find it in fh2.
        u2.fe = fh2.find(u1.fe->path);
        if (u2.fe)
        {
            if (memcmp(u1.fef->md5, u2.fef->md5, 16) != 0)
            {
//xxx
//                printf("analyze:\n");
//                display_md5(u1.fe->path, u1.fef->md5);
//                display_md5(u2.fe->path, u2.fef->md5);
                // the file has been modified. determine which
                // way to copy.
                if (u1.fef->mtime > u2.fef->mtime)
                {
                    // dir 1 is the newer copy.
                    c21.remove(u2.fe);
                    c12.add(u1.fe);
                }
                // else, leave it in c21 because dir2 is the newer copy.
            }
            else
            {
                // the file is not modified at all. strip from c21.
                c21.remove(u2.fe);
            }
        }
        else
        {
            // the file exists in dir 1 but not dir 2.
            c12.add(u1.fe);
        }
    }

    // clean up the hashes, not needed anymore.
    for (u1.fe = fel1->get_head(); u1.fe; u1.fe = fel1->get_next(u1.fe))
        fh1.remove(u1.fe);
    for (u2.fe = fel2->get_head(); u2.fe; u2.fe = fel2->get_next(u2.fe))
        fh2.remove(u2.fe);

    // what's left on c12 and c21 are the files which need to be
    // copied.

    while ((u1.fe = c12.dequeue_head()) != NULL)
        copy_file(dir1, u1.fe->path, dir2, u1.fe->path);

    while ((u2.fe = c21.dequeue_head()) != NULL)
        copy_file(dir2, u2.fe->path, dir1, u2.fe->path);
}
