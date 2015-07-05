
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
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
