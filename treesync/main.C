
#include <stdio.h>

#include <Btree.H>
#include <bst.H>

#include "FileList.H"
#include "db.H"
#include "protos.H"

int treesync_verbose = 0;

#define DELETE_LIST(list)                       \
    while ((fe = list->dequeue_head()) != NULL) \
        delete fe;                              \
    delete list

int
treesync_main( int argc, char ** argv )
{
    FileEntryList * fel1, * fel2;
    Btree * db1, * db2;
    char * dir1, * dir2;
    FileEntry * fe;

    if (argc > 1)
    {
        if (strcmp(argv[1], "-v") == 0)
        {
            treesync_verbose = 1;
            argc--;
            argv++;
        }
    }

    if (argc == 2)
    {
        dir1 = argv[1];
        db1 = open_ts_db(dir1);
        if (!db1)
            return 1;
        fel1 = generate_file_list(dir1);
        update_db(dir1, db1, fel1);
        DELETE_LIST(fel1);
        db1->get_fbi()->compact(0);
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

    db1->get_fbi()->compact(0);
    db2->get_fbi()->compact(0);

    delete db1;
    delete db2;

    return 0;
}
