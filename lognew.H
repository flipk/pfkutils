#if 1
#define LOGNEW new
#define MALLOC(size) malloc( size )
#define STRDUP(str) strdup( str )
#else

#ifdef __cplusplus

void * operator new     ( size_t s, char * file, int line );
void * operator new[]   ( size_t s, char * file, int line );

#define LOGNEW new(__FILE__,__LINE__)

extern "C" {

#endif

void * malloc_record( char * file, int line, int size );
char * strdup_record( char * file, int line, char *str );
#define MALLOC(size) malloc_record( __FILE__, __LINE__, size )
#define STRDUP(str) strdup_record( __FILE__, __LINE__, str )

#ifdef __cplusplus
};
#endif
#endif /* if 0 */
