/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#ifndef __RPC_H_
#define __RPC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "xdr.h"

typedef struct {
    u_int xid;
#define RPC_CALL_MSG 0
#define RPC_REPLY_MSG 1
    enum_t dir;
    u_int rpcvers;
    u_int program;
    u_int version;
    u_int procedure;
    u_int cred_flavor;
    u_int cred_length;
#define MAX_AUTH_BYTES 400
    uchar * cred_bytes;
    u_int verf_flavor;
    u_int verf_length;
    uchar * verf_bytes;
} rpccallmsg;

typedef struct {
    u_int xid;
    enum_t dir;
#define RPC_MSG_ACCEPTED 0
#define RPC_MSG_DENIED   1
    enum_t stat;
    u_int verf_flavor;
    u_int verf_length;
    uchar * verf_base;
#define RPC_AUTH_OK           0
#define RPC_AUTH_BADCRED      1
#define RPC_AUTH_REJECTEDCRED 2
#define RPC_AUTH_BADVERF      3
#define RPC_AUTH_REJECTEDVERF 4
#define RPC_AUTH_TOOWEAK      5
#define RPC_AUTH_INVALIDRESP  6
#define RPC_AUTH_FAILED       7
    enum_t verf_stat;
} rpcreplymsg;

bool_t myxdr_rpccallmsg  ( XDR * xdr, rpccallmsg * call );
bool_t myxdr_rpcreplymsg ( XDR * xdr, rpcreplymsg * reply );

#ifdef __cplusplus
}
#endif

#endif /* __RPC_H_ */
