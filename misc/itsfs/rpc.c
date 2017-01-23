
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
