
#pragma once

using namespace System;

namespace BST_MSG {

#ifdef  BUILDING_BST_DLL

	public ref class pk_msg_ext_hdr : public BST_BASE::BST {
		BST_BASE::BST_UINT32_t magic;
		BST_BASE::BST_UINT16_t type;
		BST_BASE::BST_UINT16_t length;
		BST_BASE::BST_UINT32_t checksum;
	public:
		static int type_offset(void) { return 4; }
		static int length_offset(void) { return 6; }
		static int checksum_offset(void) { return 8; }
		literal UInt32 MAGIC = 0x819b8300UL;
		literal UInt32 CHECKSUM_START = 0xc2db895cUL;
		pk_msg_ext_hdr(UInt16 _type);
		property bool Valid_magic { bool get(void) { return magic.v == MAGIC; }}
		property UInt16 Length {
			UInt16 get(void) { return length.v; }
			void set(UInt16 len) { length.v = len; }
		}
		property UInt16 Type { UInt16 get(void) { return type.v; }}
		property UInt32 Checksum {
			UInt32 get(void) { return checksum.v; }
			void set(UInt32 v) { checksum.v = v; }
		}
		static void post_encode_set_checksum( BST_BASE::BST_BUFSEG ^ buf,
			UInt32 checksum );
		static void post_encode_set_len( BST_BASE::BST_BUFSEG ^ buf );
		static UInt32 calc_checksum( BST_BASE::BST_BUFSEG ^ buf );
	};

	public ref class pk_msg_ext abstract : public BST_BASE::BST {
	public:
		pk_msg_ext_hdr  hdr;
		// ability to make a linked list?
		pk_msg_ext( UInt16 type ) : hdr(type) {
			hdr.reg(this);
		}
		virtual ~pk_msg_ext(void) { }
		virtual UInt16 get_TYPE(void) = 0;
	};

	public ref class PK_Message_Ext_Handler abstract {
	public:
		virtual ~PK_Message_Ext_Handler(void) { }
		virtual pk_msg_ext ^ make_msg( UInt16 type ) = 0;
	};

	public ref class PK_Message_Ext_Link abstract {
	protected:
		PK_Message_Ext_Handler ^ handler;
		bool connected;
	public:
		PK_Message_Ext_Link( PK_Message_Ext_Handler ^ _handler ) {
			connected = false;
			handler = _handler;
		}
		virtual ~PK_Message_Ext_Link( void ) { }
		property bool Connected { bool get(void) { return connected; }};
		virtual bool write( BST_BASE::BST_BUFSEG ^ buf ) = 0;
		virtual int read( BST_BASE::BST_BUFSEG ^ buf ) = 0;
	};

	public ref class PK_Message_Ext_Manager {
		BST_BASE::BST_BUF ^ sendbuf;
		BST_BASE::BST_BUF ^ rcvbuf;
		int rcvbufpos;
		int rcvbufsize;
		enum class decoder_state {
			STATE_HEADER_HUNT_1,
			STATE_HEADER_HUNT_2,
			STATE_HEADER_HUNT_3,
			STATE_HEADER_HUNT_4,
			STATE_TYPE_READ_1,
			STATE_TYPE_READ_2,
			STATE_LEN_READ_1,
			STATE_LEN_READ_2,
			STATE_READ_BODY
		};
		decoder_state s;
		int read_remaining; // needed?
		UInt16 get_byte(int ticks, bool beginning);
		UInt16 get_byte(int ticks) { return get_byte(ticks,false); }
	protected:
		PK_Message_Ext_Handler ^ handler;
		PK_Message_Ext_Link ^ link;
	public:
		literal int MAX_MSG_SIZE = 8192;
		PK_Message_Ext_Manager( PK_Message_Ext_Handler ^ _handler,
								PK_Message_Ext_Link ^ _link );
		~PK_Message_Ext_Manager( void );
		bool send( pk_msg_ext ^ msg );
		pk_msg_ext ^ recv(int ticks);
	};

#endif

	template <class T, int typeValue>
	public ref class pk_msg_ext_body : public pk_msg_ext {
	public:
		literal UInt16 TYPE = typeValue;
		pk_msg_ext_body( void ) : pk_msg_ext( TYPE ) {
			body.reg(this);
		}
		T body;
		virtual UInt16 get_TYPE(void) override { return TYPE; }
	};

#define PkMsgExtDef( classname, typevalue, bodytype ) \
	typedef pk_msg_ext_body<bodytype,typevalue> classname;

}
