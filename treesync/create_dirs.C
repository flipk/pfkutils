
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

int
treesync_create_dirs(char * path)
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
