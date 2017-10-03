
#include <pk-md5.h>
#include <Btree.H>
#include <bst.H>

#include "FileList.H"
#include "protos.H"

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

    if (treesync_verbose)
        fprintf(stderr, "md5 %s = ", fullpath);

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

    if (treesync_verbose)
    {
        int i;
        for (i=0; i < 16; i++)
            fprintf(stderr,"%02x", digest.digest[i]);
        fprintf(stderr, "\n");
    }
}
