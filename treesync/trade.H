
#ifndef __TRADE_H_
#define __TRADE_H_

void trade_init( int fd, bool first );
void trade_files( void );
void trade_close( void );


#define MESSAGE_LIST                               \
                                                   \
PkMsgDef( RegenStarted        ,  0x6c81c36b, );    \
                                                   \
PkMsgDef( RegenDone           ,  0xdebbe210, );    \
                                                   \
PkMsgDef( AffectedFile        ,  0x1a079393,       \
             char       filename[ MAX_PK_PATH ];   \
    );                                             \
                                                   \
PkMsgDef( AffectedFileStatus  ,  0x0573d267,       \
             UINT8      conflicted;                \
    );                                             \
                                                   \
PkMsgDef( AffectedDone        ,  0xd0cd0c73, );    \
                                                   \
PkMsgDef( ChangedFileName     ,  0xa02d472f,       \
             char       filename[ MAX_PK_PATH ];   \
             UINT32_t   file_length;               \
    );                                             \
                                                   \
PkMsgDef( ChangedFileContents ,  0x4cdea40d,       \
             UINT16_t   piece_length;              \
             char       buffer[ MAX_PIECE_LEN ];   \
    );                                             \
                                                   \
PkMsgDef( ChangedFileDone     ,  0x652a7e6d, );    \
                                                   \
PkMsgDef( RemovedFileName     ,  0x17b753ad,       \
             char       filename[ MAX_PK_PATH ];   \
    );                                             \
                                                   \
PkMsgDef( SyncDone            ,  0x606b53a3, );


#endif /* __TRADE_H_ */
