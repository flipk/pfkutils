
#include <stdio.h>
#include <string.h>

#define BUFPRINTF(buf,len,format...) \
    do { \
        len += snprintf(buf + len, sizeof(buf)-len-1, format); \
        if (len > (sizeof(buf)-1)) \
            len = sizeof(buf)-1; \
    } while (0)

int
main()
{
    char buf[4];
    int len = 0;

    BUFPRINTF(buf, len, "this ");
    BUFPRINTF(buf, len, "is ");
    BUFPRINTF(buf, len, "a ");
    BUFPRINTF(buf, len, "test\n");
    buf[len++] = 0;

    printf("%s", buf);
    return 0;
}
