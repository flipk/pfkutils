
#ifndef __RECNOMAP_H__
#define __RECNOMAP_H__

#include "recno.H"

//
// alloc and free blocks dynamically.
// alloc returns a recno, and free deletes it.
// uses the above object for I/O.
// note: don't invoke methods from the above object
// using the frnmap recnos, as they don't map 1 to 1.
// instead use the get_record/put_record access methods
// in this class.
// in bitmap, 0 means free, 1 means used.
//

class FileRecordNumberMap {
private:
    int recmapsize;
    FileRecordNumber * f;
    int first_free_recno;
    // this is always a multiple of recmapsize.
    // what is the inverse function for this?
    // it doesn't matter, we never need one.
    void recno_getinfo( int recno, int &real_recno, int &bm_recno,
                        int &bm_byte, int &bm_bit ) {
        real_recno = recno_getinfo2( recno );
        int offset = recno % recmapsize;
        bm_recno = real_recno - offset - 1;
        bm_byte = offset / 8;
        bm_bit = offset % 8;
    }
    int recno_getinfo2( int recno ) {
        return ( recno + ( recno / recmapsize ) + 1 );
    }
    int recno_getinfo3( int recno ) {
        int real_recno = recno_getinfo2( recno );
        int offset = recno % recmapsize;
        return real_recno - offset - 1;
    }

    int find_first_one( UCHAR * bm ) {
        int i, ret = 0;
        // race thru bytes
        for ( i = 0; i < f->record_length; i++ )
        {
            if ( bm[i] != 0xff )
                break;
            ret += 8;
        }
        if ( i == f->record_length )
            return -1;
        // then slow down and look a bit at a time
        int k, j = 1;
        for ( k = 0; k < 8; k++ )
        {
            if (( bm[i] & j ) == 0 )
                return ret;
            ret++;
            j <<= 1;
        }

        printf( "error? %s:%d\n", __FILE__, __LINE__ );
        return -1;
    }
public:
    FileRecordNumberMap( FileRecordNumber * f );
    ~FileRecordNumberMap( void );
    int record_length;
    int alloc( void );
    void free( int recno );
    int flush( void ) { return f->flush(); }
    UCHAR * get_record( int recno, UINT32& magic );
    UCHAR * get_empty_record( int recno, UINT32& magic );
    void unlock_record( UINT32 magic, bool dirty );
};

#endif
