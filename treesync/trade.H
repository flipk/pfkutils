
#ifndef __TRADE_H_
#define __TRADE_H_

void trade_init( int fd, bool first );
void trade_files( void );
void trade_close( void );


#define MESSAGE_LIST                               \
                                                   \
PkTcpMsgDef( RegenStarted        ,  0x6c81c36b, ); \
                                                   \
PkTcpMsgDef( RegenDone           ,  0xdebbe210, ); \
                                                   \
PkTcpMsgDef( AffectedFile        ,  0x1a079393,    \
             char       filename[ MAX_PK_PATH ];   \
    );                                             \
                                                   \
PkTcpMsgDef( AffectedFileStatus  ,  0x0573d267,    \
             UINT8      conflicted;                \
    );                                             \
                                                   \
PkTcpMsgDef( AffectedDone        ,  0xd0cd0c73, ); \
                                                   \
PkTcpMsgDef( ChangedFileName     ,  0xa02d472f,    \
             char       filename[ MAX_PK_PATH ];   \
             UINT32_t   file_length;               \
    );                                             \
                                                   \
PkTcpMsgDef( ChangedFileContents ,  0x4cdea40d,    \
             UINT16_t   piece_length;              \
             char       buffer[ MAX_PIECE_LEN ];   \
    );                                             \
                                                   \
PkTcpMsgDef( ChangedFileDone     ,  0x652a7e6d, ); \
                                                   \
PkTcpMsgDef( RemovedFileName     ,  0x17b753ad,    \
             char       filename[ MAX_PK_PATH ];   \
    );                                             \
                                                   \
PkTcpMsgDef( SyncDone            ,  0x606b53a3, );


#endif /* __TRADE_H_ */
