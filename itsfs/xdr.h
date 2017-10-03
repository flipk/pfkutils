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
