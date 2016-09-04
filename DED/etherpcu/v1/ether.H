
#ifndef __ETHER_H_
#define __ETHER_H_

#include "threads.H"

#define ETHER_TYPE_LIST \
    ETHER_TYPE_ITEM( E_START_BCAST,   "start broadcast"  ) \
    ETHER_TYPE_ITEM( E_FINISH_BCAST,  "finish broadcast" ) \
    ETHER_TYPE_ITEM( E_CLAIM,         "claim"            ) \
    ETHER_TYPE_ITEM( E_CLAIM_ACK,     "claim ack"        ) \
    ETHER_TYPE_ITEM( E_CLAIM_NMM,     "claim nmm"        ) \
    ETHER_TYPE_ITEM( E_HEARTBEAT,     "heartbeat"        ) \
    ETHER_TYPE_ITEM( E_HEARTBEAT_ACK, "heartbeat ack"    )

#define ETHER_TYPE_ITEM(a,b) a,
enum eth_msg_type {
    ETHER_TYPE_LIST
};
#undef  ETHER_TYPE_ITEM

#ifdef INCLUDE_ETHER_TYPE_NAMES
#define ETHER_TYPE_ITEM(a,b) b,
static const char * ether_type_names[] = {
    ETHER_TYPE_LIST
    0
};
#undef  ETHER_TYPE_ITEM
#endif

#undef ETHER_TYPE_LIST

MsgDef( ETHERNET_MSG, 0xe37bdc2,
        eth_msg_type ether_type;
        int src_mac;
        int dest_mac; // -1 if broadcast
    );

class ethernet : public ThreadShortCuts {
    int max_mqs;
    int nummqs;
    int * mqs;
public:
    ethernet( int _max_mqs );
    int register_mac( int mqid );
    void unregister_mac( int mac );
    void tx( int src_mac, int dest_mac, eth_msg_type type );
    void tx( int src_mac, eth_msg_type type );
    static const char * type_name( eth_msg_type type );
};

extern ethernet * ether;

#endif /* __ETHER_H_ */
