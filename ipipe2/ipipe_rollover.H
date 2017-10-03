
#ifndef __IPIPE_ROLLOVER_H__
#define __IPIPE_ROLLOVER_H__

#include "m.h"

class ipipe_rollover { 
    int current_size;
    int max_size;
    char * filename;
    int flags;
    char * ret;
    int counter;
public:
    ipipe_rollover( int _max_size, char * _filename, int _flags );
    ~ipipe_rollover( void );
    void check_rollover( int fd, int added );
    char * get_next_filename(void);
};

#endif /* __IPIPE_ROLLOVER_H__ */
