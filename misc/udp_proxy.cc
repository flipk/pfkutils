/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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

/*
   This program only works for one udp connection, as it has only one
   socket to use for forwarding, and the server side needs unique
   source addresses to distinguish "connections." to start more than
   one, use different port pairs for each.

     [server is on 2005]

     ./udp_proxy 2006 127.0.0.1 2005 <error> <latency>
     [first client connects to 2006]
     
     ./udp_proxy 2007 127.0.0.1 2005 <error> <latency>
     [second client connects to 2007]

   <error> is in percent, e.g. 0 == no errors, 10 == 10% of packets dropped
   <latency> is in ticks, at 10 ticks per second, e.g. 5 == half a second

   NOTE that even <latency> of 0 is not immediate, it has to wait for
   the next spin of the ticker wheel. don't be surprised by jitter in
   the delay time through this proxy. but that's a good test, no?
*/

#include "posix_fe.h"
#include "dll3.h"

#define VERBOSE 0

#define PRINTF(x...) // printf(x)

bool dropit(int percent_error);

struct proxy_buf;
typedef DLL3::List<proxy_buf,1,false,false> Proxy_Buf_List;

struct proxy_buf : public Proxy_Buf_List::Links {
    pxfe_string  buf;
    int remaining;
};

int
main(int argc, char ** argv)
{
    if (argc != 6)
    {
    usage:
        printf("usage:  udp_proxy listenport remoteip remoteport "
               "percent_error rtd_ticks\n");
        printf("  (note a tick rate of 10 ticks per second)\n");
        return 1;
    }

    short listenport = (short) atoi(argv[1]);
    short remoteport = (short) atoi(argv[3]);
    uint32_t remoteip = 0;
    int percent_error = atoi(argv[4]);
    int rtd_ticks = atoi(argv[5]);
    if (pxfe_iputils::hostname_to_ipaddr(argv[2], &remoteip) == false)
    {
        printf("not a valid ipdaddr\n");
        goto usage;
    }

    pxfe_errno  e;

    pxfe_udp_socket   sock_one;
    pxfe_udp_socket   sock_two;

    pxfe_sockaddr_in   sock_one_source;
    pxfe_sockaddr_in   sock_one_dest;

    sock_one_dest.init(remoteip, remoteport);

    if (sock_one.init(listenport, &e) == false)
    {
        printf("sock one init: %s\n", e.Format().c_str());
        return 1;
    }
    if (sock_two.init(&e) == false)
    {
        printf("sock two init: %s\n", e.Format().c_str());
        return 1;
    }

    Proxy_Buf_List  first_to_second;
    Proxy_Buf_List  second_to_first;

    pxfe_ticker  ticker;
    ticker.start(0,100000);
    while (1)
    {
        pxfe_select   sel;

        if (first_to_second.get_cnt() < 1000)
            sel.rfds.set(sock_one);
        if (second_to_first.get_cnt() < 1000)
            sel.rfds.set(sock_two);
        sel.rfds.set(ticker.fd());
        sel.tv.set(1,0);

        sel.select();

        proxy_buf * pb, * npb;

        if (sel.rfds.is_set(ticker.fd()))
        {
            if (ticker.doread() == false)
                break;

            PRINTF("tick: ");

            for (pb = first_to_second.get_head();
                 pb;
                 pb = npb)
            {
                npb = first_to_second.get_next(pb);
                PRINTF("1t2(%d) ", pb->remaining);
                pb->remaining --;
                if (pb->remaining <= 0)
                {
                    first_to_second.remove(pb);
                    if (sock_two.send(pb->buf, sock_one_dest, &e) == false)
                    {
                        PRINTF("sock two send: %s\n", e.Format().c_str());
                    }
                    delete pb;
                }
            }

            for (pb = second_to_first.get_head();
                 pb;
                 pb = npb)
            {
                npb = second_to_first.get_next(pb);
                pb->remaining --;
                PRINTF("2t1(%d) ", pb->remaining);
                if (pb->remaining <= 0)
                {
                    second_to_first.remove(pb);
                    if (sock_one.send(pb->buf, sock_one_source, &e) == false)
                    {
                        PRINTF("sock one send: %s\n", e.Format().c_str());
                    }
                    delete pb;
                }
            }

            PRINTF("\n");
        }
        if (sel.rfds.is_set(sock_one))
        {
            pb = new proxy_buf;
            pb->remaining = rtd_ticks;
            if (sock_one.recv(pb->buf, sock_one_source, &e) == false)
            {
                printf("sock one recv: %s\n", e.Format().c_str());
                delete pb;
            }
            else
            {
                PRINTF("got from one: %s\n", pb->buf.format_hex().c_str());
                if (dropit(percent_error))
                {
                    printf("DROPPING PACKET!!\n");
                    delete pb;
                }
                else
                {
                    first_to_second.add_tail(pb);
                }
            }
        }
        if (sel.rfds.is_set(sock_two))
        {
            pb = new proxy_buf;
            pb->remaining = rtd_ticks;
            pxfe_sockaddr_in dummy;
            if (sock_two.recv(pb->buf, dummy, &e) == false)
            {
                printf("sock two recv: %s\n", e.Format().c_str());
                delete pb;
            }
            else
            {
                PRINTF("got from two: %s\n", buffer.format_hex().c_str());
                if (dropit(percent_error))
                {
                    printf("DROPPING PACKET\n");
                    delete pb;
                }
                else
                {
                    second_to_first.add_tail(pb);
                }
            }
        }
    }

    return 0;
}

bool
dropit(int percent_error)
{
    int v = random() % 100;
    if (v < percent_error)
        return true;
    return false;
}
