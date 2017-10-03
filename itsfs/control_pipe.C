/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include "mytypes.h"
#include "control_pipe.H"
#include <errno.h>
#include <string.h>
#include <stdio.h>

static uchar * data;
static int datalen;

Control_Pipe :: Control_Pipe( void )
{
    data = NULL;
    datalen = 0;
}

Control_Pipe :: ~Control_Pipe( void )
{
    if ( data )
        delete[] data;
    datalen = 0;
}

int
Control_Pipe :: write( uchar * buf, int  length )
{
    if ( data )
        delete[] data;
    data = new uchar[ length ];
    memcpy( data, buf, length );
    datalen = length;
    return 0;
}

int
Control_Pipe :: read ( uchar * buf, int &length )
{
    if ( !data )
    {
        length = 0;
        return 0;
    }
    if ( length > datalen )
        length = datalen;
    memcpy( buf, data, length );
    delete[] data;
    data = NULL;
    datalen = 0;
    return 0;
}
