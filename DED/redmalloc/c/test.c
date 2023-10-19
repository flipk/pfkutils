
#include <stdio.h>
#include "redmalloc.h"

int
main()
{
    redmallocinit();

    char * ptr1 = REDMALLOC(5);
    char * ptr2 = REDMALLOC(254);
    char * ptr3 = REDMALLOC(120);

    redcheck(1);
    redfree(ptr3);
    redcheck(0);
    redfree(ptr1);
    redcheck(0);
    redfree(ptr2);
    redcheck(0);

    return 0;
}
