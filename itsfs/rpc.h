/*
 * This code is written by Phillip F Knaack. This code is in the
 * public domain. Do anything you want with this code -- compile it,
 * run it, print it out, pass it around, shit on it -- anything you want,
 * except, don't claim you wrote it.  If you modify it, add your name to
 * this comment in the COPYRIGHT file and to this comment in every file
 * you change.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
