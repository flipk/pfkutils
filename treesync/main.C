#if 0

opts=-O3
incs="-I../h -I../dll2 -I../util -I../FileBlock"
srcs="main.C FileList.C"
libs="../FileBlock/libFileBlock.a ../dll2/libdll2.a"
objs=""

for f in $srcs ; do
   g++ $opts $incs -c -D_FILE_OFFSET_BITS=64 $f
   objs="$objs ${f%.C}.o"
done

g++ $opts $objs $libs -o t
exit 0

       ;;

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <utime.h>

#include <Btree.H>
#include <pk-md5.h>
#include <pk-md5.c> // xxx this is temporary
#include <bst.H>

#include "FileList.H"
#include "db.H"

#define DB_CACHE_SIZE 2097152
#define DB_ORDER      25

#define TREESYNC_DB  "treesync.db"

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
        dbi.data.num_files.v = 0;
        dbi.put(true);
    }

    return ret;
}

void
display_md5(char *path, UINT8 *md5)
{
    int i;
    for (i=0; i < 16; i++)
        printf("%02x", md5[i]);
    printf("  %s\n", path);
}

void
calc_md5( char *root_dir, char *relpath, UINT8 * hashbuffer )
{
    FILE         * f;
    MD5_CTX        ctx;
    MD5_DIGEST     digest;
    unsigned char  inbuf[8192];
    unsigned int   len;
    char           fullpath[512];

    snprintf(fullpath, sizeof(fullpath), "%s/%s", root_dir, relpath);
    fullpath[511]=0;

    f = fopen(fullpath,"r");
    if (!f )
    {
        fprintf(stderr, "unable to calc md5 hash on %s\n", relpath);
        memset(hashbuffer,0,16);
        return;
    }

    MD5Init( &ctx );

    while (1)
    {
        len = fread(inbuf, 1, sizeof(inbuf), f);
        if (len == 0)
            break;
        MD5Update( &ctx, inbuf, len );
    }

    MD5Final( &digest, &ctx );
    fclose(f);

    memcpy(hashbuffer, digest.digest, 16);

//xxx
//    printf("calc_md5:\n");
//    display_md5(fullpath, hashbuffer);
}

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

int
create_dirs(char * path)
{
    // search for every slash. zero it out.
    // attempt a stat. if it exists (and is a dir) skip it.
    // if it does not exist, mkdir it. unzero it out.
    // move to next slash, until there are no more slashes.

    char * p = path;
    struct stat sb;

    if (*p == '/')
        p++;

    while (*p)
    {
        if (*p == '/')
        {
            *p = 0;
            if (stat(path, &sb) < 0)
            {
                if (errno == ENOENT)
                {
                    // we can create it and move on.
                    if (mkdir(path, 0755) < 0)
                    {
                        fprintf(stderr, "mkdir %s: %s\n",
                                path, strerror(errno));
                        *p = '/';
                        return -1;
                    }
                }
                else
                {
                    fprintf(stderr, "error stat dir %s: %s\n",
                            path, strerror(errno));
                    *p = '/';
                    return -1;
                }
            }
            *p = '/';
        }
        p++;
    }

    return 0; 
}

void
copy_file(char *fromroot, char *fromfile,
          char *toroot,   char *tofile    )
{
    char from_full_path[512];
    char   to_full_path[512];

    snprintf(from_full_path, sizeof(from_full_path),
             "%s/%s", fromroot, fromfile);
    snprintf(  to_full_path, sizeof(  to_full_path),
               "%s/%s", toroot,   tofile);

    printf("copy %s\n", fromfile);

    int fd1, fd2;

    fd1 = open(from_full_path, O_RDONLY);
    if (fd1 < 0)
    {
        fprintf(stderr, "error opening source file '%s'\n",
                from_full_path);
        return;
    }
    (void) unlink(to_full_path);
    if (create_dirs(to_full_path) < 0)
    {
        close(fd1);
        return;
    }        
    fd2 = open(to_full_path, O_CREAT | O_WRONLY, 0644);
    if (fd2 < 0)
    {
        fprintf(stderr, "error opening destination file '%s': %s\n",
                to_full_path, strerror(errno));
        close(fd1);
        return;
    }

    int cc;
    char buffer[65536];

    while (1)
    {
        cc = read(fd1, buffer, sizeof(buffer));
        if (cc <= 0)
            break;
        write(fd2, buffer, cc);
    }

    close(fd1);
    close(fd2);
}

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

#define DELETE_LIST(list)                       \
    while ((fe = list->dequeue_head()) != NULL) \
        delete fe;                              \
    delete list

int
main( int argc, char ** argv )
{
    FileEntryList * fel1, * fel2;
    Btree * db1, * db2;
    char * dir1, * dir2;
    FileEntry * fe;

    if (argc == 2)
    {
        dir1 = argv[1];
        db1 = open_ts_db(dir1);
        if (!db1)
            return 1;
        fel1 = generate_file_list(dir1);
        update_db(dir1, db1, fel1);
        DELETE_LIST(fel1);
        db1->get_fbi()->compact(true);
        delete db1;
        return 0;
    }

    if (argc != 3)
    {
        fprintf(stderr, "usage: pfktreesync dir1 dir2\n");
        return 1;
    }

    dir1 = argv[1];
    dir2 = argv[2];

    db1 = open_ts_db(dir1);
    if (!db1)
        return 1;
    db2 = open_ts_db(dir2);
    if (!db2)
    {
        delete db1;
        return 1;
    }

    fel1 = generate_file_list(dir1);
    fel2 = generate_file_list(dir2);

    if (!fel1 || !fel2)
    {
        delete db1;
        delete db2;
        return 1;
    }

    update_db( dir1, db1, fel1 );
    update_db( dir2, db2, fel2 );

    // next step: analyze fel1 and fel2, and
    // move files around to synchronize them.
    analyze( dir1, fel1, dir2, fel2 );

    DELETE_LIST(fel1);
    DELETE_LIST(fel2);

    fel1 = generate_file_list(dir1);
    fel2 = generate_file_list(dir2);

    update_db( dir1, db1, fel1 );
    update_db( dir2, db2, fel2 );

    DELETE_LIST(fel1);
    DELETE_LIST(fel2);

    db1->get_fbi()->compact(true);
    db2->get_fbi()->compact(true);

    delete db1;
    delete db2;

    return 0;
}
