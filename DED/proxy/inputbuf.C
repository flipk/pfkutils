
#include "inputbuf.H"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void *
inputbuf :: operator new( size_t crap, int size )
{
    inputbuf * r;
    r = (inputbuf*) malloc( sizeof( inputbuf ) + size );
    r->size = size;
    return (void*)r;
}

void
inputbuf :: operator delete( void * p )
{
    free( p );
}

inputbuf :: inputbuf( int _fd, int (*_rf)(int,char*,int) )
{
    fd = _fd;
    readfunc = _rf;
    curdat = 0;
    memset( data, 0, size );
}

int
inputbuf :: readmore( void )
{
    int max;
    int cc;

    max = size - curdat;

    cc = readfunc( fd, data + curdat, max );
    if ( cc < 0 )
        return -1;

    curdat += cc;
    return cc;
}

// look:
//   look for newline in curdat
//   if no newline found
//     if room left in data[]
//       read more data in
//       goto look
//     else
//       return full buf
//     endif
//   endif
//   prepare return buf with part found
//   memcpy remaining data down
//   return buf

char *
inputbuf :: getline( void )
{
    char * ret;
    int i = 0;
    bool newline;
    // some requestors might send only a newline;
    // some might send only a carriage return;
    // some might send both. and when both are sent,
    // they could be sent in either order.
    // look for either, and if the opposite follows, discard it.
    bool discard = false;

    do {
        newline = false;

        for ( ; i < curdat && !newline; i++ )
            if ( data[i] == '\r' || data[i] == '\n' )
            {
                newline = true;
                if ( i != (curdat-1) )
                    // here's how we look for \r followed by \n
                    // or \n followed by \r in one bit of code..
                    // if we know one of them is first, the sum of
                    // one plus the next is 23 only if the other
                    // is the opposite.
                    if (( data[i] + data[i+1] ) == 23 )
                        discard = true;
            }

        if ( i == size )
        {
            ret = new char[size+1];
            memcpy( ret, data, size );
            ret[size] = 0;
            curdat = 0;
            return ret;
        }
        /* else */

        if ( !newline )
        {
            int r = readmore();
            if ( r <= 0 )
            {
                return NULL;
            }
        }
    } while ( !newline );

    ret = new char[ i ];
    memcpy( ret, data, i-1 );
    ret[i-1] = 0;

    if ( discard )
        i++;

    if ( i != curdat )
        memcpy( data, data + i, curdat - i );
    curdat -= i;

    return ret;
}

int
inputbuf :: read( char * buf, int len )
{
    if ( len > size )
    {
        printf( "error, inbuf::read got %d req, but max size is %d\n",
                len, size );
        return -2;
    }

    while ( curdat < len )
    {
        int r = readmore();
        if ( r == 0 )
            break;
        if ( r < 0 )
            return -1;
    }

    if ( curdat < len )
        len = curdat;

    memcpy( buf, data, len );
    if ( len != curdat )
        memcpy( data, data + len, curdat - len );

    curdat -= len;
    return len;
}
