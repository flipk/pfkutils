
void myputs( char * s );
void myputwhitespace( void );
void myputpreproc( char * s );
void consume_preproc( int (*func)(void) );
void consume_define( int (*func)(void) );
