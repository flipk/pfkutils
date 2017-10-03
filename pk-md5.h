
#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#define u_int32_t unsigned int

typedef struct MD5Context {
    u_int32_t state[4];         /* state (ABCD) */
    u_int32_t count[2];	        /* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64];   /* input buffer */
} MD5_CTX;

void   MD5Init      ( MD5_CTX *context );
void   MD5Update    ( MD5_CTX *context, const unsigned char *data,
                      unsigned int len );
void   MD5Final     ( unsigned char digest[16], MD5_CTX *context );
char * MD5End       ( MD5_CTX *context, char *buf );
char * MD5File      ( const char *filename, char *buf );
char * MD5FileChunk ( const char *filename, char *buf, off_t offset,
                      off_t length );
char * MD5Data      ( const unsigned char *data,
                      unsigned int len, char *buf );
void   MD5Pad       ( MD5_CTX *context );


#ifdef __cplusplus
};
#endif
