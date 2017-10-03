
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <pk-md5.h>
#include <Btree.H>
#include <bst.H>

#include "FileList.H"
#include "protos.H"

void
treesync_display_md5(char *path, UINT8 *md5)
{
    int i;
    for (i=0; i < MD5_DIGEST_SIZE; i++)
        printf("%02x", md5[i]);
    printf("  %s\n", path);
}

void
treesync_calc_md5( char *root_dir, char *relpath, UINT8 * hashbuffer )
{
    FILE         * f;
    MD5_CTX        ctx;
    MD5_DIGEST     digest;
    unsigned char  inbuf[8192];
    unsigned int   len;
    char           fullpath[512];

    snprintf(fullpath, sizeof(fullpath), "%s/%s", root_dir, relpath);
    fullpath[511]=0;

    if (treesync_verbose)
        fprintf(stderr, "md5 %s = ", fullpath);

    f = fopen(fullpath,"r");
    if (!f )
    {
        fprintf(stderr, "unable to calc md5 hash on %s\n", relpath);
        memset(hashbuffer, 0, MD5_DIGEST_SIZE);
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

    memcpy(hashbuffer, digest.digest, MD5_DIGEST_SIZE);

    if (treesync_verbose)
    {
        int i;
        for (i=0; i < MD5_DIGEST_SIZE; i++)
            fprintf(stderr,"%02x", digest.digest[i]);
        fprintf(stderr, "\n");
    }
}
