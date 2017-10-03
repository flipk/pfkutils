
#include "FileBlockLocal.H"
#include "FileBlockLocal_internal.H"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

FileBlockLocalCache :: FileBlockLocalCache( int _fd, int _max_size )
{
    total_size = 0;
    max_size = _max_size;
    fd = _fd;
}

FileBlockLocalCache :: ~FileBlockLocalCache( void )
{
    FBLB * b;
    flush();
    while ((b = list.get_head()) != NULL)
    {
        remove(b);
        delete b;
    }
}

void
FileBlockLocalCache :: remove( FBLB * b )
{
    if (!b->valid())
    {
        fprintf(stderr, "invalid block!\n");
        exit( 1 );
    }
    if (!lru.onthislist(b))
    {
        fprintf(stderr, "ERROR: removing locked block!\n");
        exit( 1 );
    }
    if (b->dirty)
    {
        lseek(fd, b->offset, SEEK_SET);
        write(fd, b->ptr, b->size);
    }
    list.remove(b);
    lru.remove(b);
    hash.remove(b);
    total_size -= b->size;
}

FBLB *
FileBlockLocalCache :: find( UINT32 block )
{
    FBLB * ret;
    ret = hash.find(block);
    if (!ret)
        return NULL;
    if (!lru.onthislist(ret))
    {
        fprintf(stderr, "block %d is already locked!\n", block);
        exit(1);
    }
    lru.remove(ret);
    return ret;
}

FBLB *
FileBlockLocalCache :: get_for_write( UINT32 block, off_t offset, int size )
{
    FBLB * ret;
#if 1 /* REMOVE TO OPTIMIZE */
    ret = find( block );
    if (ret)
    {
        fprintf(stderr, "ERROR: get_for_write block %d: already in cache!\n",
                block);
        exit( 1 );
    }
#endif
    ret = new FBLB(block, offset, size);
    memset(ret->ptr, 0, size);
    list.add(ret);
    hash.add(ret);
    total_size += ret->size;
    return ret;
}

FBLB *
FileBlockLocalCache :: get( UINT32 block, off_t offset, int size )
{
    FBLB * ret;
#if 1 /* REMOVE TO OPTIMIZE */
    ret = find( block );
    if (ret)
    {
        fprintf(stderr, "ERROR: get_for_write block %d: already in cache!\n",
                block);
        exit( 1 );
    }
#endif
    ret = new FBLB(block, offset, size);
    lseek(fd, offset, SEEK_SET);
    read(fd, ret->ptr, size);
    list.add(ret);
    hash.add(ret);
    total_size += ret->size;
    return ret;
}

void
FileBlockLocalCache :: unlock( FBLB * b, bool dirty )
{
    if (!b->valid())
    {
        fprintf(stderr, "bogus cookie!\n");
        exit( 1 );
    }
    if (lru.onthislist(b))
    {
        fprintf(stderr, "block is already unlocked!\n");
        exit( 1 );
    }
    lru.add(b);
    if (dirty)
        b->dirty = true;
    trim();
}

void
FileBlockLocalCache :: trim( void )
{
    while (total_size > max_size)
    {
        FBLB * b = lru.get_oldest();
        remove(b);
    }
}

static int
block_compare( const void * _a, const void * _b )
{
    FBLB * a = *(FBLB **)_a;
    FBLB * b = *(FBLB **)_b;
    if (a->block > b->block)
        return 1;
    if (a->block < b->block)
        return -1;
    return 0;
}

void
FileBlockLocalCache :: flush( void )
{
    FBLB * b;
    int i, count;

    count = list.get_cnt();
    FBLB * blocks[count];
    i = 0;
    for (b = list.get_head(); b; b = list.get_next(b))
        if (b->dirty)
            blocks[i++] = b;
    count = i;

    qsort( blocks, count, sizeof(FBLB*),
           (int(*)(const void *, const void *))block_compare);

    for (i = 0; i < count; i++)
    {
        b = blocks[i];
        lseek(fd, b->offset, SEEK_SET);
        write(fd, b->ptr, b->size);
        b->dirty = false;
    }
}
