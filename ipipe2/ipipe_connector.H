
#ifndef __ipipe_connector_H__
#define __ipipe_connector_H__

#include "fd_mgr.H"

#include "ipipe_factories.H"

// generic connector; calls the factory when a new connection is successful.
class ipipe_connector : public fd_interface {
    ipipe_new_connection * connection_factory;
public:
    ipipe_connector( struct sockaddr_in *, ipipe_new_connection * );
    virtual ~ipipe_connector( void );
    virtual bool read ( fd_mgr * );
    virtual bool write( fd_mgr * );
    virtual bool select_for_read( fd_mgr * );
    virtual bool select_for_write( fd_mgr * );
    virtual bool over_write_threshold( void );
    virtual bool write_to_fd( char * buf, int len );
};

#endif /* __ipipe_connector_H__ */
