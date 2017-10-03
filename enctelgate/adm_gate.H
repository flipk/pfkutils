
#include "fd_mgr.H"
#include "packet_encoder.H"
#include "packet_decoder.H"
#include "circular_buffer.H"

class Adm_Gate_fd;

class Adm_pkt_encoder_io : public packet_encoder_io {
protected:
    Adm_Gate_fd  * me;
public:
    Adm_pkt_encoder_io ( void ) { /* nothing */ }
    void setup_me      ( Adm_Gate_fd * _me    ) { me = _me; }

    /*virtual*/ void outbytes ( char * buf, int len ) = 0;
};

class Adm_pkt_decoder_io : public packet_decoder_io {
protected:
    Adm_Gate_fd * me;
    Adm_Gate_fd * other;
public:
    Adm_pkt_decoder_io ( void ) { /* nothing */ }
    void setup_me      ( Adm_Gate_fd * _me    ) { me = _me; }
    void setup_other   ( Adm_Gate_fd * _other ) { other = _other; }

    /*virtual*/ void outbytes ( char *, int ) = 0;
    /*virtual*/ void outpacket( char *, int ) = 0;
};

class Adm_Gate_fd : public fd_interface {
    bool                 connecting;
    bool                 doread;
    bool                 dowrite;

    static const int     max_write  = 25000;
    circular_buffer      write_buf;

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
                 Adm_pkt_encoder_io * _encoder,
                 Adm_pkt_decoder_io * _decoder );
    /*virtual*/ ~Adm_Gate_fd( void );

    /**/        void setup_other( class Adm_Gate_fd * _other );

    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ rw_response write( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * rd, bool * wr );
    /*virtual*/ bool over_write_threshold( void );
    /*virtual*/ bool write_to_fd( char * buf, int len );
};
