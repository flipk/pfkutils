
#ifndef __PIPE_MGR_H__
#define __PIPE_MGR_H__

#include "types.H"
#include "packet_encoder.H"
#include "packet_decoder.H"

class Pipe_instance;

class Pipe_Mgr {
    static const int max_pipes = 256;
    Pipe_instance * pipes[ max_pipes ];
    packet_encoder * encoder;
public:
    Pipe_Mgr( void );
    ~Pipe_Mgr( void );

    void register_encoder( packet_encoder * _encoder ) { 
        encoder = _encoder;
    }
    void deregister_encoder( void ) {
        encoder = NULL;
    }
    // ?
    void handle_pkt( short pipeno, char * buf, int len );
};

#endif /* __PIPE_MGR_H__ */
