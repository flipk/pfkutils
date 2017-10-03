
#include "recnomap.H"

// recnos in this class are not the same as recnos in 
// FileRecordNumber class. for each 128 records in
// FileRecordNumberMap, there is one more free-record
// bitmap. method "recno_getinfo" returns info on how
// to locate real record in FileRecordNumber.

// for instance:
// frnm# frn# offset
//  0    1      0
//  127  128    127
//  128  130    0

FileRecordNumberMap :: FileRecordNumberMap( FileRecordNumber * _f )
{
    f = _f;
    record_length = f->record_length;
    recmapsize = f->record_length * 8;
    first_free_recno = 0;
}

FileRecordNumberMap :: ~FileRecordNumberMap( void )
{
    delete f;
}

int
FileRecordNumberMap :: alloc( void )
{
    int recno, real_recno, bm_recno, bm_byte, bm_bit;
    int firstone;
    UINT32 magic;
    UCHAR * bm;

    while ( 1 )
    {
        recno = first_free_recno;
        bm = f->get_record( recno_getinfo3( recno ), magic );
        firstone = find_first_one( bm );
        if ( firstone != -1 )
        {
            recno += firstone;
            recno_getinfo( recno, real_recno,
                           bm_recno, bm_byte, bm_bit );
            bm[ bm_byte ] |= 1 << bm_bit;
            f->unlock_record( magic, true );
            return recno;
        }
        f->unlock_record( magic, false );
        first_free_recno += recmapsize;
    }
}

void
FileRecordNumberMap :: free( int recno )
{
    int real_recno, bm_recno, bm_byte, bm_bit;
    recno_getinfo( recno, real_recno, bm_recno, bm_byte, bm_bit );

    UINT32 magic;
    UCHAR * bm = f->get_record( bm_recno, magic );

    int mask = 1 << bm_bit;
    if (( bm[ bm_byte ] & mask ) == 0 )
    {
        f->unlock_record( magic, false );
        printf( "error? free recno already freed %s:%d\n",
                __FILE__, __LINE__ );
        return;
    }

    bm[ bm_byte ] &= ~mask;
    f->unlock_record( magic, true );

    int new_first_free = ( recno / recmapsize ) * recmapsize;
    if ( new_first_free < first_free_recno )
        first_free_recno = new_first_free;
}

UCHAR *
FileRecordNumberMap :: get_record( int recno, UINT32& magic )
{
    int real_recno, bm_recno, bm_byte, bm_bit;
    recno_getinfo( recno, real_recno, bm_recno, bm_byte, bm_bit );

    UINT32 bm_magic;
    UCHAR * bm = f->get_record( bm_recno, bm_magic );
    bool bad = false;

    if (( bm[ bm_byte ] & ( 1 << bm_bit )) == 0 )
        bad = true;
    f->unlock_record( bm_magic, false );

    if ( bad )
    {
        printf( "FileRecordNumberMap :: get_record : "
                "requested record is free! %s:%d\n",
                __FILE__, __LINE__ );
        return NULL;
    }

    return f->get_record( real_recno, magic );
}

UCHAR *
FileRecordNumberMap :: get_empty_record( int recno, UINT32& magic )
{
    int real_recno, bm_recno, bm_byte, bm_bit;
    recno_getinfo( recno, real_recno, bm_recno, bm_byte, bm_bit );

    UINT32 bm_magic;
    UCHAR * bm = f->get_record( bm_recno, bm_magic );
    bool bad = false;

    if (( bm[ bm_byte ] & ( 1 << bm_bit )) == 0 )
        bad = true;
    f->unlock_record( bm_magic, false );

    if ( bad )
    {
        printf( "FileRecordNumberMap :: get_empty_record : "
                "requested record is free! %s:%d\n",
                __FILE__, __LINE__ );
        return NULL;
    }

    return f->get_empty_record( real_recno, magic );
}

void
FileRecordNumberMap :: unlock_record( UINT32 magic, bool dirty )
{
    f->unlock_record( magic, dirty );
}
