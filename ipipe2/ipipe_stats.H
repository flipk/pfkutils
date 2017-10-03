/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

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
