/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#ifndef __ipipe_connector_H__
#define __ipipe_connector_H__

#include "fd_mgr.H"

#include "ipipe_factories.H"

// generic connector; calls the factory when a new connection is successful.
class ipipe_connector : public fd_interface {
    ipipe_new_connection * connection_factory;
public:
    ipipe_connector( struct sockaddr_in *, ipipe_new_connection * );

    /*virtual*/ ~ipipe_connector( void );
    /*virtual*/ rw_response write( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * rd, bool * wr );

    /* note virtual functions read, write_to_buf, and over_write_threshold
       are not implemented here. */
};

#endif /* __ipipe_connector_H__ */
