/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*-  */

#include "fd_mgr.H"
#include <string>

class proxyClientTcpAcceptor : public fd_interface
{
    std::string proxy;
    std::string url;
    bool proxyConnect;
public:
    proxyClientTcpAcceptor(short listenPort, std::string _proxy,
                           bool _proxyConnect, std::string _url);
    /*virtual*/ ~proxyClientTcpAcceptor(void);
    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *mgr, bool * do_read, bool * do_write );
};
