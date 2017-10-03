/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "pk_threads.H"
#include "pk_messages.H"
#include "pk_messages_ext.H"

#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

PK_Message_Ext_Manager :: PK_Message_Ext_Manager(
    PK_Message_Ext_Handler * _handler,
    PK_Message_Ext_Link * _link )
{
    handler = _handler;
    link = _link;
    rcvbufsize = 0;
    s = STATE_HEADER_HUNT_1;
    rcvbufpos = 0;
    read_remaining = 0;
}

PK_Message_Ext_Manager :: ~PK_Message_Ext_Manager( void )
{
}

bool
PK_Message_Ext_Manager :: send( pk_msg_ext * msg )
{
    bool retval = false;
    UINT32 checksum = 0;
    int  buflen = MAX_MSG_SIZE;

    if (msg->bst_encode( sendbuf, &buflen ))
    {
        pk_msg_ext_hdr::post_encode_set_len(sendbuf, buflen);
        pk_msg_ext_hdr::post_encode_set_checksum(sendbuf, 0);
        checksum = pk_msg_ext_hdr::calc_checksum(sendbuf, buflen);
        pk_msg_ext_hdr::post_encode_set_checksum(sendbuf, checksum);

        if (link->write(sendbuf, buflen))
            retval = true;
    }
    else
        printf("failure encoding msg\n");

    delete msg;
    return retval;
}

inline UINT16
PK_Message_Ext_Manager :: get_byte( int ticks )
{
    if (rcvbufpos >= rcvbufsize)
    {
        rcvbufsize = rcvbufpos = 0;
        int cc = link->read(rcvbuf, MAX_MSG_SIZE, ticks);
        if (cc == 0)
            return 0xFFFF;
        if (cc < 0)
        {
            int err = errno;
            printf("get_byte: read: %s\n", strerror(errno));
            handler->connection_lost(err);
            return 0xFFFF;
        }
        rcvbufsize = cc;
    }

    return (UINT16) rcvbuf[rcvbufpos++];
}

pk_msg_ext *
PK_Message_Ext_Manager :: recv( int ticks )
{
    UINT16 byte;
    UINT16_t * len;
    UINT16_t * type;
    pk_msg_ext * ret = NULL;

    while (1)
    {
#if 0
        printf("state=%d rcvbufsize=%d rcvbufpos=%d read_remaining=%d\n",
               s, rcvbufsize, rcvbufpos, read_remaining);
#endif

        switch (s)
        {
        case STATE_HEADER_HUNT_1:
            byte = get_byte(ticks);
            if (byte == 0xFFFF)
                goto bail;
            if (byte == ((pk_msg_ext_hdr::MAGIC >> 24) & 0xFF))
                s = STATE_HEADER_HUNT_2;
            break;

        case STATE_HEADER_HUNT_2:
            byte = get_byte(ticks);
            if (byte == 0xFFFF)
                goto bail;
            if (byte == ((pk_msg_ext_hdr::MAGIC >> 16) & 0xFF))
                s = STATE_HEADER_HUNT_3;
            else
                s = STATE_HEADER_HUNT_1;
            break;

        case STATE_HEADER_HUNT_3:
            byte = get_byte(ticks);
            if (byte == 0xFFFF)
                goto bail;
            if (byte == ((pk_msg_ext_hdr::MAGIC >> 8) & 0xFF))
                s = STATE_HEADER_HUNT_4;
            else
                s = STATE_HEADER_HUNT_1;
            break;

        case STATE_HEADER_HUNT_4:
            byte = get_byte(ticks);
            if (byte == 0xFFFF)
                goto bail;
            if (byte == (pk_msg_ext_hdr::MAGIC & 0xFF))
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
                    memmove(rcvbuf, rcvbuf+junksize, rcvbufsize-junksize);
                    rcvbufsize -= junksize;
                    rcvbufpos = 0;
                }
                s = STATE_TYPE_READ_1;
            }
            else
                s = STATE_HEADER_HUNT_1;
            break;

        case STATE_TYPE_READ_1:
            byte = get_byte(ticks);
            if (byte == 0xFFFF)
                goto bail;
            s = STATE_TYPE_READ_2;
            break;

        case STATE_TYPE_READ_2:
            byte = get_byte(ticks);
            if (byte == 0xFFFF)
                goto bail;
            // now we have all the type.
            type = (UINT16_t *) (rcvbuf + rcvbufpos - 2);
            ret = handler->make_msg(type->get());
            s = STATE_LEN_READ_1;
            break;

        case STATE_LEN_READ_1:
            byte = get_byte(ticks);
            if (byte == 0xFFFF)
                goto bail;
            s = STATE_LEN_READ_2;
            break;

        case STATE_LEN_READ_2:
            byte = get_byte(ticks);
            if (byte == 0xFFFF)
                goto bail;
            len = (UINT16_t *) (rcvbuf + rcvbufpos - 2);
            read_remaining = len->get();
            read_remaining -= rcvbufpos; // accound for what's read already
            s = STATE_READ_BODY;
            break;

        case STATE_READ_BODY:
            // there is optimization possible here.
            byte = get_byte(ticks);
            if (byte == 0xFFFF)
                goto bail;
            read_remaining--;
            if (read_remaining == 0)
            {
                s = STATE_HEADER_HUNT_1;
                if (ret)
                {
                    if (ret->bst_decode(rcvbuf, rcvbufpos))
                    {
                        UINT32 rcvd_checksum, calced_checksum;

                        pk_msg_ext_hdr::post_encode_set_checksum(
                            rcvbuf, 0);
                        calced_checksum = pk_msg_ext_hdr::calc_checksum(
                            rcvbuf, rcvbufpos);
                        rcvd_checksum = ret->hdr.get_checksum();

                        if (calced_checksum == rcvd_checksum)
                        {
                            if (rcvbufpos < rcvbufsize)
                            {
                                rcvbufsize -= rcvbufpos;
                                memmove(rcvbuf, rcvbuf+rcvbufpos, rcvbufsize);
                                rcvbufpos = 0;
                            }
                            return ret;
                        }

                        // else
                        printf("checksum validation failure! (%#x != %#x)\n",
                               calced_checksum, rcvd_checksum);
                    }
                    else
                        printf("failure decoding msg\n");
                }
                if (rcvbufpos < rcvbufsize)
                {
                    rcvbufsize -= rcvbufpos;
                    memmove(rcvbuf, rcvbuf+rcvbufpos, rcvbufsize);
                    rcvbufpos = 0;
                }
            }
            break;
        }
    }

bail:
    if (ret)
        delete ret;

    return NULL;
}
