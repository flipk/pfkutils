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

#ifndef __XDR_H_
#define __XDR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mytypes.h"

typedef struct {
    int encode_decode;
    uchar * data;
    int position;
    int bytes_left;
} XDR;

#define XDR_ENCODE 1
#define XDR_DECODE 2
#define XDR_FREE   3

typedef bool_t (*xdrproc_t)(XDR *,...);

typedef struct {
    enum_t val;
    xdrproc_t proc;
} xdrunion_table_t;

bool_t myxdr_void      ( XDR * xdr );
bool_t myxdr_bytes     ( XDR * xdr,
                         uchar ** data, u_int * data_len, u_int max_data );
bool_t myxdr_opaque    ( XDR * xdr,
                         uchar * data, u_int size );
bool_t myxdr_reference ( XDR * xdr,
                         uchar ** data, u_int size, xdrproc_t method );
bool_t myxdr_pointer   ( XDR * xdr,
                         uchar ** data, u_int size, xdrproc_t method );
bool_t myxdr_string    ( XDR * xdr,
                         uchar ** str, u_int max );
bool_t myxdr_u_int     ( XDR * xdr,
                         u_int * v );
bool_t myxdr_int       ( XDR * xdr,
                         int * v );
bool_t myxdr_enum      ( XDR * xdr,
                         enum_t * v );
bool_t myxdr_bool      ( XDR * xdr,
                         bool_t * v );
bool_t myxdr_union     ( XDR * xdr,
                         enum_t * disc, uchar * union_ptr,
                         xdrunion_table_t * tab, xdrproc_t def );

#ifdef __cplusplus
}
#endif

#endif /* __XDR_H_ */
