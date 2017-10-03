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

#include <stdio.h>
#include <string.h>

#include "filehandle.H"

bool
FileHandle :: decode( encrypt_iface * crypt, nfs_fh *buffer )
{
    if ( crypt )
        crypt->decrypt( (UCHAR*)this, buffer->data, FH_SIZE );
    else
        memcpy( (UCHAR*)this, buffer->data, FH_SIZE );
    return valid();
}

void
FileHandle :: encode( encrypt_iface * crypt, nfs_fh *buffer )
{
    magic.set( MAGIC );
    checksum.set( calc_checksum() );
    if ( crypt )
        crypt->encrypt( buffer->data, (UCHAR*)this, FH_SIZE );
    else
        memcpy( buffer->data, (UCHAR*)this, FH_SIZE );
}

UINT32
FileHandle :: calc_checksum( void )
{
    UINT32 old_checksum, sum;
    int count;
    uchar * ptr;

    old_checksum = checksum.get();
    checksum.set( 0 );

    sum = 0;
    ptr = (uchar*)this;

    for ( count = 0; count < FH_SIZE; count++ )
    {
        sum += *ptr++;
    }

    checksum.set( old_checksum );

    return sum + SUM_CONSTANT;
}
