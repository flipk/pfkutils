
#include "pk_tcp_msg.H"

PkTcpMsgDef( Config_Add_Listen_Port,
             0x78683374,
             UINT16_t port_number;
    );
    
PkTcpMsgDef( Config_Delete_Listen_Port,
             0x6004a65f,
             UINT16_t port_number;
    );

PkTcpMsgDef( Config_Display_Listen_Ports,
             0x7c245a82,
             /*nothing*/;
    );

PkTcpMsgDef( Config_Display_Proxy_Ports,
             0xe8635033,
             /*nothing*/;
    );
