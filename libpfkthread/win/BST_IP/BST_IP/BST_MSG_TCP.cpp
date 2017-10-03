
#include "stdafx.h"
#include "BST.h"
#include "BST_MSG.h"
#include "BST_MSG_TCP.h"

using namespace BST_BASE;
using namespace BST_MSG;

PK_Message_Ext_Link_TCP :: PK_Message_Ext_Link_TCP(
	PK_Message_Ext_Handler ^ _handler,
	UInt16 port )
	: PK_Message_Ext_Link(_handler)
{
	Net::Sockets::TcpListener ^ listener;
	listener = gcnew Net::Sockets::TcpListener(Net::IPAddress::Any, port);
	listener->Start(1);
	client = listener->AcceptTcpClient();
	delete listener;
	connected = true;
}

PK_Message_Ext_Link_TCP :: PK_Message_Ext_Link_TCP(
	PK_Message_Ext_Handler ^ _handler,
	String ^ host,
	UInt16 port )
	: PK_Message_Ext_Link(_handler)
{
	try {
		client = gcnew Net::Sockets::TcpClient(host, port);
	}
	catch (...) {
		return;
	}
	connected = true;
}

PK_Message_Ext_Link_TCP :: ~PK_Message_Ext_Link_TCP( void )
{
	if (client)
		delete client;
}

//virtual
bool
PK_Message_Ext_Link_TCP :: write( BST_BUFSEG ^ buf )
{
	try {
		client->GetStream()->Write(buf->Array, buf->Offset, buf->Count);
	}
	catch (...) {
		connected = false;
		return false;
	}
	return true;
}

//virtual
int
PK_Message_Ext_Link_TCP :: read( BST_BUFSEG ^ buf )
{
	int cc;
	int ret = -1;

	try {
		cc = client->GetStream()->Read(buf->Array, buf->Offset, buf->Count);
		if (cc > 0)
		{
			ret = cc;
		}
		else
		{
			connected = false;
		}
	}
	catch (...) {
		connected = false;
	}

	return ret;
}
