
#include "fd_mgr.H"
#include "packet_encoder.H"
#include "packet_decoder.H"
#include "circular_buffer.H"
#include "pipe_mgr.H"

class Adm_pkt_decoder_io;
class Adm_pkt_encoder_io;

class Adm_Gate_fd : public fd_interface {
    bool                 connecting;
    bool                 doread;
    bool                 dowrite;

    static const int     max_write  = 25000;
    circular_buffer      write_buf;
    Pipe_Mgr           * pipe_mgr;

    Adm_pkt_decoder_io * decode_io;
    Adm_pkt_encoder_io * encode_io;
    packet_encoder     * encoder;
    packet_decoder     * decoder;

    class Adm_Gate_fd  * other_fd;

    friend class Adm_pkt_decoder_io;
    friend class Adm_pkt_encoder_io;

public:
    Adm_Gate_fd( int fd, bool connecting,
                 bool doread, bool dowrite,
                 bool doencoder, bool dodecoder,
                 Pipe_Mgr  * _pipemgr );
    /*virtual*/ ~Adm_Gate_fd( void );

    /**/        void setup_other( class Adm_Gate_fd * _other );

    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ rw_response write( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * rd, bool * wr );
    /*virtual*/ bool over_write_threshold( void );
    /*virtual*/ bool write_to_fd( char * buf, int len );
};
