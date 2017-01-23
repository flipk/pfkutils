
void count_line( void );
void list_add( WENTLIST *, WENT * );
void machine_addname( WENT * w );
void inputlist_add_timeout( void );
void statelist_add_unknown( void );
void add_state( char * statename, WENT * pre, WENT * inputs );
void add_next( WENT * state );
WENT * lookup_inputname( char * );
WENT * lookup_outputname( char * );
void add_call( WENT * call );
VERBATIM * copy_verbatim( int (*)(void) );
