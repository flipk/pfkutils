
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <Btree.H>
#include <bst.H>

#include "FileList.H"
#include "db.H"
#include "protos.H"
#include "macros.H"

void
treesync_delete_file(char *root, char *file)
{
    char from_full_path[512];
    char   to_full_path[512];
    char   to_file_name[512];
    char * p, * q;

    for (p = file, q = to_file_name; *p; p++)
    {
        if (*p != '/')
            *q++ = *p;
        else
            *q++ = '.';
    }
    *q++ = 0;

    snprintf(from_full_path, sizeof(from_full_path),
             "%s/%s", root, file);
    snprintf(  to_full_path, sizeof(  to_full_path),
             "%s/" TRASH_DIR "/%s", root, to_file_name);

    printf("delete %s\n", from_full_path);
    (void)unlink(to_full_path);
    if (rename(from_full_path, to_full_path) < 0)
        fprintf(stderr, "rename: %s\n", strerror(errno));
}
