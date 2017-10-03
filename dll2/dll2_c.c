/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdarg.h>
#include "dll2_c.h"

#define LINKS(list,item) \
    (&((DLL2_LINKS*)(((unsigned int)item) + links_offset))[list->list_index])

void
dll2_list_add( DLL2_LIST * list, int links_offset, void * item,
               char * file, int line )
{
    DLL2_LINKS * ll = LINKS(list,item);
#if DLL2_MAGIC_VALIDATIONS
    if ( !DLL2_LIST_ISINIT(list) )
        dll2_error( file, line, "dll2_list_add list not initialized" );
    if ( ll->links_magic != DLL2_LINKS_MAGIC )
        dll2_error( file, line, "dll2_list_add item not initialized" );
#endif
    if ( ll->links_onlist != 0 )
        dll2_error( file, line, "dll2_list_add item already on list" );
    ll->links_onlist = list;
    ll->links_next = 0;
    ll->links_prev = list->list_tail;
    if ( list->list_head != 0 )
    {
        ll = LINKS(list,list->list_tail);
        ll->links_next = item;
    }
    else
        list->list_head = item;
    list->list_tail = item;
    list->list_count++;
}

void
dll2_list_remove( DLL2_LIST * list, int links_offset, void * item,
                  char * file, int line )
{
    DLL2_LINKS * ll = LINKS(list,item);
    DLL2_LINKS * el;
#if DLL2_MAGIC_VALIDATIONS
    if ( !DLL2_LIST_ISINIT(list) )
        dll2_error( file, line, "dll2_list_remove list not initialized" );
#endif
    if ( ll->links_onlist != list )
        dll2_error( file, line, "dll2_list_remove wrong list" );
    ll->links_onlist = 0;
    if ( ll->links_next != 0 )
    {
        el = LINKS(list,ll->links_next);
        el->links_prev = ll->links_prev;
    }
    else
        list->list_tail = ll->links_prev;
    if ( ll->links_prev != 0 )
    {
        el = LINKS(list,ll->links_prev);
        el->links_next = ll->links_next;
    }
    else
        list->list_head = ll->links_next;
    ll->links_next = ll->links_prev = 0;
    list->list_count--;
}

void
dll2_list_add_after( DLL2_LIST * list, int links_offset,
                     void * existing, void * item,
                     char * file, int line )
{
    DLL2_LINKS * ell = LINKS(list,existing);
    DLL2_LINKS * ill = LINKS(list,item);
#if DLL2_MAGIC_VALIDATIONS
    if ( !DLL2_LIST_ISINIT(list) )
        dll2_error( file, line, "dll2_list_add_after list not initialized" );
    if ( ill->links_magic != DLL2_LINKS_MAGIC )
        dll2_error( file, line, "dll2_list_add_after item not initialized" );
#endif
    if ( ell->links_onlist != list )
        dll2_error( file, line, "dll2_list_add_after existing on wrong list" );
    if ( ill->links_onlist != 0 )
        dll2_error( file, line, "dll2_list_add_after already on list" );
    ill->links_onlist = list;
    ill->links_next = ell->links_next;
    ill->links_prev = existing;
    ell->links_next = item;
    if ( ill->links_next )
    {
        ill = LINKS(list,ill->links_next);
        ill->links_prev = item;
    }
    else
        list->list_tail = item;
    list->list_count++;
}

void
dll2_list_add_before( DLL2_LIST * list, int links_offset,
                      void * existing, void * item,
                      char * file, int line )
{
    DLL2_LINKS * ell, * ill;
    ell = LINKS(list,existing);
    ill = LINKS(list,item);
#if DLL2_MAGIC_VALIDATIONS
    if ( !DLL2_LIST_ISINIT(list) )
        dll2_error( file, line, "dll2_list_add_before list not initialized" );
    if ( ill->links_magic != DLL2_LINKS_MAGIC )
        dll2_error( file, line, "dll2_list_add_before item not initialized" );
#endif
    if ( ell->links_onlist != list )
        dll2_error( file, line,
                    "dll2_list_add_before existing on wrong list" );
    if ( ill->links_onlist != 0 )
        dll2_error( file, line, 
                    "dll2_list_add_before already on list" );
    ill->links_onlist = list;
    ill->links_prev = ell->links_prev;
    ill->links_next = existing;
    ell->links_prev = item;
    if ( ill->links_prev )
    {
        ill = LINKS(list,ill->links_prev);
        ill->links_next = item;
    }
    else
        list->list_head = item;
    list->list_count++;
}

void *
dll2_error( char * file, int line, char * format, ... )
{
    static char out[100];
    va_list ap;
    va_start( ap, format );
    vsnprintf( out, sizeof(out), format, ap );
    out[sizeof(out)-1] = 0;
    va_end( ap );
    printf( "dll2_error at %s:%d:%s\n", file, line, out );
    kill( 0, 6 );
    return 0;
}
