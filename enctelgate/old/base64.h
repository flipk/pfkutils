
#ifndef __BASE64_H__
#define __BASE64_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "types.H"

int b64_is_valid_char( char c );

/* return 4 if ok, 0 if not ok */
int b64_encode_quantum( char * in3, int in_len, char * out4 );

/* return length of bytes decoded, or 0 if not ok */
int b64_decode_quantum( char * in4, char * out3 );


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __BASE64_H__ */
