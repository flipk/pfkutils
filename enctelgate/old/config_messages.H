
#ifndef __CONFIG_MESSAGES_H__
#define __CONFIG_MESSAGES_H__

#include "pk_tcp_msg.H"
#include "magic_numbers.H"

PkTcpMsgDef( Config_Add_Listen_Port,
             Config_Add_Listen_Port_Magic,
             UINT32_t sequence_number;
             UINT16_t port_number;
             UINT32_t remote_host;
             UINT16_t remote_port;
    );
PkTcpMsgDef( Config_Add_Listen_Port_Reply,
             Config_Add_Listen_Port_Reply_Magic,
             UINT32_t sequence_number;
    );
    
PkTcpMsgDef( Config_Delete_Listen_Port,
             Config_Delete_Listen_Port_Magic,
             UINT16_t port_number;
             UINT32_t sequence_number;
    );
PkTcpMsgDef( Config_Delete_Listen_Port_Reply,
             Config_Delete_Listen_Port_Reply_Magic,
             UINT32_t sequence_number;
    );

PkTcpMsgDef( Config_Display_Listen_Ports,
             Config_Display_Listen_Ports_Magic,
             /*nothing*/;
    );

PkTcpMsgDef( Config_Display_Proxy_Ports,
             Config_Display_Proxy_Ports_Magic,
             /*nothing*/;
    );

#define LARGEST_CONFIG_MSG_SIZE \
    MAX(sizeof(Config_Add_Listen_Port), \
    MAX(sizeof(Config_Add_Listen_Port_Reply), \
    MAX(sizeof(Config_Delete_Listen_Port), \
    MAX(sizeof(Config_Delete_Listen_Port_Reply), \
    MAX(sizeof(Config_Display_Listen_Ports), \
    sizeof(Config_Display_Proxy_Ports))))))

#endif /* __CONFIG_MESSAGES_H__ */
