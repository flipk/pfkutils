
#include "recnoblock.H"

FileBlockNumber :: FileBlockNumber( FileRecordNumberMap * _map,
                                    int _cachesize )
{
    map = _map;
    block_piece_len = map->record_length;
    cachesize = _cachesize;
    hash = new block_mgt*[ cachesize ];
    for ( int i = 0; i < cachesize; i++ )
        hash[i] = NULL;
}

FileBlockNumber :: ~FileBlockNumber( void )
{
    block_mgt *x, *y;
    int i;

    for ( i = 0; i < cachesize; i++ )
        for ( x = hash[i]; x; x = y )
        {
            y = x->next;

            // since application didn't clean up and
            // unlock all outstanding magics, 
            // unlock them for the application and
            // assume they're all dirty, since we 
            // don't know.

            printf( "FileBlockNumber destructor: "
                    "application left blockno %d behind\n",
                    x->blockno );

            x->unlock( map, true );
            delete x;
        }

    delete[] hash;
    delete map;
}

FileBlockNumber ::
block_entry :: block_entry( int bpl, FileRecordNumberMap * map, int _recno )
{
    block_piece_len = bpl;
    next = NULL;
    recno = _recno;
    dat = map->get_record( recno, magic );
}

void
FileBlockNumber ::
block_entry :: unlock( FileRecordNumberMap * map, bool dirty )
{
    map->unlock_record( magic, dirty );
    dat = NULL;
    magic = 0;
}

FileBlockNumber ::
block_mgt :: block_mgt( int block_piece_len, 
                        FileRecordNumberMap * map, int _blockno )
{
    // first populate the block_entry chain
    block_entry ** next_dest = &records;
    int cur_recno = _blockno;
    blockno = _blockno;
    int _blocksize = 0;
    refcount = 0;

    do {
        block_entry * be = new block_entry( block_piece_len, map, cur_recno );
        if ( be->dat == NULL )
        {
            // we're probably fucked at this point.
            // hmm. lets see when this actually happens.
            delete be;
            throw constructor_failed();
        }
        *next_dest = be;
        cur_recno = be->next_rec();
        next_dest = &be->next;
        _blocksize += be->piece_len();
    } while ( cur_recno != 0 );

    blocksize = *(UINT16*)(records->dat);

    // now make a user_dat and populate it
    user_dat = new UCHAR[ _blocksize ];
    UCHAR * p = user_dat;
    block_entry * be;
    for ( be = records; be; be = be->next )
    {
        if ( be == records )
        {
            memcpy( p, be->dat + 2, be->piece_len() - 2 );
            p += be->piece_len() - 2;
        }
        else
        {
            memcpy( p, be->dat, be->piece_len() );
            p += be->piece_len();
        }
    }
}

FileBlockNumber ::
block_mgt :: ~block_mgt( void )
{
    delete[] user_dat;
    block_entry *x, *y;
    for ( x = records; x; x = y )
    {
        y = x->next;
        delete x;
    }
}

void
FileBlockNumber ::
block_mgt :: unlock( FileRecordNumberMap * map, bool dirty )
{
    block_entry *x;
    UCHAR * p = user_dat;
    for ( x = records; x; x = x->next )
    {
        if ( x == records )
        {
            memcpy( x->dat + 2, p, x->piece_len() - 2 );
            p += x->piece_len() - 2;
        }
        else
        {
            memcpy( x->dat, p, x->piece_len() );
            p += x->piece_len();
        }
        x->unlock( map, dirty );
    }
}

// return a blockno
int
FileBlockNumber :: alloc( int _bytes )
{
    int recno = -1, i, numrecords, bytes = _bytes;
    block_entry * first = NULL;
    block_entry * cur = NULL;
    block_entry ** next_dest = &first;

    // compensate for space needed for size
    bytes += 2;

    i = 0;
    do {
        int newrec = map->alloc();
        if ( recno == -1 )
            recno = newrec;
        if ( cur != NULL )
        {
            cur->setnext( newrec );
            bytes -= cur->piece_len();
        }
        cur = new block_entry( block_piece_len );
        *next_dest = cur;
        next_dest = &(cur->next);
        cur->next = NULL;
        cur->recno = newrec;
        cur->dat = map->get_empty_record( newrec, cur->magic );
        cur->setnext( 0 );
    } while ( bytes > cur->piece_len() );

    *(UINT16*)(first->dat) = _bytes;

    while ( 1 )
    {
        cur = first;
        if ( cur == NULL )
            break;
        first = first->next;
        map->unlock_record( cur->magic, true );
        delete cur;
    }

    return recno;
}

void
FileBlockNumber :: free( int blockno )
{
    block_entry be( block_piece_len );

    while ( blockno != 0 )
    {
        be.recno = blockno;
        be.dat = map->get_record( blockno, be.magic );
        blockno = be.next_rec();
        map->unlock_record( be.magic, false );
        map->free( be.recno );
    }
}

UCHAR *
FileBlockNumber :: get_block( int blockno, UINT32 &magic )
{
    int dummy;
    return get_block( blockno, dummy, magic );
}

UCHAR *
FileBlockNumber :: get_block( int blockno, int &size, UINT32 &magic )
{
    block_mgt * bm;
    int h = hashv( blockno );

    // search cache
    for ( bm = hash[h]; bm; bm = bm->next )
        if ( bm->blockno == blockno )
            break;

    if ( bm == NULL )
    {
        try {
            bm = new block_mgt( block_piece_len, map, blockno );
        }
        catch ( block_mgt::constructor_failed ) {
            // hmm. this sucks.
            printf( "FileBlockNumber::get_block::block_mgt: "
                    "constructor failed!\n" );
            return NULL;
        }
        // insert into cache
        bm->next = hash[h];
        hash[h] = bm;
    }

    magic = (UINT32)bm;
    size = bm->blocksize;
    bm->refcount++;
    return bm->user_dat;
}

void
FileBlockNumber :: unlock_block( UINT32 magic, bool dirty )
{
    block_mgt * bm = (block_mgt*) magic;

    if ( dirty )
        bm->dirty = true;

    if ( --(bm->refcount) == 0 )
    {
        block_mgt *x, *ox;
        int h = hashv( bm->blockno );

        bm->unlock( map, bm->dirty );

        // delete from hash
        for ( ox = NULL, x = hash[h]; x; ox = x, x = x->next )
            if ( x == bm )
                break;

        if ( ox == NULL )
            hash[h] = x->next;
        else
            ox->next = x->next;

        x->next = NULL;
        delete bm;
    }
}
