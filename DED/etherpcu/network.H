
#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <pthread.h>

#include "dll2.H"

extern char * packet_names[];

// packet_names in network.C and the list below must be kept in sync!
enum packet_type {
    PKSTGEN_TIMER,
    PACKET_WAKEUP_INIT_TIME,
    PACKET_BCI_S,
    PACKET_BCI_F
    //xxx
};

struct net_packet {
    LListLinks <net_packet> links[1];
    packet_type type;
    int src;  // mac
    int dest; // mac, -1 if bcast
    int extra; // an extra value member
private:
    net_packet( void ) { src = dest = -1; }
    ~net_packet( void ) { /**/ }
    friend net_packet * packet_alloc( void );
    friend void net_cleanup( void );
};

#define WAIT_FOREVER  -1
#define BROADCAST_MAC -1

#define MSG_ARGS packet_type type, int src, int dest, int extra

void packet_free( net_packet * );
net_packet * packet_alloc( MSG_ARGS );
void packet_send( MSG_ARGS );
void packet_send( net_packet * );


// called at exit-time
void net_cleanup( void );

struct net_iface {
    net_iface( void );
    net_packet * receive( int timeout ); /* tenths of a second */
    net_packet * receive( void ) { return receive( -1 ); }
    void send( net_packet * );

    void activate( void );
    void deactivate( void );
private:    
    void   lock( void ) { pthread_mutex_lock  ( &mtx ); }
    void unlock( void ) { pthread_mutex_unlock( &mtx ); }

    bool                  active;
    pthread_mutex_t       mtx;
    pthread_cond_t        cond;
    LList <net_packet,0>  pkts;
};

extern net_iface * mproc_nets;

enum hsc_state {
    HSC_ACTIVE,
    HSC_FREE
};

struct hsc_controller {
    pthread_mutex_t  mtx;
    hsc_state  domains[2];
    hsc_controller( void ); 
    void   lock( void ) { pthread_mutex_lock  ( &mtx ); }
    void unlock( void ) { pthread_mutex_unlock( &mtx ); }
};

extern hsc_controller * hscs;

void init_nets( int num_mprocs );
void activate_mproc( int mac );
void deactivate_mproc( int mac );

#endif /* __NETWORK_H__ */
