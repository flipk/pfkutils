
#include <netinet/in.h>

#include "ipipe_factories.H"
#include "ipipe_forwarder.H"
#include "ipipe_connector.H"
#include "ipipe_stats.H"

//////////// ipipe_forwarder_factory

ipipe_forwarder_factory :: ipipe_forwarder_factory
( bool _dowuncomp, bool _dowcomp, ipipe_rollover * _rollover,
  bool _outdisc, bool _inrand )
{
    dowuncomp  = _dowuncomp ;
    dowcomp    = _dowcomp   ;
    rollover   = _rollover  ;
    outdisc    = _outdisc   ;
    inrand     = _inrand    ;
}

//virtual
ipipe_new_connection :: new_conn_response
ipipe_forwarder_factory :: new_conn( fd_mgr * mgr, int new_fd )
{
    ipipe_forwarder * ifn, * if0, * if1;

    //  fd      read   write  w_uncomp   w_comp   rollover
    //  outdisc  inrand
    if0 = new ipipe_forwarder( 
        0,      true,  false, false,     false,   NULL,
        false,   inrand );
    ifn = new ipipe_forwarder(
        new_fd, true,  true,  false,     dowcomp, NULL,
        false,   false  );
    if1 = new ipipe_forwarder(
        1,      false, true,  dowuncomp, false,   rollover,
        outdisc, false  );

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

ipipe_proxy_factory :: ipipe_proxy_factory( struct sockaddr_in * _sa )
{
    sa = new sockaddr_in;
    *sa = *_sa;
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
    ipipe_new_connection * inc = new ipipe_proxy2_factory( new_fd );

    fd_interface * fdi = new ipipe_connector( sa, inc );

    mgr->register_fd( fdi );

    return CONN_CONTINUE;
}

//////////// ipipe_proxy2_factory

ipipe_proxy2_factory :: ipipe_proxy2_factory( int _fda )
{
    fda = _fda;
}

//virtual
ipipe_new_connection :: new_conn_response
ipipe_proxy2_factory :: new_conn( fd_mgr * mgr, int new_fd )
{
    ipipe_forwarder * ifa, * ifb;

    ifa = new ipipe_forwarder( fda,     true, true, false, false,
                               NULL, false, false );
    ifb = new ipipe_forwarder( new_fd,  true, true, false, false,
                               NULL, false, false );

    ifa->register_others(  ifb, ifb );
    ifb->register_others(  ifa, ifa );

    mgr->register_fd( ifa );
    mgr->register_fd( ifb );

    return CONN_DONE;
}
