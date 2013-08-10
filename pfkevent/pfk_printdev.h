
#ifndef __PFK_PRINTDEV_H__
#define __PFK_PRINTDEV_H__

#include <stdarg.h>

extern int pfk_printdev_enabled(void);
extern void pfk_printdev(char *format, ...);
extern void pfk_printhexdev(unsigned char *buf, int len);
extern void printdev_init(void);
extern void printdev_exit(void);

#define PFKPR(format...)                        \
    do {                                        \
        if (pfk_printdev_enabled())             \
            pfk_printdev(format);               \
    } while(0)

#define DEVNAME "pfkprint"

#endif /* __PFK_PRINTDEV_H__ */
