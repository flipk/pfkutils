
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>

#include "pk_msg.H"

enum states {
    HUNT1,
    HUNT2,
    HUNT3,
    HUNT4,
    HEADER,
    BODY
};

pk_msgr :: pk_msgr( int _max_msg_len )
{
    max_msg_len = _max_msg_len;
    state = HUNT1;
    msg = new MaxPkMsgType;
    outbuf = (UINT8*)msg;
}

pk_msgr :: ~pk_msgr( void )
{
    delete msg;
}

bool
pk_msgr :: send( pk_msg * m )
{
    m->set_checksum();
    int l = m->get_len();
    char * p = m->get_ptr();
    while ( l > 0 )
    {
        int cc = writer( p, l );
        if ( cc <= 0 )
            return false;
        p += cc;
        l -= cc;
    }
    return true;
}

void
pk_msgr :: process_read( UINT8 * buf, int buflen )
{
    int tocopy;
    while (buflen > 0)
    {
        switch (state)
        {
        case HUNT1:
            if (*buf == pk_msg::magic_1)
                state = HUNT2;
            buf++;
            buflen--;
            break;

        case HUNT2:
            if (*buf == pk_msg::magic_2)
                state = HUNT3;
            else
                state = HUNT1;
            buf++;
            buflen--;
            break;

        case HUNT3:
            if (*buf == pk_msg::magic_3)
                state = HUNT4;
            else
                state = HUNT1;
            buf++;
            buflen--;
            break;

        case HUNT4:
            if (*buf == pk_msg::magic_4)
            {
                state = HEADER;
                msg->magic.set(pk_msg::magic_value);
                outbuf = (UINT8*)msg;
                outbuf += 4;
                stateleft = sizeof(pk_msg) - 4;
            }
            else
                state = HUNT1;
            buf++;
            buflen--;
            break;

        case HEADER:
            tocopy = buflen;
            if (tocopy > stateleft)
                tocopy = stateleft;

            if (outbuf != buf)
                memmove(outbuf, buf, tocopy);
#if 0 // theorize that the read(int) method will use this
            else
                printf("you were right about the header optimization\n");
#endif

            outbuf += tocopy;
            buf += tocopy;
            stateleft -= tocopy;
            buflen -= tocopy;

            if (stateleft == 0)
            {
                if (msg->get_len() > MAX_PK_MSG_LENGTH)
                {
                    printf("pk_msgr::process_read: bogus length %d!\n",
                           msg->get_len());
                    outbuf = (UINT8*)msg;
                    state = HUNT1;
                }
                else
                {
                    state = BODY;
                    stateleft = msg->get_len() - sizeof(pk_msg);
                }
            }
            break;

        case BODY:
            tocopy = buflen;
            if (tocopy > stateleft)
                tocopy = stateleft;

            if (outbuf != buf)
                memmove(outbuf, buf, tocopy);
#if 0 // theory
            else
                printf("you were right about the body optimization\n");
#endif

            outbuf += tocopy;
            buf += tocopy;
            stateleft -= tocopy;
            buflen -= tocopy;

            if (stateleft == 0)
            {
                if (msg->verif_checksum())
                {
                    recv(msg);
                    msg = new MaxPkMsgType;
                }
                else
                {
                    printf("pk_msgr::process_read: "
                           "bogus checksum %#x != %#x\n",
                           msg->checksum.get(), msg->calc_checksum());
                }
                outbuf = (UINT8*)msg;
                state = HUNT1;
            }
            break;
        }
    }
}

int
pk_msgr :: process_read( int fd )
{
    int pos = outbuf - (UINT8*)msg;
    int max = sizeof(MaxPkMsgType) - pos;
    int cc = ::read(fd, (char*)outbuf, max);
    if (cc > 0)
        process_read(outbuf,cc);
    return cc;
}
