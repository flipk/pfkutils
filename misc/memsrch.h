
#ifndef _MEMSRCH_H_
#define _MEMSRCH_H_

#include "mytypes.h"

#ifdef __cplusplus
extern "C" {
#endif

int memsrch( uchar * bigbuf, uint biglen,
             uchar * littlebuf, uint littlelen );

#ifdef __cplusplus
}; /*extern "C"*/
#endif

#endif /* _MEMSRCH_H_ */
