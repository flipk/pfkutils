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

#ifndef __FILEHANDLE_H_
#define __FILEHANDLE_H_

#include <string.h>
#include "types.H"
#include "nfs_prot.h"
#include "config.h"
#ifdef USE_CRYPT
#include "encrypt_iface.H"
#else
class encrypt_iface;
#endif

#define MagicNumbers_Filehandle_magic          0xa62ff4e6
#define MagicNumbers_Filehandle_sum_constant   0x10f379cc

// an nfs filehandle is 32 bytes of data.
// this is a class which will produce and
// decode the contents of filehandles.

class FileHandle {
private:
    static const int FH_SIZE = 32;
    static const unsigned int MAGIC = MagicNumbers_Filehandle_magic;
    static const int SUM_CONSTANT = MagicNumbers_Filehandle_sum_constant;
    UINT32 calc_checksum( void );
public:
    FileHandle( void )
        { memset( (void*)this, 0, FH_SIZE ); }
    bool decode( encrypt_iface * crypt, nfs_fh *buffer );
    void encode( encrypt_iface * crypt, nfs_fh *buffer );

    bool valid( void )
        {
            if ( magic.get() != MAGIC )
            {
                printf( "FileHandle::valid: invalid magic\n" );
                return false;
            }
            if ( calc_checksum() != checksum.get() )
            {
                printf( "FileHandle::valid: invalid checksum\n" );
                return false;
            }
            return true;
        }
    int size_of( void )
        { return FH_SIZE; }

private:
    UINT32_t magic;        // offset 0
    UINT32_t checksum;     // 4
public:                
    UINT32_t userid;       // 8
    UINT32_t tree_id;      // 12
    UINT32_t file_id;      // 16
    UINT32_t remote_ip;    // 20
    UINT32_t remote_port;  // 24
    UINT32_t mount_id;     // 28
    // offset 32
};

#undef  UINT_T

#endif /* __FILEHANDLE_H_ */
