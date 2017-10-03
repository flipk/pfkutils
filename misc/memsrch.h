/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

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
