
#include "hsc.H"

hsc :: hsc( int _cage, int _slot7_mqid, int _slot9_mqid )
{
    cage = _cage;
    slot7_mqid = _slot7_mqid;
    slot9_mqid = _slot9_mqid;
    owner = 0;
}

bool
hsc :: take( int slot, bcast_func_type func, void * arg )
{
    if ( owner == 0 )
    {
        owner = slot;
        return true;
    }
    if ( owner == slot )
    {
        return true;
    }

    func( arg, BCAST_START );
    HSC_OTHER_ATTEMPTED_CLAIM * hoac = new HSC_OTHER_ATTEMPTED_CLAIM;
    hoac->dest.set( (slot == 7) ? slot9_mqid : slot7_mqid );
    send( hoac, &hoac->dest );
    func( arg, BCAST_FINISH );

    return false;
}

void
hsc :: release( int slot )
{
    if ( owner == slot )
    {
        HSC_OTHER_RELEASED * hor = new HSC_OTHER_RELEASED;
        hor->dest.set( (owner == 7) ? slot9_mqid : slot7_mqid );
        owner ^= (7^9);
        send( hor, &hor->dest );
        return;
    }
}

void
hsc :: reset_other( int slot )
{
    if ( owner != slot )
    {
        printf( "can't reset other cuz i don't own domain!\n" );
        return;
    }

    HSC_RESET_PLEASE * hrp = new HSC_RESET_PLEASE;
    hrp->dest.set( (slot == 7) ? slot9_mqid : slot7_mqid );
    send( hrp, &hrp->dest );
}
