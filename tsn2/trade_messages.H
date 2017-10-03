
#ifndef __TRADE_MESSAGES_H__
#define __TRADE_MESSAGES_H__

#include "pk_tcp_msg.H"
#include "magic.H"

PkTcpMsgDef( MsgScanStarted, MAGIC_SCAN_STARTED,
             );

PkTcpMsgDef( MsgScanComplete, MAGIC_SCAN_COMPLETE,
             );

#define TRADE_MAX_PATHLEN 200

PkTcpMsgDef( MsgFileDigest, MAGIC_FILE_DIGEST,
             MD5_DIGEST  digest;
             char filename[200];
             );

PkTcpMsgDef( MsgFileDigestAck, MAGIC_FILE_DIGEST_ACK,
             char filename[200];
             );

#if 0
PkTcpMsgDef( Msg, MAGIC_,
             );
PkTcpMsgDef( Msg, MAGIC_,
             );
PkTcpMsgDef( Msg, MAGIC_,
             );
PkTcpMsgDef( Msg, MAGIC_,
             );
PkTcpMsgDef( Msg, MAGIC_,
             );
PkTcpMsgDef( Msg, MAGIC_,
             );
PkTcpMsgDef( Msg, MAGIC_,
             );
#endif

#endif /* __TRADE_MESSAGES_H__ */
