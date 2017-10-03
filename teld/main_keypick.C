/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include "pick.H"

extern "C" int
pfktel_keypick_main()
{
    pfk_key_pick(true);
    return 0;
}
