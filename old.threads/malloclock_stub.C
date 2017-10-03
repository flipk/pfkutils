
void
malloclock_die( void )
{
}

extern "C" void * malloc( int );
extern "C" void free( void * );
#define size_t unsigned int

extern "C"
void *
malloc_record( char * file, int line, int s )
{
	return malloc( s );
}

extern "C"
char *
strdup_record( char * file, int line, char * s )
{
	int len = strlen( s ) + 1;
	char * ret = (char*) malloc( len );
	memcpy( ret, s, len );
	return ret;
}

void * operator new     ( size_t s ) { return (void*)malloc( s ); }
void * operator new[]   ( size_t s ) { return (void*)malloc( s ); }
void   operator delete  ( void * p ) { free( p ); }
void   operator delete[]( void * p ) { free( p ); }

void * operator new     ( size_t s, char * file, int line )
{
    return (void*)malloc_record( file, line, s );
}

void * operator new[]   ( size_t s, char * file, int line )
{
    return (void*)malloc_record( file, line, s );
}

