
#ifndef __CONFIG_MESSAGES_H__
#define __CONFIG_MESSAGES_H__

#include "pk_tcp_msg.H"
#include "magic_numbers.H"

// always ensure 'sequence_number' is first field of every 
// inbound config message.

PkTcpMsgDef2( Config_Add_Listen_Port,
              Config_Add_Listen_Port_Magic,
              UINT32_t sequence_number;
              UINT16_t port_number;
              UINT32_t remote_host;
              UINT16_t remote_port;,
              ( UINT32 s, UINT16 p, UINT32 rh, UINT16 rp ),
              COMMA sequence_number(s)
              COMMA port_number(p)
              COMMA remote_host(rh)
              COMMA remote_port(rp) { }
    );
PkTcpMsgDef2( Config_Add_Listen_Port_Reply,
              Config_Add_Listen_Port_Reply_Magic,
              UINT32_t sequence_number;,
              ( UINT32 s ),
              COMMA sequence_number(s) { }
    );
    
PkTcpMsgDef2( Config_Delete_Listen_Port,
              Config_Delete_Listen_Port_Magic,
              UINT32_t sequence_number;
              UINT16_t port_number;,
              ( UINT32 s, UINT16 p ),
              COMMA sequence_number(s)
              COMMA port_number(p) { }
    );
PkTcpMsgDef2( Config_Delete_Listen_Port_Reply,
              Config_Delete_Listen_Port_Reply_Magic,
              UINT32_t sequence_number;,
              ( UINT32 s ),
              COMMA sequence_number(s) { }
    );

PkTcpMsgDef2( Config_Display_Listen_Ports,
              Config_Display_Listen_Ports_Magic,
              UINT32_t sequence_number;,
              ( UINT32 s ),
              COMMA sequence_number(s) { }
    );

PkTcpMsgDef2( Config_Display_Proxy_Ports,
              Config_Display_Proxy_Ports_Magic,
              UINT32_t sequence_number;,
              ( UINT32 s ),
              COMMA sequence_number(s) { }
    );

#define LARGEST_CONFIG_MSG_SIZE \
    MAX(sizeof(Config_Add_Listen_Port), \
    MAX(sizeof(Config_Add_Listen_Port_Reply), \
    MAX(sizeof(Config_Delete_Listen_Port), \
    MAX(sizeof(Config_Delete_Listen_Port_Reply), \
    MAX(sizeof(Config_Display_Listen_Ports), \
    sizeof(Config_Display_Proxy_Ports))))))

#endif /* __CONFIG_MESSAGES_H__ */
