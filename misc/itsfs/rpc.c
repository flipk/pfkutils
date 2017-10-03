
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "rpc.h"

bool_t
myxdr_rpccallmsg( XDR * xdr, rpccallmsg * call )
{
    if ( !myxdr_u_int( xdr, &call->xid ))
        return FALSE;
    if ( !myxdr_enum( xdr, &call->dir ))
        return FALSE;
    if ( call->dir != RPC_CALL_MSG )
        return FALSE;
    if ( !myxdr_u_int( xdr, &call->rpcvers ))
        return FALSE;
    if ( !myxdr_u_int( xdr, &call->program ))
        return FALSE;
    if ( !myxdr_u_int( xdr, &call->version ))
        return FALSE;
    if ( !myxdr_u_int( xdr, &call->procedure ))
        return FALSE;
    if ( !myxdr_u_int( xdr, &call->cred_flavor ))
        return FALSE;
    if ( !myxdr_bytes( xdr, (uchar**)&call->cred_bytes,
                     &call->cred_length, MAX_AUTH_BYTES ))
        return FALSE;
    if ( !myxdr_u_int( xdr, &call->verf_flavor ))
        return FALSE;
    if ( !myxdr_bytes( xdr, (uchar**)&call->verf_bytes,
                     &call->verf_length, MAX_AUTH_BYTES ))
        return FALSE;
    return TRUE;
}

bool_t
myxdr_rpcreplymsg( XDR * xdr, rpcreplymsg * reply )
{
    if ( !myxdr_u_int( xdr, &reply->xid ))
        return FALSE;
    if ( !myxdr_enum( xdr, &reply->dir ))
        return FALSE;
    if ( reply->dir != RPC_REPLY_MSG )
        return FALSE;
    if ( !myxdr_enum( xdr, &reply->stat ))
        return FALSE;
    if ( !myxdr_enum( xdr, &reply->verf_flavor ))
        return FALSE;
    if ( !myxdr_bytes( xdr, (uchar**)&reply->verf_base,
                     &reply->verf_length, MAX_AUTH_BYTES ))
        return FALSE;
    if ( !myxdr_enum( xdr, &reply->verf_stat ))
        return FALSE;
    return TRUE;
}
