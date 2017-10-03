
#include "../threads/h/threads.H"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

class connLog {
    FILE * f;
    int lognumber;
public:
    connLog( void );
    ~connLog( void );
    enum direction {
        FROM_BROWSER,
        FROM_SITE,
        TO_BROWSER,
        TO_SITE
    };
    void logdat( direction, char * buf, int len );
    void print( char * format, ... );
    int get_lognumber( void ) { return lognumber; }
};
