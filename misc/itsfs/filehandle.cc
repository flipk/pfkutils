
#include <stdio.h>
#include <string.h>

#include "filehandle.H"
#include "config.h"

bool
FileHandle :: decode( encrypt_iface * crypt, nfs_fh *buffer )
{
#ifdef USE_CRYPT
    if ( crypt )
        crypt->decrypt( (UCHAR*)this, buffer->data, FH_SIZE );
    else
#endif
        memcpy( (UCHAR*)this, buffer->data, FH_SIZE );
    return valid();
}

void
FileHandle :: encode( encrypt_iface * crypt, nfs_fh *buffer )
{
    magic.set( MAGIC );
    checksum.set( calc_checksum() );
#ifdef USE_CRYPT
    if ( crypt )
        crypt->encrypt( buffer->data, (UCHAR*)this, FH_SIZE );
    else
#endif
        memcpy( buffer->data, (UCHAR*)this, FH_SIZE );
}

UINT32
FileHandle :: calc_checksum( void )
{
    UINT32 old_checksum, sum;
    int count;
    uchar * ptr;

    old_checksum = checksum.get();
    checksum.set( 0 );

    sum = 0;
    ptr = (uchar*)this;

    for ( count = 0; count < FH_SIZE; count++ )
    {
        sum += *ptr++;
    }

    checksum.set( old_checksum );

    return sum + SUM_CONSTANT;
}
