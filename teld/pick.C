/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
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
                   strlen(resp+10));
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
