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

#ifndef __ID_NAME_DB_H_
#define __ID_NAME_DB_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mytypes.h"
#include "ino_types.h"
#include "../bt/btree.H"

class id_name_db {
    Btree * bt;
    uchar * fetch_ret;
    int fetch_ret_len;
    char fname[ 64 ];

    static char * btdump_sprint( void * arg, int noderec,
                                 int keyrec, void * key, int keylen,
                                 int datrec, void * dat, int datlen );
    static void   btdump_sprintfree( void * arg, char * s );
    static void   btdump_print( void * arg, char * format, ... );

public:
    id_name_db( void );
    ~id_name_db( void );

    void add( int mount_id, uchar * path, inode_file_type ftype, int id );
    void del( int id );
    void purge_mount( int mount_id );
    // return null if not present
    uchar * fetch( int id, inode_file_type &ftype );
    // return -1 if not present
    int fetch( int mount_id, uchar * path, inode_file_type &ftype );
    int alloc_id( void )
        {
            int id;
            inode_file_type dummy;
            while ( 1 )
            {
                id = random();
                if ( fetch( id, dummy ) == NULL )
                    return id;
            }
        }

    void flush( void ) { bt->flush(); }
};

#endif /* __ID_NAME_DB_H_ */
