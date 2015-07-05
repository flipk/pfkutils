
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if 0
set -e -x
ccppc -O0 -I /usr/test/bssdata2/tornado3/mcp750.0/target/h -c redmalloc.C -DCPU=PPC604
exit 0
#endif

#include <string.h>
#include <stdio.h>
#include <intLib.h>
#include <taskLib.h>
#include <tickLib.h>

#include "polling_print.C"

#include "redmalloc_internal.H"
#include "redmalloc.H"


int red_malloc_initialized = 0;
int red_malloc_multitasking = 0;

#define used_list  pk_memory_used_list
#define free_list  pk_memory_free_list
#define array_list pk_memory_array_list

pkiub_list         * used_list;
pkiub_list         * free_list;
pkiub_array_list   * array_list;

// the very first 'malloc' call makes this code run below.
// note that this is before process scheduling is up, so we can't
// create the audit task here.

// note also that what we're constructing here are dll2 lists, 
// and the redmalloc_dll2 implementation overloads the 'new' operators
// to use _real_malloc instead of malloc, so that these 'new' calls will
// not recurse!

void
redmallocinit( void )
{
    static int recurse_protect = 0;

    if ( recurse_protect == 1 )
    {
        pprintf( "redmallocinit recursion!\n" );
        while ( 1 )
            ;
    }

    recurse_protect = 1;
    used_list  = new pkiub_list;
    free_list  = new pkiub_list;
    array_list = new pkiub_array_list;
    recurse_protect = 0;

    red_malloc_initialized = 1;
}

static void redmallocaudittask( void );

// the audit task is started by this call; this call is made from
// the bsp USER_APPL_INIT macro once scheduling is up.

void
redmallocspawntask( void )
{
    red_malloc_multitasking = 1;
    taskSpawn( "redmalloc",  240,  0,  8192,
               (FUNCPTR) redmallocaudittask,
               0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
}

// static
pkiub *
pkiub :: find_pointer( void * _ptr )
{
    char * ptr = (char *)_ptr;
    ptr -= REDZONE_SIZE;

    // first try to see if there is a valid pkiub pointer
    // within the memory block.

    int bad = NO_ERROR;
    int pkiub_pointer1, pkiub_pointer2, pkiub_pointer3, pkiub_pointer4;

    pkiub_pointer1 = *(int*)(ptr+(REDZONE_SIZE/2)  );
    pkiub_pointer2 = *(int*)(ptr+(REDZONE_SIZE/2)+4);

    pkiub * iub;

    if ( pkiub_pointer1 == ( pkiub_pointer2 ^ RANDOM_XOR ))
    {
        iub = (pkiub *)pkiub_pointer1;

        pkiub_pointer3 = *(int*)(ptr+iub->size+(REDZONE_SIZE*3/2)  );
        pkiub_pointer4 = *(int*)(ptr+iub->size+(REDZONE_SIZE*3/2)+4);

        if ( pkiub_pointer3 == ( pkiub_pointer4 ^ RANDOM_XOR )  &&
             pkiub_pointer1 == pkiub_pointer3 )
        {
            return (pkiub *) pkiub_pointer1;
        }

        bad |= INVALID_PTR_MG2;
    }
    else
        bad |= INVALID_PTR_MG1;

    // else there is not, so search the in-use list instead.

    for ( iub = used_list->get_head(); iub;
          iub = used_list->get_next( iub ))
    {
        if ( iub->pointer == ptr )
            break;
    }

    if ( iub )
    {
        if ( iub->bad == NO_ERROR )
            pprintf( "error, find_pointer had to resort to "
                     "searching the used list\n" );
        iub->bad |= bad;
    }
    else
        pprintf( "error, pointer not found on the used list\n" );

    return iub;
}

void *
pkiub :: get_memory( int _size, int _pc )
{
    size = _size;
    pc = _pc;
    bad = NO_ERROR;

    if ( red_malloc_multitasking )
    {
        char * name = taskName( taskIdSelf() );
        if ( name )
            strncpy( task_name, name, TASK_NAME_WIDTH );
        else
            strcpy( task_name, "error" );
    }
    else
        strcpy( task_name, "init" );

    pointer = (char*) _real_malloc( size + (REDZONE_SIZE*2) );

    if ( pointer == NULL )
        return NULL;

    memset( pointer                      , ZONE_VALUE, REDZONE_SIZE );
    memset( pointer + REDZONE_SIZE       , LIVE_VALUE, size         );
    memset( pointer + REDZONE_SIZE + size, ZONE_VALUE, REDZONE_SIZE );

    *(int*)(pointer+     (REDZONE_SIZE  /2)  ) = ((int)this);
    *(int*)(pointer+     (REDZONE_SIZE  /2)+4) = ((int)this) ^ RANDOM_XOR;
    *(int*)(pointer+size+(REDZONE_SIZE*3/2)  ) = ((int)this);
    *(int*)(pointer+size+(REDZONE_SIZE*3/2)+4) = ((int)this) ^ RANDOM_XOR;

    return pointer + REDZONE_SIZE;
}

int
pkiub :: check_zones( bool ignore_history )
{
    unsigned char * p = (unsigned char *) pointer;
    int i;
    int ret = NO_ERROR;

    for ( i = 0; i < (REDZONE_SIZE/2); i++ )
        if ( *p++ != ZONE_VALUE )
            ret |= ZONE1_WRONG;

    if ( *(int*)(p  ) != (((int)this))   ||
         *(int*)(p+4) != (((int)this) ^ RANDOM_XOR ))
    {
        ret |= INVALID_PTR_MG1;
    }
    p += 8;
    i += 8;

    for ( ; i <  REDZONE_SIZE; i++ )
        if ( *p++ != ZONE_VALUE )
            ret |= ZONE2_WRONG;

    p += size;

    for ( i = 0; i < (REDZONE_SIZE/2); i++ )
        if ( *p++ != ZONE_VALUE )
            ret |= ZONE3_WRONG;

    if ( *(int*)(p  ) != (((int)this))   ||
         *(int*)(p+4) != (((int)this) ^ RANDOM_XOR ))
    {
        ret |= INVALID_PTR_MG2;
    }
    p += 8;
    i += 8;

    for ( ; i <  REDZONE_SIZE; i++ )
        if ( *p++ != ZONE_VALUE )
            ret |= ZONE4_WRONG;

    if ( !ignore_history )
        if ( (bad ^ ret) == 0 )
            return 0;

    bad |= ret;

    return ret;
}

void
pkiub :: free_memory( void )
{
    memset( pointer, DEAD_VALUE, size + (REDZONE_SIZE*2) );
    _real_free( pointer );
    pointer = NULL;
}

pkiub_array :: pkiub_array( void )
{
    for ( int i = 0; i < ALLOC_AT_ONCE; i++ )
        free_list->add( &blocks[i] );
}

static pkiub *
get_free_iub( void )
{
    pkiub * iub;
    int lock_value;

    lock_value = LOCK();
    if ( free_list->get_cnt() == 0 )
        array_list->add( new pkiub_array );
    iub = free_list->dequeue_head();
    UNLOCK(lock_value);

    return iub;
}

void *
_redrealloc( void * p, int new_sz, int pc )
{
    int lock_value;
    void * new_p;

    new_p = _redmalloc( new_sz, pc );
    if ( new_p == NULL )
        return NULL;

    if ( p )
    {
        lock_value = LOCK();
        pkiub * iub = pkiub::find_pointer( p );
        if ( !iub )
        {
            UNLOCK(lock_value);
            pprintf( "realloc: can't find old pointer\n" );
        }
        else
        {
            if ( iub->size > 0 )
                memcpy( new_p, p, iub->size );
            UNLOCK(lock_value);
            _redfree( p );
        }
    }

    return new_p;
}

void *
_redcalloc( int num, int size, int pc )
{
    void * ret;

    ret = _redmalloc( num * size, pc );
    if ( ret )
        memset( ret, 0, num*size );

    return ret;
}

void *
_redmalloc( int size, int pc )
{
    int lock_value;
    void * ret;
    pkiub * iub;

    if ( red_malloc_initialized == 0 )
        redmallocinit();

    iub = get_free_iub();
    ret = iub->get_memory( size, pc );

    if ( ret == NULL )
        return NULL;

    lock_value = LOCK();
    used_list->add( iub );
    UNLOCK(lock_value);

    return ret;
}

void
_redfree( void * ptr )
{
    int error_flag = pkiub::NO_ERROR, lock_value;
    pkiub * iub;
    int pc;

    lock_value = LOCK();

    iub = pkiub::find_pointer( ptr );
    if ( !iub )
    {
        error_flag |= pkiub::INVALID_POINTER;
        pc = 0;
    }
    else
    {
        used_list->remove( iub );
        error_flag |= iub->check_zones( false );
        pc = iub->pc;
        iub->free_memory();
        free_list->add( iub );
    }

    UNLOCK(lock_value);

    if ( error_flag != pkiub::NO_ERROR )
        pprintf( "task %#x (%s) while freeing %#x, err %#x, pc %#x\n",
                taskIdSelf(), taskName( taskIdSelf() ), ptr, error_flag, pc );
}

static void
_redmallocaudit( void )
{
    pkiub            * iub;
    int                lock_value, err_val;
    display_item_list  items;

    lock_value = LOCK();

    for ( iub = used_list->get_head(); iub;
          iub = used_list->get_next( iub ))
    {
        err_val = iub->check_zones( false );
        if ( err_val != 0 )
            // can't print during a LOCK, so make a list instead.
            items.add( new display_item( iub, err_val ));
    }

    UNLOCK(lock_value);

    while ( display_item * item = items.get_head() )
    {
        items.remove( item );
        item->display(true);
        delete item;
    }
}

static void
redmallocaudittask( void )
{
    while ( 1 )
    {
        _redmallocaudit();
        taskDelay( 20 );
    }
}



void
red_malloc_display( void )
{
    pkiub            * iub;
    int                lock_value, sum;
    display_item_list  items;

    lock_value = LOCK();

    for ( iub = used_list->get_head(); iub;
          iub = used_list->get_next( iub ))
    {
        items.add( new display_item( iub, iub->check_zones( true )));
    }

    UNLOCK(lock_value);

    sum = 0;
    while ( display_item * item = items.get_head() )
    {
        items.remove( item );
        sum += item->display(false);
        delete item;
    }

    printf( "total memory in use: %d bytes\n", sum );
}
