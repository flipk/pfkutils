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

#include "keys.H"
#include "pick.H"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

#define BYE() printf("\r\n\r\nAccess Denied\r\n\r\n")

bool
pfk_key_pick( bool verbose )
{
    PfkKeyPairs pairs;
    PfkKeyPair p;
    char buf[512];
    char * bufptr;
    int remaining, cc, i;
    char * resp;
    fd_set rfds;
    struct timeval tv;
    bool done;

    if (pairs.pick_key( &p ) == false)
    {
        printf("\r\n\r\nNo more keys.\r\n");
        return false;
    }

    printf("\r\n\r\nchallenge: %s\r\n\r\n", p.challenge);

    bufptr = buf;
    remaining = sizeof(buf)-1;
    memset(buf,0,sizeof(buf));
    done = false;

    while (!done && remaining > 0)
    {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);

        tv.tv_sec = 10;
        tv.tv_usec = 0;

        if (select(1,&rfds,NULL,NULL,&tv) <= 0)
        {
            if (verbose)
                printf("\r\nGoodbye\r\n");
            else
                BYE();
            return false;
        }

        cc = read(0, bufptr, remaining);
        if (cc <= 0)
        {
            if (verbose)
                printf("\r\nerror reading response\r\n\r\n");
            else
                BYE();
            return false;
        }
        for (i=0; i < cc; i++)
            if (bufptr[i] == '\r' || bufptr[i] == '\n')
            {
                done = true;
                break;
            }
        remaining -= cc;
        bufptr += cc;
    }

    buf[sizeof(buf)-1] = 0;

    resp = strstr(buf, "response: ");
    if (!resp)
    {
        if (verbose)
            printf("\r\n\r\nResponse header not found.\r\n\r\n");
        else
            BYE();
        return false;
    }

    if (strlen(resp+10) < (PFK_KEYLEN-1))
    {
        if (verbose)
            printf("\r\n\r\nResponse header incomplete (%d)\r\n\r\n",
                   (int)strlen(resp+10));
        else
            BYE();
        return false;
    }

    if (memcmp(resp+10, p.response, PFK_KEYLEN-1) != 0)
    {
        BYE();
        return false;
    }

    return true;
}
