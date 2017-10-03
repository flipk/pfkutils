
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "dll2.H"

enum {
    BC_HASH,
    BC_CLEAN_DIRTY_LIST,
    BC_LRU,
    NUM_LISTS
};

#define BLOCK_SIZE 512

struct block {
    LListLinks <block>  links[ NUM_LISTS ];

    int id;
    char buf[ BLOCK_SIZE ];

    int read( int fd ) {
        ::lseek( fd, id * BLOCK_SIZE, SEEK_SET );
        ::read( fd, buf, BLOCK_SIZE );
    }
    int write( int fd ) {
        ::lseek( fd, id * BLOCK_SIZE, SEEK_SET );
        ::write( fd, buf, BLOCK_SIZE );
    }
};

class block_hash_1 {
public:
    static int hash_key( block * b ) {
        return hash_key( b->id );
    }
    static int hash_key( int id ) {
        return id;
    }
    static bool hash_key_compare( block * b, int id ) {
        return ( b->id == id );
    }
};

typedef LListHash < block, int, block_hash_1, BC_HASH > block_hash;
typedef LList     < block,        BC_CLEAN_DIRTY_LIST > block_cd_list;
typedef LListLRU  < block,                     BC_LRU > block_lru;

class block_cache {
    block_hash     hash;
    block_cd_list  clean;
    block_cd_list  dirty;
    block_lru      lru;
    int max_blocks;
    int fd;
public:
    block_cache( int _fd, int _max_blocks, int _hash_size )
        : hash( _hash_size ) {
        fd = _fd; max_blocks = _max_blocks;
    }
    ~block_cache( void ) {
        block * b;
        while ( b = dirty.dequeue_head() )
        {
            b->write( fd );
            hash.remove( b );
            if ( lru.onthislist( b ))
                lru.remove( b );
            delete b;
        }
        while ( b = clean.dequeue_head() )
        {
            hash.remove( b );
            if ( lru.onthislist( b ))
                lru.remove( b );
        }
    }
    block * get( int id ) {
        block * b = hash.find( id );
        if ( !b )
        {
            b = new block;
            b->id = id;
            b->read( fd );
            hash.add( b );
            clean.add( b );
        }
        else
            lru.remove( b );
        return b;
    }
    block * get_for_write( int id ) {
        block * b = hash.find( id );
        if ( !b )
        {
            b = new block;
            b->id = id;
            memset( b->buf, 0x55, BLOCK_SIZE );
            hash.add( b );
            dirty.add( b );
        }
        else
        {
            lru.remove( b );
            if ( clean.onthislist( b ))
            {
                clean.remove( b );
                dirty.add( b );
            }
        }
        return b;
    }
    void release( block * b, bool dirtyflag ) {
        if ( dirtyflag && clean.onthislist( b ))
        {
            clean.remove( b );
            dirty.add( b );
        }
        lru.add( b );
        trim();
    }
    void flush( void ) {
        block * b;
        while ( b = dirty.dequeue_head() )
        {
            b->write( fd );
            clean.add( b );
        }
    }
private:
    void trim( void ) {
        while ( lru.get_cnt() > max_blocks )
        {
            block * b = lru.dequeue_tail();
            if ( dirty.onthislist( b ))
            {
                b->write( fd );
                dirty.remove( b );
            }
            else if ( clean.onthislist( b ))
            {
                clean.remove( b );
            }
            hash.remove( b );
            delete b;
        }
    }
};

int
main()
{
    int fd;

    block_cache * bc;
    block       * b;

    fd = open( "testfile", O_RDWR | O_CREAT, 0644 );
    bc = new block_cache( fd, 5000, 200 );

    b = bc->get( 12 );
    b->buf[ 4 ] = 5;
    bc->release( b, true );

    b = bc->get_for_write( 13 );
    b->buf[ 5 ] = 6;
    bc->release( b, true );

    delete bc;

    return 0;
};
