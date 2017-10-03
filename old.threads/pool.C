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
#include <stdlib.h>
#include "pool.H"
#include "threads.H"

memory_pool::memory_pool( int _size, int _num_bufs )
    throw ( constructor_failed, constructor_invalid_args )
{
    if ( _size < 1 || _num_bufs < 1 )
        throw constructor_invalid_args();

    size = _size;
    num_bufs = _num_bufs;
    free_bufs = num_bufs;

    int num_bytes = size * num_bufs;

    num_longs = (num_bufs / 32) + 1;

    try
    {
        memory = NULL;
        memory = new UCHAR[ num_bytes ];
        bitmap = new  UINT[ num_longs ];
    }
    catch (...)
    {
        if ( memory != NULL )
            delete[] memory;
        throw constructor_failed();
    }

    memset( bitmap, 0xff, num_longs * 4 );
    memset( memory, 0, num_bytes );
}

void *
memory_pool::alloc( void ) throw ( out_of_buffers )
{
    int bitnum, longnum;

    if ( free_bufs == 0 )
    {
        throw out_of_buffers();
    }

    for ( longnum = 0; longnum < num_longs; longnum++ )
        if ( bitmap[longnum] != 0 )
            break;

    if ( longnum == num_longs )
    {
        th->printf( "memory_pool::alloc internal error\n" );
        // why did free_bufs not report it?
        throw out_of_buffers();
    }

    for ( bitnum = 0; bitnum < 32; bitnum++ )
        if ( bitmap[longnum] & ( 1 << bitnum ))
            break;

    bitmap[longnum] &= ~(1 << bitnum);
    free_bufs--;

    UCHAR * ret = bufaddr( bufnum( bitnum, longnum ));

#ifdef POOL_ALLOC_FILL_BYTE
    memset( ret, POOL_ALLOC_FILL_BYTE, size );
#endif

    return (void*) ret;
}

void
memory_pool::free( void *ptr ) throw ( invalid_pointer, already_free )
{
    int bitnum, longnum;
    if ( valid_pointer( ptr ) == false )
        throw invalid_pointer();

    longbit_number( bufnum( ptr ), bitnum, longnum );
    if ( bitmap[longnum] & ( 1 << bitnum ))
    {
        throw already_free();
    }

    bitmap[longnum] |= ( 1 << bitnum );
    free_bufs++;
}

memory_pool::~memory_pool( void )
{
    delete[] memory;
    delete[] bitmap;
}

memory_pools::memory_pools( int _num_pools, int * sizes, int * quantities )
    throw ( constructor_failed, constructor_invalid_args )
{
    num_pools = _num_pools;

    if ( num_pools < 1 )
        throw constructor_invalid_args();

    try
    {
        pools = new memory_pool*[ num_pools ];
    }
    catch (...)
    {
        throw constructor_failed();
    }

    int * sqs = new int[ num_pools * 2 ];

    // copy sizes & quantities into a temp array
    // so that we can sort the sizes in increasing 
    // order of size. then construct pools from this
    // sorted listing. (we don't want to modify the
    // user-supplied list, and if the list is in readonly
    // constant data segment we wouldn't be able to anyway.)

    int i;
    for ( i = 0; i < num_pools; i++ )
    {
        sqs[ i*2   ] = sizes[i];
        sqs[(i*2)+1] = quantities[i];
    }

    qsort( (void*)sqs, i, sizeof(int)*2, cmp_entries );

    enum { none, failed, invalid_args, unknown } err = none;

    try
    {
        for ( i = 0; i < num_pools; i++ )
            pools[i] = new memory_pool( sqs[ i*2   ],
                                        sqs[(i*2)+1] );
    }
    catch ( memory_pool :: constructor_failed )
    {
        err = failed;
    }
    catch ( memory_pool :: constructor_invalid_args )
    {
        err = invalid_args;
    }
    catch (...)
    {
        err = unknown;
    }

    delete[] sqs;

    if ( err != none )
    {
        for ( i--; i >= 0; i-- )
            delete pools[i];

        delete[] pools;

        switch ( err )
        {
        case failed:
            throw constructor_failed();

        case invalid_args:
            throw constructor_invalid_args();

        default:
            th->printf( "memory_pools: memory_pool constructor "
                        "threw unknown exception!\n" );
            throw;
        }
    }
}

memory_pools::~memory_pools( void )
{
    int i;

    for ( i = 0; i < num_pools; i++ )
        delete pools[i];

    delete[] pools;
}

void *
memory_pools::alloc( int size )
    throw ( out_of_buffers, invalid_size )
{
    if ( size >= pools[num_pools-1]->size )
        // the caller should've known better...
        throw invalid_size();

    int i;

    for ( i = 0; i < num_pools; i++ )
        if (( pools[i]->size >= size ) &&
            ( pools[i]->free_bufs > 0 ))
            break;

    if ( i == num_pools )
    {
        throw out_of_buffers();
    }

    void * ret = NULL;

    try
    {
        ret = pools[i]->alloc();
    }
    catch ( memory_pool :: out_of_buffers )
    {
        // internal error? why did the pool say there
        // was room but then fail?
        th->printf( "memory_pools::alloc internal error\n" );
        throw out_of_buffers();
    }

    return ret;
}

void
memory_pools::free( void *ptr ) throw ( invalid_pointer, already_free )
{
    int i;

    for ( i = 0; i < num_pools; i++ )
        if ( pools[i]->valid_pointer( ptr ))
            break;

    if ( i == num_pools )
        throw invalid_pointer();

    try
    {
        pools[i]->free( ptr );
    }
    catch ( memory_pool :: invalid_pointer )
    {
        throw invalid_pointer();
    }
    catch ( memory_pool :: already_free )
    {
        throw already_free();
    }
}


#ifdef POOL_MAIN
int
main()
{
    const int szs[]  = { 176, 200,   8 };
    const int qnts[] = { 100, 200, 500 };
    memory_pools * pools;

    try
    {
        pools = NULL;
        pools = new memory_pools( 2, (int*)szs, (int*)qnts );
    }
    catch ( memory_pools :: constructor_failed )
    {
        th->printf( "memory_pools constructor failed\n" );
    }
    catch ( memory_pools :: constructor_invalid_args )
    {
        th->printf( "memory_pools constructor invalid args\n" );
    }
    catch (...)
    {
        th->printf( "unknown exception\n" );
    }

    try
    {
        void * ptrs[10];
        int i;

        for ( i = 0; i < 10; i++ )
        {
            ptrs[i] = pools->alloc( 500 );
            th->printf( "got buf at %x\n", ptrs[i] );
        }

        for ( i = 0; i < 10; i++ )
            pools->free( ptrs[i] );

        pools->free( ptrs[0] );
    }
    catch (...)
    {
        th->printf( "caught exception\n" );
    }



    if ( pools != NULL )
        delete pools;

    return 0;
}
#endif
