
#pragma once

using namespace System;

#include "BST.h"
#include "BST_MSG.h"

namespace BST_MSG {

#ifdef  BUILDING_BST_DLL

	public ref class PK_Message_Ext_Link_TCP : public PK_Message_Ext_Link {
		/// stream, not client
		Net::Sockets::TcpClient ^ client;
	public:
		PK_Message_Ext_Link_TCP( PK_Message_Ext_Handler ^ _handler, UInt16 port );
		PK_Message_Ext_Link_TCP( PK_Message_Ext_Handler ^ _handler,
								String ^ host, UInt16 port );
		~PK_Message_Ext_Link_TCP( void );
		virtual bool write( BST_BASE::BST_BUFSEG ^ buf ) override;
		virtual int read( BST_BASE::BST_BUFSEG ^ buf ) override;
	};

#endif

}
