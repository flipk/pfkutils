
#ifndef __MESSAGES_H_
#define __MESSAGES_H_

struct DataBuffer {
    static const int MAX_DATA_SIZE = 1000;
    unsigned short len;
    unsigned char data[ MAX_DATA_SIZE ];
};

enum {
    DummyMsgType,
    AppDataType,  // 1
    AppQuenchType, // 2
    AppUnquenchType, // 3
    NetworkDataType, // 4
    NetworkQuenchType, // 5
    NetworkUnquenchType, // 6
    NetworkAckType, // 7
    TxTimerExpireType, // 8
    TxFreeTimerType,  // 9
    UserDataType, // 10
    ConnectReqType, // 11
    ConnectAckType, // 12
    ConnectRefusedType, // 13
    ConnectCloseType, // 14
    ConnectCloseAckType, // 15
    AuthChallengeReqType, // 16
    AuthChallengeRespType, // 17
};

// TXAPP to TX, RX to RXAPP
PkMsgIntDef( AppData, AppDataType,
             DataBuffer * data;
    );

// RXAPP to RX, TX to TXAPP
PkMsgIntDef( AppQuench, AppQuenchType,
             int channel;
    );
PkMsgIntDef( AppUnquench, AppUnquenchType,
             int channel;
    );

// TX to TXNET, TXNET to RXNET, RXNET to RX
PkMsgIntDef( NetworkData, NetworkDataType,
             int channel;
             unsigned char seqno;
             DataBuffer * data; // note, NETWORK SHOULD NOT FREE AFTER TX!
    );

// RX to RXNET, RXNET to TXNET, TXNET to TX
PkMsgIntDef( NetworkQuench, NetworkQuenchType,
             int channel;
    );
PkMsgIntDef( NetworkUnquench, NetworkUnquenchType,
             int channel;
    );

// RX to NET, NET to NET, NET to TX
PkMsgIntDef( NetworkAck, NetworkAckType,
             int channel;
             unsigned char seqno;
    );

// internal to TX
struct DataBufferEntry;
PkMsgIntDef( TxTimerExpire, TxTimerExpireType,
             DataBufferEntry * data;
    );
PkMsgIntDef( TxFreeTimer, TxFreeTimerType,
             int dummy;
    );


// CM

PkMsgIntDef( AuthChallengeReq, AuthChallengeReqType,
             int keyset;
             int keynumber;
    );
PkMsgIntDef( AuthChallengeResp, AuthChallengeRespType,
             int keyset;
             int keynumber;
             unsigned char key[64];
    );
PkMsgIntDef( ConnectReq, ConnectReqType,
             int channel;
    );
PkMsgIntDef( ConnectAck, ConnectAckType,
             int channel;
    );
PkMsgIntDef( ConnectRefused, ConnectRefusedType,
             int channel;
    );
PkMsgIntDef( ConnectClose, ConnectCloseType,
             int channel;
    );
PkMsgIntDef( ConnectCloseAck, ConnectCloseAckType,
             int channel;
    );

// User data

PkMsgIntDef( UserData, UserDataType,
             DataBuffer * data;
    );

#endif /* __MESSAGES_H_ */
