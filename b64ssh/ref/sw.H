
#ifndef __SLIDING_WINDOW_H__
#define __SLIDING_WINDOW_H__

#include "pk_threads.H"
#include "messages.H"
#include "sw_int.H"

struct DataBufferEntry;

const int SLIDING_WINDOW_SIZE = 16;

class sliding_window_tx_thread : public PK_Thread {
    int channel;
    int net_qid; // for sending tx data
    int app_qid; // for sending quench/unquench
    int my_qid;
    unsigned char next_seqno; // for when a new pkt arrives from app
    bool net_quenched;
    bool app_quenched;
    DataBufferList tx_queue;
    DataBufferQueue delayed_free_queue;
    /*virtual*/ void entry( void );
    void start_tx_timer( DataBufferEntry * ent );
    void send_to_net( DataBufferEntry * ent );
    void send_app_quench( void );
    void send_app_unquench( void );
    void handle_tx_data( AppData * swtx );
    void handle_tx_timer( TxTimerExpire * txtimer );
    void handle_net_quench( NetworkQuench * quench );
    void handle_net_unquench( NetworkUnquench * unquench );
    void handle_net_ack( NetworkAck * ack );
    void handle_free_timer( void );
public:
    sliding_window_tx_thread( int _channel );
    ~sliding_window_tx_thread( void );
    int get_my_qid(void) { return my_qid; }
    void register_app_net_qids(int _app, int _net) {
        app_qid = _app; net_qid = _net;
    }
};

class sliding_window_rx_thread : public PK_Thread {
    int channel;
    int app_qid;
    int net_qid;
    int my_qid;
    bool app_quenched; // app has told us to quench
    bool net_quenched; // we have told net to quench
    unsigned char next_app_seqno; // next seqno to application
    DataBufferList rx_queue;
    /*virtual*/ void entry( void );
    void send_ack( unsigned char seqno );
    void send_net_quench( void );
    void send_net_unquench( void );
    void handle_app_quench( AppQuench * q );
    void handle_app_unquench( AppUnquench * q );
    void handle_network_data( NetworkData * data );
public:
    sliding_window_rx_thread( int _channel );
    ~sliding_window_rx_thread( void );
    int get_my_qid(void) { return my_qid; }
    void register_app_net_qids(int _app, int _net) {
        app_qid = _app; net_qid = _net;
    }
};

#endif /* __SLIDING_WINDOW_H__ */
