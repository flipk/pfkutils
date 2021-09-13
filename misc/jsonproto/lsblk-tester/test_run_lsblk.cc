
#include <stdio.h>

#include "run_lsblk.h"

int
main()
{
    pfk::diskmanager::lsblk::BlockDevices_m devs;

    if (run_lsblk(devs) == false)
    {
        printf("failed to get lsblk output\n");
    }
    else
    {
        printf("lsblk output:\n%s\n", devs.DebugString().c_str());
    }

    return 0;
}
