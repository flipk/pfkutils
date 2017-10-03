
#include "database_elements.H"
#include "params.H"
#include "protos.H"

#include <FileList.H>
#include <pk-md5.h>

void
pfkbak_md5_buffer( UCHAR * buf, int buflen, UCHAR * md5 )
{
    MD5_CTX ctx;
    MD5_DIGEST     digest;

    MD5Init( &ctx );
    MD5Update( &ctx, buf, buflen );
    MD5Final( &digest, &ctx );

    memcpy(md5, digest.digest, MD5_DIGEST_SIZE);
}
