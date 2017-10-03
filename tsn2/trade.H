#ifndef __TRADE_H___
#define __TRADE_H___

#include "pk_tcp_msg.H"

class tsn_tcp_chan : public pk_tcp_msgr {
private:
    /*virtual*/ int reader( void * buf, int buflen );
    /*virtual*/ int writer( void * buf, int buflen );
    /*virtual*/ bool read_pending( void );
    int fd;
public:
    tsn_tcp_chan( int _fd ) { fd = _fd; }
    virtual ~tsn_tcp_chan( void ) { close( fd ); }
};

extern tsn_tcp_chan * tcp_chan;

bool trade_first( void );
bool trade_second( void );

#endif /* __TRADE_H___ */
