
#include "stdafx.h"
#include "BST.h"
#include "BST_MSG.h"

using namespace BST_BASE;
using namespace BST_MSG;

pk_msg_ext_hdr :: pk_msg_ext_hdr(UInt16 _type)
{
	magic.reg(this);
	type.reg(this);
	length.reg(this);
	checksum.reg(this);
	magic.v = MAGIC;
	type.v = _type;
}

//static
void
pk_msg_ext_hdr :: post_encode_set_checksum( BST_BUFSEG ^ buf, UInt32 checksum )
{
	BST_UINT32_t  val;
	val.v = checksum;
	val.bst_encode(BST_BUFSEG(buf->Array,buf->Offset+checksum_offset(),4));
}

//static
void
pk_msg_ext_hdr :: post_encode_set_len( BST_BUFSEG ^ buf )
{
	BST_UINT16_t  val;
	val.v = buf->Count;
	val.bst_encode(BST_BUFSEG(buf->Array,buf->Offset+length_offset(),2));
}

//static
UInt32
pk_msg_ext_hdr :: calc_checksum( BST_BASE::BST_BUFSEG ^ buf )
{
	UInt32 checksum = CHECKSUM_START;
	for (int i=0; i < buf->Count; i++)
		checksum = ((checksum << 5) + checksum) + (buf->Array[buf->Offset + i] + i);
	return checksum;
}

PK_Message_Ext_Manager :: PK_Message_Ext_Manager(
	PK_Message_Ext_Handler ^ _handler,
	PK_Message_Ext_Link ^ _link )
{
	handler = _handler;
	link = _link;
	sendbuf = gcnew BST_BUF(MAX_MSG_SIZE);
	rcvbuf = gcnew BST_BUF(MAX_MSG_SIZE);
	s = decoder_state::STATE_HEADER_HUNT_1;
	read_remaining = 0;
}

PK_Message_Ext_Manager :: ~PK_Message_Ext_Manager( void )
{
	delete sendbuf;
	delete rcvbuf;
}

bool
PK_Message_Ext_Manager :: send( pk_msg_ext ^ msg )
{
	bool retval = false;

	BST_BUFSEG ^ encoded_bytes = msg->bst_encode(BST_BUFSEG(sendbuf));
	if (encoded_bytes)
	{
		pk_msg_ext_hdr::post_encode_set_len(encoded_bytes);
		pk_msg_ext_hdr::post_encode_set_checksum(encoded_bytes, 0);
		UInt32 checksum = pk_msg_ext_hdr::calc_checksum(encoded_bytes);
		pk_msg_ext_hdr::post_encode_set_checksum(encoded_bytes, checksum);

		if (link->write(encoded_bytes))
			retval = true;
	}
	else
	{
		Console::WriteLine(L"failure encoding msg");
	}

	return retval;
}

inline UInt16
PK_Message_Ext_Manager :: get_byte( int ticks, bool beginning )
{
	if (rcvbufpos >= rcvbufsize)
	{
		if (beginning)
			rcvbufsize = rcvbufpos = 0;
		int bytes_read = link->read(
			BST_BUFSEG(
				rcvbuf,
				rcvbufpos,
				rcvbuf->Length - rcvbufsize
				));
		if (bytes_read <= 0)
			return 0xFFFF;
		rcvbufsize += bytes_read;
	}
	return rcvbuf[rcvbufpos++];
}

#define RECV_DEBUGGING 0

pk_msg_ext ^
PK_Message_Ext_Manager :: recv( int ticks )
{
	UInt16  byte;
	pk_msg_ext ^ ret = nullptr;

	while (1)
	{
#if RECV_DEBUGGING
		Console::WriteLine(L"mgr->read state {0} rcvbufpos {1} rcvbufsize {2} read_remaining {3}",
			s, rcvbufpos, rcvbufsize, read_remaining);
#endif
		switch (s)
		{
		case decoder_state::STATE_HEADER_HUNT_1:
			byte = get_byte(ticks,true);
			if (byte == 0xFFFF)
				return nullptr;
			if (byte == ((pk_msg_ext_hdr::MAGIC >> 24) & 0xFF))
				s = decoder_state::STATE_HEADER_HUNT_2;
			break;

		case decoder_state::STATE_HEADER_HUNT_2:
			byte = get_byte(ticks);
			if (byte == 0xFFFF)
				return nullptr;
			if (byte == ((pk_msg_ext_hdr::MAGIC >> 16) & 0xFF))
				s = decoder_state::STATE_HEADER_HUNT_3;
			else
				s = decoder_state::STATE_HEADER_HUNT_1;
			break;

		case decoder_state::STATE_HEADER_HUNT_3:
			byte = get_byte(ticks);
			if (byte == 0xFFFF)
				return nullptr;
			if (byte == ((pk_msg_ext_hdr::MAGIC >>  8) & 0xFF))
				s = decoder_state::STATE_HEADER_HUNT_4;
			else
				s = decoder_state::STATE_HEADER_HUNT_1;
			break;

		case decoder_state::STATE_HEADER_HUNT_4:
			byte = get_byte(ticks);
			if (byte == 0xFFFF)
				return nullptr;
			if (byte == ((pk_msg_ext_hdr::MAGIC >>  0) & 0xFF))
			{
				if (rcvbufpos != 4)
				{
// imagine currently in the rcvbuf:  (example)
//    5 bytes of junk
//    4 bytes of magic, read 32 total.
// so at this point, pos=9, rcvbufsize=32.
// thus, rcvbuf+pos-4 is the start of the magic, skipping the junk.
// the number of bytes to copy back to the beginning is
// rcvbufsize-pos+4, or 32-9+4=27.
// after moving back, new size is 27.
					int junksize = rcvbufpos-4; // also, magic start is here.
					Array::Copy(rcvbuf,junksize,rcvbuf,0,rcvbufsize-junksize);
					rcvbufsize -= junksize;
					rcvbufpos = 0;
				}
				s = decoder_state::STATE_TYPE_READ_1;
			}
			else
				s = decoder_state::STATE_HEADER_HUNT_1;
			break;

		case decoder_state::STATE_TYPE_READ_1:
			byte = get_byte(ticks);
			if (byte == 0xFFFF)
				return nullptr;
			s = decoder_state::STATE_TYPE_READ_2;
			break;

		case decoder_state::STATE_TYPE_READ_2:
			byte = get_byte(ticks);
			if (byte == 0xFFFF)
				return nullptr;
			else
			{
				BST_UINT16_t  type;
				type.bst_decode(BST_BUFSEG(rcvbuf,pk_msg_ext_hdr::type_offset(),2));
#if RECV_DEBUGGING
				Console::WriteLine(L"decoded type {0:X4}", type.v);
#endif
				ret = handler->make_msg(type.v);
#if RECV_DEBUGGING
				if (ret)
					Console::WriteLine(L"got msg body");
				else
					Console::WriteLine(L"did not get msg body");
#endif
				s = decoder_state::STATE_LEN_READ_1;
			}
			break;

		case decoder_state::STATE_LEN_READ_1:
			byte = get_byte(ticks);
			if (byte == 0xFFFF)
				return nullptr;
			s = decoder_state::STATE_LEN_READ_2;
			break;

		case decoder_state::STATE_LEN_READ_2:
			byte = get_byte(ticks);
			if (byte == 0xFFFF)
				return nullptr;
			else
			{
				BST_UINT16_t  len;
				len.bst_decode(BST_BUFSEG(rcvbuf,pk_msg_ext_hdr::length_offset(),2));
				read_remaining = len.v;
				read_remaining -= rcvbufpos;
#if RECV_DEBUGGING
				Console::WriteLine(L"decoded length {0}", len.v);
#endif
				s = decoder_state::STATE_READ_BODY;
			}
			break;

		case decoder_state::STATE_READ_BODY:
			byte = get_byte(ticks);
			if (byte == 0xFFFF)
				return nullptr;
			read_remaining--;
			if (read_remaining <= 0)
			{
				s = decoder_state::STATE_HEADER_HUNT_1;
				if (ret != nullptr)
				{
					BST_BUFSEG ^ msgseg = BST_BUFSEG(rcvbuf,0,rcvbufpos);
					if (ret->bst_decode(msgseg))
					{
						UInt32 rcvd_checksum, calced_checksum;

						pk_msg_ext_hdr::post_encode_set_checksum(msgseg,0);
						calced_checksum = pk_msg_ext_hdr::calc_checksum(msgseg);
						rcvd_checksum = ret->hdr.Checksum;

						if (calced_checksum == rcvd_checksum)
						{
#if RECV_DEBUGGING
							Console::WriteLine(L"valid checksum found!");
#endif
							if (rcvbufpos < rcvbufsize)
							{
								rcvbufsize -= rcvbufpos;
								Array::Copy(rcvbuf,rcvbufpos,rcvbuf,0,rcvbufsize);
								rcvbufpos = 0;
							}
							return ret;
						}
						// else
#if RECV_DEBUGGING
						Console::WriteLine(L"checksum mismatch {0,2:X2} != {1,2:X2}",
							calced_checksum, rcvd_checksum);
#endif
					}
#if RECV_DEBUGGING
					else
						Console::WriteLine(L"failure decoding msg");
#endif
				}
				if (rcvbufpos < rcvbufsize)
				{
					rcvbufsize -= rcvbufpos;
					Array::Copy(rcvbuf,rcvbufpos,rcvbuf,0,rcvbufsize);
					rcvbufpos = 0;
				}
			}
			break;
		}
	}

	return nullptr;
}
