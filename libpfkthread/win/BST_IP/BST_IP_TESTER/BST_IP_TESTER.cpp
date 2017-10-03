
#include "stdafx.h"
#include "../BST_IP/BST.h"
#include "../BST_IP/BST_MSG.h"
#include "../BST_IP/BST_MSG_TCP.h"

#include "BST_IP_TESTER.h"

using namespace System;
using namespace System::Threading;

MyMessage ^ pfk_buildit(void)
{
	MyMessage    ^  msg = gcnew MyMessage;
	msg->body.one.v = 0x123456789abcdef0ULL;
	msg->body.two.v = 0xbcca;
#if 0
	msg->body.three.pointer = nullptr;
#else
	msg->body.three.pointer = gcnew MyType2;
	msg->body.three.pointer->four.resize(5);
	msg->body.three.pointer->four.binary[0] = 1;
	msg->body.three.pointer->four.binary[1] = 2;
	msg->body.three.pointer->four.binary[2] = 3;
	msg->body.three.pointer->four.binary[3] = 4;
	msg->body.three.pointer->four.binary[4] = 5;
	msg->body.three.pointer->five.str = L"HELLO";
#endif
	msg->body.six.alloc(4);
	msg->body.six.arr[0]->v = 5;
	msg->body.six.arr[1]->v = 4;
	msg->body.six.arr[2]->v = 6;
	msg->body.six.arr[3]->v = 7;
#if 0
	msg->body.seven.which.v = (int) MyType3::msgtype::NINE;
	msg->body.seven.nine.v = 0x5541;
#else
	msg->body.seven.which.v = (int) MyType3::msgtype::EIGHT;
	msg->body.seven.eight.str = L"CRAPOLA";
#endif

	return msg;
}

void pfk_printit(MyMessage ^ msg)
{
	if (msg->hdr.Valid_magic)
		Console::WriteLine(L"magic is valid");
	else
		Console::WriteLine(L"magic is NOT valid");

	Console::Write(L"one={0,16:X16} ", msg->body.one.v);
	Console::Write(L"two={0,4:X4} three={{", msg->body.two.v);
	if (msg->body.three.pointer == nullptr)
		Console::Write(L"NULL");
	else
	{
		Console::Write(L"four=");
		for (int i = 0; i < msg->body.three.pointer->four.dim; i++)
			Console::Write(L"{0,2:X2}", msg->body.three.pointer->four.binary[i]);
		Console::Write(L" five={0}", msg->body.three.pointer->five.str);
	}
	Console::Write(L"} six={");
	if (!msg->body.six.arr)
		Console::Write(L"NULL");
	else
	{
		for (int i=0; i < msg->body.six.arr->Length; i++)
			Console::Write(L"{0}{1}", msg->body.six.arr[i]->v,
				(i != (msg->body.six.arr->Length-1)) ? "," : "");
	}
	Console::Write(L"} seven={");
	switch (msg->body.seven.which.v)
	{
	case MyType3::msgtype::EIGHT:
		Console::Write(L"eight={0}", msg->body.seven.eight.str);
		break;
	case MyType3::msgtype::NINE:
		Console::Write(L"nine={0,2:X2}", msg->body.seven.nine.v);
		break;
	}
	Console::WriteLine(L"}");
}

public ref class Worker : PK_Message_Ext_Handler {
public:
	Worker(void) {
		ParameterizedThreadStart ^ threadDelegate =
			gcnew ParameterizedThreadStart(this, &Worker::entry);
		System::Threading::Thread ^ th = gcnew System::Threading::Thread(threadDelegate);
		th->Start(42);
	}
	~Worker(void) {
		Console::WriteLine(L"Worker thread is dead");
	}
	virtual pk_msg_ext ^ make_msg( UInt16 type ) override {
		switch (type)
		{
		case 4660:
			return gcnew MyMessage;
		}
		return nullptr;
	}
	void entry(System::Object ^ w) {
		Console::WriteLine(L"This is a worker: {0}", w);

		PK_Message_Ext_Link ^ tcplink =
//			gcnew PK_Message_Ext_Link_TCP(this, "blade", 2005);
			gcnew PK_Message_Ext_Link_TCP(this, 2005);

		if (!tcplink->Connected)
		{
			Console::WriteLine(L"connection failed");
		}
		else
		{
			PK_Message_Ext_Manager ^ mgr = gcnew PK_Message_Ext_Manager(this,tcplink);
#if 0
			MyMessage ^ msg = pfk_buildit();
			if (!mgr->send(msg))
			{
				Console::WriteLine(L"unable to send msg");
			}
			if (!mgr->send(msg))
			{
				Console::WriteLine(L"unable to send msg");
			}
			if (!mgr->send(msg))
			{
				Console::WriteLine(L"unable to send msg");
			}
			if (!mgr->send(msg))
			{
				Console::WriteLine(L"unable to send msg");
			}
#else
			while (true)
			{
				pk_msg_ext ^ msg = mgr->recv(999);
				if (!msg)
				{
					Console::WriteLine(L"unable to read msg");
					break;
				}
				else
				{
					switch(msg->get_TYPE())
					{
					case MyMessage::TYPE:
						{
							MyMessage ^ mym = safe_cast<MyMessage^>(msg);
							pfk_printit(mym);
							break;
						}
					default:
						Console::WriteLine(L"unknown msg type {0:X8}", msg->get_TYPE());
					}
				}
			}
#endif
		}
		delete tcplink;

		delete this;
	}
};

int main(array<String ^> ^args)
{
	gcnew Worker();

	Console::WriteLine(L"press any key to exit");
	Console::ReadKey();

    return 0;
}
