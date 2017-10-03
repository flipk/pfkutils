/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#ifndef __BASE64_H__
#define __BASE64_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "types.H"

int b64_is_valid_char( unsigned char c );

/* return 4 if ok, 0 if not ok */
int b64_encode_quantum( unsigned char * in3, int in_len, unsigned char * out4 );

/* return length of bytes decoded, or 0 if not ok */
int b64_decode_quantum( unsigned char * in4, unsigned char * out3 );


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __BASE64_H__ */
