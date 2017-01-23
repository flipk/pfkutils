
// these are methods that redmalloc provides.
extern "C" void * _redmalloc( int size, int pc );
extern "C" void * _redcalloc( int num, int size, int pc );
extern "C" void * _redrealloc( void * p, int new_sz, int pc );
extern "C" void   _redfree( void * ptr );
extern "C" void   _redmallocaudit( void );

extern "C" void redmallocspawntask( void );
extern "C" void red_malloc_display( void );
