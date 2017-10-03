
#include <stdarg.h>
#include "dll2_c.h"

#define LINKS(list,item) \
    (&((DLL2_LINKS*)(((unsigned int)item) + links_offset))[list->index])

void
dll2_list_add( DLL2_LIST * list, int links_offset, void * item,
               char * file, int line )
{
    DLL2_LINKS * ll = LINKS(list,item);
    if ( ll->onlist != 0 )
        dll2_error( file, line, "dll2_list_add wrong list" );
    ll->onlist = list;
    ll->next = 0;
    ll->prev = list->tail;
    if ( list->head != 0 )
    {
        ll = LINKS(list,list->tail);
        ll->next = item;
    }
    else
        list->head = item;
    list->tail = item;
    list->count++;
}

void
dll2_list_remove( DLL2_LIST * list, int links_offset, void * item,
                  char * file, int line )
{
    DLL2_LINKS * ll = LINKS(list,item);
    DLL2_LINKS * el;
    if ( ll->onlist != list )
        dll2_error( file, line, "dll2_list_remove wrong list" );
    ll->onlist = 0;
    if ( ll->next != 0 )
    {
        el = LINKS(list,ll->next);
        el->prev = ll->prev;
    }
    else
        list->tail = ll->prev;
    if ( ll->prev != 0 )
    {
        el = LINKS(list,ll->prev);
        el->next = ll->next;
    }
    else
        list->head = ll->next;
    ll->next = ll->prev = 0;
    list->count--;
}

void
dll2_list_add_after( DLL2_LIST * list, int links_offset,
                     void * existing, void * item,
                     char * file, int line )
{
    DLL2_LINKS * ell = LINKS(list,existing);
    DLL2_LINKS * ill = LINKS(list,item);
    if ( ell->onlist != list )
        dll2_error( file, line, "dll2_list_add_after wrong list" );
    if ( ill->onlist != 0 )
        dll2_error( file, line, "dll2_list_add_after already on list" );
    ill->onlist = list;
    ill->next = ell->next;
    ill->prev = existing;
    ell->next = item;
    if ( ill->next )
    {
        ill = LINKS(list,ill->next);
        ill->prev = item;
    }
    else
        list->tail = item;
}

void
dll2_list_add_before( DLL2_LIST * list, int links_offset,
                      void * existing, void * item,
                      char * file, int line )
{
    DLL2_LINKS * ell, * ill;
    ell = LINKS(list,existing);
    ill = LINKS(list,item);
    if ( ell->onlist != list )
        dll2_error( file, line,
                    "dll2_list_add_before existing on wrong list" );
    if ( ill->onlist != 0 )
        dll2_error( file, line, 
                    "dll2_list_add_before already on list" );
    ill->onlist = list;
    ill->prev = ell->prev;
    ill->next = existing;
    ell->prev = item;
    if ( ill->prev )
    {
        ill = LINKS(list,ill->prev);
        ill->next = item;
    }
    else
        list->head = item;
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
