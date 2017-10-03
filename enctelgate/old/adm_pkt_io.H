
class Adm_pkt_decoder_io : public packet_decoder_io {
public:
    Adm_Gate_fd * me;
    Adm_Gate_fd * other;
    Pipe_Mgr    * pipe_mgr;
    Adm_pkt_decoder_io( Adm_Gate_fd * _me, Pipe_Mgr * _pipe_mgr ) {
        me = _me;  pipe_mgr = _pipe_mgr;
    }
    void setup_other ( Adm_Gate_fd * _other ) { other = _other; }
    void outbytes( char *, int );
    void outpacket( short pipeno, char *, int );
};


class Adm_pkt_encoder_io : public packet_encoder_io {
public:
    fd_interface * me;
    Adm_pkt_encoder_io( fd_interface * _me ) {
        me = _me;
    }
    void outbytes( char * buf, int len );
};
