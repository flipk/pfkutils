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

#define POOL_ALLOC_FILL_BYTE 0xdd

#include "types.H"

class memory_pool {
public:
    // exception classes for constructor
    class constructor_failed         { };
    class constructor_invalid_args   { };
    // alloc
    class out_of_buffers             { };
    // free
    class invalid_pointer            { };
    class already_free               { };

private:
    int size;
    int num_bufs;
    int free_bufs;
    int num_longs;
    UCHAR * memory;
    UINT *bitmap;
    bool valid_pointer( void * _addr ) {
        UINT addr = (UINT)_addr;
        UINT mem  = (UINT)memory;
        if ( addr < mem )
            return false;
        if ( addr > ( mem + ( num_bufs * size )))
            return false;
        if ((( addr - mem ) % size ) != 0 )
            return false;
        return true;
    }
    int bufnum( void * addr ) {
        return ((UINT)addr - (UINT)memory) / size;
    }
    int bufnum( int bitnum, int longnum ) {
        return (longnum * 32) + bitnum;
    }
    UCHAR * bufaddr( int bufnum ) {
        return memory + ( bufnum * size );
    }
    void longbit_number( int bufnum, int &bitnum, int &longnum ) {
        longnum = bufnum >> 5;
        bitnum  = bufnum & 31;
    }
public:
    memory_pool( int size, int num_bufs )
        throw ( constructor_failed, constructor_invalid_args );
    ~memory_pool( void );
    void * alloc( void ) throw ( out_of_buffers );
    void free( void * ) throw ( invalid_pointer, already_free );
    friend class memory_pools;
};

class memory_pools {
public:
    // exception classes for constructor
    class constructor_failed          { };
    class constructor_invalid_args    { };
    // alloc
    class out_of_buffers              { };
    class invalid_size                { };
    // free
    class invalid_pointer             { };
    class already_free                { };

private:
    int num_pools;
    memory_pool ** pools;
    // a qsort plugin
    static int cmp_entries( const void * _a, const void * _b ) {
        int * a = (int*) _a;
        int * b = (int*) _b;

        if ( *a > *b )
            return 1;
        if ( *a < *b ) 
            return -1;
        return 0;
    }
public:
    memory_pools( int num_pools, int * sizes, int * quantities )
        throw ( constructor_failed, constructor_invalid_args );
    ~memory_pools( void );

    bool available( int size ) throw ();
    void * alloc( int size ) throw ( out_of_buffers, invalid_size );
    void free( void * ) throw ( invalid_pointer, already_free );
};
