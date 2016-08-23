
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>

#include "libpfkscriptutil.h"

using namespace std;

int
main()
{
    pfkscript_ctrl   ctrl;
    if (ctrl.ok())
    {
        string  path;
        bool open;
        bool ret = ctrl.getFile(path, open);
        if (ret == false)
        {
            printf("failure\n");
        }
        else
        {
            printf("open: %d\n", open);
            if (open)
            {
                printf("path: %s\n", path.c_str());
            }
        }
    }
    return 0;
}
