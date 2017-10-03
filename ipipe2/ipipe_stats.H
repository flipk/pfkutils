
#ifndef __ipipe_stats_H__
#define __ipipe_stats_H__

#include "fd_mgr.H"
#include "ipipe_forwarder.H"

void stats_init     ( bool _shortform, bool _verbose, bool _final,
                      bool is_proxy, struct timeval * tick_tv );
void stats_tick     ( void );
void stats_add_conn ( void );
void stats_del_conn ( void );
void stats_add      ( int r, int w );
void stats_reset    ( void );
void stats_done     ( void );

#endif /* __ipipe_stats_H__ */
