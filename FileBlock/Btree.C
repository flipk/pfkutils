
#include "Btree.H"
#include "Btree_internal.H"

#include <stdio.h>

//static
Btree *
Btree :: open( FileBlockInterface * _fbi )
{
    if (BtreeInternal::valid_file(_fbi))
        return new BtreeInternal(_fbi);
    return NULL;
}

//static
bool
Btree :: init_file( FileBlockInterface * _fbi )
{

    return false;
}

const char BtreeInternal::BTInfoFileInfoName[] = "PFKBTREEINFO";

//static
bool
BtreeInternal :: valid_file( FileBlockInterface * _fbi )
{
    //xxx
    return true;
}

BtreeInternal :: BtreeInternal( FileBlockInterface * _fbi )
{
    fbi = _fbi;
    //xxx
}

//virtual
BtreeInternal :: ~BtreeInternal(void)
{
    //xxx
}

//virtual
bool
BtreeInternal :: get( _BTDatum * key, _BTDatum * data )
{
    return false;
}

//virtual
bool
BtreeInternal :: put( _BTDatum * key, _BTDatum * data )
{
    return false;
}

//virtual
bool
BtreeInternal :: del( _BTDatum * key  )
{
    return false;
}
