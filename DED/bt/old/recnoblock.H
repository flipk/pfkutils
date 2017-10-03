
#ifndef __RECNOBLOCK_H__
#define __RECNOBLOCK_H__

#include "recnomap.H"

//
// allocate blocks as a bunch of records.
// blocks can be any size.
// the "cache size" argument refers to the width of a hash
// table used to hold all locked blocks that exist simultaneously.
// when a block is unlocked it is removed from this hash.
//

class FileBlockNumber {
    FileRecordNumberMap * map;
    int block_piece_len;
    // we keep one of these for each record.
    struct block_entry {
        block_entry( int bpl ) { block_piece_len = bpl; }
        block_entry( int bpl, FileRecordNumberMap * map, int recno );
        void unlock( FileRecordNumberMap * map, bool dirty );
        block_entry * next;
        UINT32 magic;
        int recno;
        int block_piece_len;
        UCHAR *dat;
        int piece_len( void ) {
            if (( dat[block_piece_len-1] & 0x80 ) == 0 )
                return block_piece_len-1;
            if (( dat[block_piece_len-2] & 0x80 ) == 0 )
                return block_piece_len-2;
            if (( dat[block_piece_len-3] & 0x80 ) == 0 )
                return block_piece_len-3;
            return block_piece_len-4;
        }
        int next_rec( void ) {
            if ( dat[block_piece_len-1] == 0 )
                return 0;
            int o = 0;
            o += (dat[block_piece_len-1] & 0x7f);
            if (( dat[block_piece_len-1] & 0x80 ) == 0 )
                return recno + o;
            o += (dat[block_piece_len-2] & 0x7f) << 7;
            if (( dat[block_piece_len-2] & 0x80 ) == 0 )
                return recno + o;
            o += (dat[block_piece_len-3] & 0x7f) << 14;
            if (( dat[block_piece_len-3] & 0x80 ) == 0 )
                return recno + o;
            o += dat[block_piece_len-4] << 21;
            return recno + o;
        }
        void setnext( int nrecno ) {
            if ( nrecno == 0 )
            {
                dat[block_piece_len-1] = 0;
                return;
            }
            int o = nrecno - recno;
            dat[block_piece_len-1] = (UCHAR)(o & 0x7f);
            if ( o < (1<<7) )
                return;
            dat[block_piece_len-1] |= 0x80;
            dat[block_piece_len-2] = (o >> 7) & 0x7f;
            if ( o < (1<<14) )
                return;
            dat[block_piece_len-2] |= 0x80;
            dat[block_piece_len-3] = (o >> 14) & 0x7f;
            if ( o < (1<<21) )
                return;
            dat[block_piece_len-3] |= 0x80;
            dat[block_piece_len-4] = (o >> 21) & 0xff;
            return;
        }
    };
    // we keep one of these for each block.
    struct block_mgt {
        class constructor_failed { };
        block_mgt( void ) {
            memset( (void*)this, 0, sizeof( block_mgt ));
        }
        block_mgt( int block_piece_len,
                   FileRecordNumberMap * map, int _blockno );
        ~block_mgt( void );
        void unlock( FileRecordNumberMap * map, bool dirty );
        block_mgt * next;
        int blockno;
        int blocksize;
        int refcount;
        bool dirty;
        block_entry * records;
        UCHAR *user_dat;
    };
    int cachesize;
    block_mgt ** hash;
    int hashv( int blockno ) { return blockno % cachesize; }
public:
    FileBlockNumber( FileRecordNumberMap * map, int cachesize );
    ~FileBlockNumber( void );
    int alloc( int bytes );   // return a blockno
    void free( int blockno );
    UCHAR * get_block( int blockno, int &size, UINT32 &magic );
    UCHAR * get_block( int blockno, UINT32 &magic );
    void unlock_block( UINT32 magic, bool dirty );
    int flush( void ) { return map->flush(); }
};

#endif
