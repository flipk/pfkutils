/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "pk_threads.h"
#include "pk_messages.h"
#include "pk_messages_ext.h"

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
    uint32_t checksum = 0;
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

inline uint16_t
PK_Message_Ext_Manager :: get_byte( int ticks, bool beginning )
{
    if (rcvbufpos >= rcvbufsize)
    {
        if (beginning)
            rcvbufsize = rcvbufpos = 0;
        int bytes_read = link->read(
            rcvbuf, MAX_MSG_SIZE - rcvbufsize, ticks);
        if (bytes_read == 0)
            return 0xFFFF;
        if (bytes_read < 0)
        {
            int err = errno;
            printf("get_byte: read: %s\n", strerror(err));
            return 0xFFFF;
        }
        rcvbufsize += bytes_read;
    }

    return (uint16_t) rcvbuf[rcvbufpos++];
}

pk_msg_ext *
PK_Message_Ext_Manager :: recv( int ticks )
{
    uint16_t byte;
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
            byte = get_byte(ticks, true);
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
                        uint32_t rcvd_checksum, calced_checksum;

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
