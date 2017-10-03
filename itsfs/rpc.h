/*
 * This code is originally written by Phillip F Knaack.
 * There are absolutely no restrictions on the use of this code.  It is
 * NOT GPL!  If you use this code in a product, you do NOT have to release
 * your alterations, nor do you have to acknowledge in any way that you
 * are using this software.
 * The only thing I ask is this: do not claim you wrote it.  My name
 * should appear in this file forever as the original author, and every
 * person who modifies this file after me should also place their names
 * in this file (so those that follow know who broke what).
 * This restriction is not enforced in any way by any license terms, it
 * is merely a personal request from me to you.  If you wanted, you could
 * even completely remove this entire comment and place a new one with your
 * company's confidential proprietary license on it-- but if you are a good
 * internet citizen, you will comply with my request out of the goodness
 * of your heart.
 * If you do use this code and you make a buttload of money off of it,
 * I would appreciate a little kickback-- but again this is a personal
 * request and is not required by any licensing of this code.  (Of course
 * in the offchance that anyone every actually DOES make a lot of money 
 * using this code, I know I'm going to regret that statement, but at
 * this point in time it feels like the right thing to say.)
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
