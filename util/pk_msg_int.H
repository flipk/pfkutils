/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#ifndef __PK_MSG_INT_H_
#define __PK_MSG_INT_H_

// this is a version of a pk_msg for internal use
// (internal==between threads rather than over an external link)

#include "dll2.H"
#include "types.H"

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>

#define PkMsgIntDef( classname, typevalue, body ) \
class classname : public pk_msg_int { \
public: \
    static const UINT16 TYPE = typevalue ; \
    classname( void ) : pk_msg_int( sizeof( classname ), TYPE ) { } \
    body \
}

// the COMMA definition is useful if the 'constructor' argument below
// would like to insert member-constructions
#define COMMA ,

#define PkMsgIntDef2( classname, typevalue, body, \
                      constructargs, constructor ) \
class classname : public pk_msg_int { \
public: \
    static const UINT16 TYPE = typevalue ; \
    classname( void ) : pk_msg_int( sizeof( classname ), TYPE ) { } \
    classname constructargs : pk_msg_int( sizeof( classname ), TYPE ) \
        constructor \
    body \
}

struct pk_msg_int {
    LListLinks <pk_msg_int> links[1];
    UINT16    length; // automatically filled by PkMsgIntDef*
    UINT16    type; // automatically filled by PkMsgIntDef*
    UINT32    src_q; // user must fill
    UINT32    dest_q; // user must fill
//
    pk_msg_int( UINT16 _length, UINT16 _type )
        : length( _length ), type( _type ) { }
    template <class T> bool convert( T ** ptr ) {
        if ( type != T::TYPE )
            return false;
        *ptr = (T*)this;
        return true;
    }
};

#endif /* __PK_MSG_INT_H_ */
