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

#include <netinet/in.h>

#include "ipipe_factories.h"
#include "ipipe_forwarder.h"
#include "ipipe_connector.h"
#include "ipipe_stats.h"

//////////// ipipe_forwarder_factory

ipipe_forwarder_factory :: ipipe_forwarder_factory
( bool _dowuncomp, bool _dowcomp, ipipe_rollover * _rollover,
  bool _outdisc, bool _inrand,
  int _pausing_bytes, int _pausing_delay )
{
    dowuncomp  = _dowuncomp ;
    dowcomp    = _dowcomp   ;
    rollover   = _rollover  ;
    outdisc    = _outdisc   ;
    inrand     = _inrand    ;
    pausing_bytes = _pausing_bytes;
    pausing_delay = _pausing_delay;
}

//virtual
ipipe_new_connection :: new_conn_response
ipipe_forwarder_factory :: new_conn( fd_mgr * mgr, int new_fd )
{
    ipipe_forwarder * ifn, * if0, * if1;

    //  fd      read   write  w_uncomp   w_comp   rollover
    //  outdisc  inrand  pausing_bytes  pausing_delay
    if0 = new ipipe_forwarder( 
        0,      true,  false, false,     false,   NULL,
        false,   inrand, pausing_bytes, pausing_delay );
    ifn = new ipipe_forwarder(
        new_fd, true,  true,  false,     dowcomp, NULL,
        false,   false , pausing_bytes, pausing_delay );
    if1 = new ipipe_forwarder(
        1,      false, true,  dowuncomp, false,   rollover,
        outdisc, false , pausing_bytes, pausing_delay );

    //                   writer reader
    if0->register_others(  ifn, NULL );
    ifn->register_others(  if1,  if0 );
    if1->register_others( NULL,  ifn );

    mgr->register_fd( if0 );
    mgr->register_fd( ifn );
    mgr->register_fd( if1 );

    stats_reset();

    return CONN_DONE;
}

//////////// ipipe_proxy_factory

ipipe_proxy_factory :: ipipe_proxy_factory( struct sockaddr_in * _sa,
                                            int pause_bytes, int pause_delay )
{
    sa = new sockaddr_in;
    *sa = *_sa;
    pausing_bytes = pause_bytes;
    pausing_delay = pause_delay;
}

//virtual
ipipe_proxy_factory :: ~ipipe_proxy_factory( void )
{
    delete sa;
}

//virtual
ipipe_new_connection :: new_conn_response
ipipe_proxy_factory :: new_conn( fd_mgr * mgr, int new_fd )
{
    ipipe_new_connection * inc = new ipipe_proxy2_factory( new_fd,
                                                           pausing_bytes,
                                                           pausing_delay );

    fd_interface * fdi = new ipipe_connector( sa, inc );

    mgr->register_fd( fdi );

    return CONN_CONTINUE;
}

//////////// ipipe_proxy2_factory

ipipe_proxy2_factory :: ipipe_proxy2_factory( int _fda,
                                              int pause_bytes, int pause_delay )
{
    fda = _fda;
    pausing_bytes = pause_bytes;
    pausing_delay = pause_delay;
}

//virtual
ipipe_new_connection :: new_conn_response
ipipe_proxy2_factory :: new_conn( fd_mgr * mgr, int new_fd )
{
    ipipe_forwarder * ifa, * ifb;

    ifa = new ipipe_forwarder( fda,     true, true, false, false,
                               NULL, false, false,
                               pausing_bytes, pausing_delay );
    ifb = new ipipe_forwarder( new_fd,  true, true, false, false,
                               NULL, false, false,
                               pausing_bytes, pausing_delay );

    ifa->register_others(  ifb, ifb );
    ifb->register_others(  ifa, ifa );

    mgr->register_fd( ifa );
    mgr->register_fd( ifb );

    return CONN_DONE;
}
