
#include "pick.H"

extern "C" int
pfktel_keypick_main()
{
    pfk_key_pick(true);
    return 0;
}
