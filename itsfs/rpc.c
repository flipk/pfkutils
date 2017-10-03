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
