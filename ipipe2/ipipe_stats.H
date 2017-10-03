
#ifndef __ipipe_stats_H__
#define __ipipe_stats_H__

#include "fd_mgr.H"
#include "ipipe_forwarder.H"

void stats_init     ( fd_mgr * mgr,
                      bool _shortform, bool _verbose, bool _final );
void stats_add_conn ( void );
void stats_del_conn ( void );
void stats_add      ( int r, int w );
void stats_reset    ( void );
void stats_done     ( void );

#endif /* __ipipe_stats_H__ */
