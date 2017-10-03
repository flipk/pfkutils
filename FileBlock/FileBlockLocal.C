
#include "FileBlockLocal.H"
#include "FileBlockLocal_internal.H"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

FileBlockLocal :: FileBlockLocal( char * fname, bool create )
{
    if (create)
        fd = open( fname, O_RDWR | O_CREAT, 0644 );
    else
        fd = open( fname, O_RDWR );

    if ( fd < 0 )
    {
        fprintf(stderr, "unable to open file '%s': %s\n",
                fname, strerror( errno ));
        exit( 1 );
    }

    cache = new FileBlockLocalCache(fd, 15000000);
}

//virtual
FileBlockLocal :: ~FileBlockLocal( void )
{
    // xxx flush cache
    delete cache;
    close(fd);
}

//virtual
UINT32
FileBlockLocal :: alloc( int size )
{
    

    return 0;
}

//virtual
void
FileBlockLocal :: free( UINT32 block )
{
}

//virtual
UINT32
FileBlockLocal :: get_data_info_block( char *info_name )
{
    return 0;
}

//virtual
void
FileBlockLocal :: set_data_info_block( UINT32 block, char *info_name )
{
}

//virtual
void
FileBlockLocal :: del_data_info_block( char * info_name )
{
}

//virtual
UCHAR *
FileBlockLocal :: _get_block( UINT32 block, int *size,
                              FileBlockCookie **cookie, bool for_write )
{
    return NULL;
}

//virtual
void
FileBlockLocal :: unlock_block( FileBlockCookie *cookie )
{
}

//virtual
void
FileBlockLocal :: flush(void)
{
}
