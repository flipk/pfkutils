/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include "dll2.h"
#include <pthread.h>

#include "bst.h"

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>

/** a macro for defining internal message classes.

xxx

 */
#define PkMsgIntDef( classname, typevalue, body ) \
class classname : public pk_msg_int { \
public: \
    static const uint16_t TYPE = typevalue ; \
    classname( void ) : pk_msg_int( sizeof( classname ), TYPE ) { } \
    body \
}

/** a comma.
 * the COMMA definition is useful if the \ref PkMsgIntDef2 class constructor
 * would like to insert member-constructions. yeah, i know, it's kind
 * of gross -- i admit, not my finest hour. */
#define COMMA ,

/**

xxx

*/
#define PkMsgIntDef2( classname, typevalue, body, \
                      constructargs, constructor ) \
class classname : public pk_msg_int { \
public: \
    static const uint16_t TYPE = typevalue ; \
    classname( void ) : pk_msg_int( sizeof( classname ), TYPE ) { } \
    classname constructargs : pk_msg_int( sizeof( classname ), TYPE ) \
        constructor \
    body \
}

/** base class for internal (thread-to-thread) message,
 * see \ref PkMsgIntDef */
class pk_msg_int {
public:
    LListLinks <pk_msg_int> links[1];
    /** length in bytes of the message, automatically populated.
     * this field is automatically filled in by \ref PkMsgIntDef
     * (using sizeof). */
    uint16_t    length;
    /** type of the message body, automatically populated.
     * this field is automatically filled in by \ref PkMsgIntDef and
     * comes from the TYPE value declared by that macro. */
    uint16_t    type;
    /** source message queue id, user may fill if required.
     * this field is not automatically populated. the user must
     * fill it if the recipient of this message wants to reply
     * to sender. if the recipient will not reply to sender, this
     * field does not have to be populated. */
    uint32_t    src_q;
    /** destination message queue id, user must fill.
     * this field is not automatically populated. the user must
     * fill this in order for the message to be delivered to its
     * destination. */
    uint32_t    dest_q;
//
    /** constructor, called automatically by \ref PkMsgIntDef. 
     * user should not have to call this.
     * \param _length the length of the message in bytes.
     * \param _type  the type of the message, from the TYPE constant. */
    pk_msg_int( uint16_t _length, uint16_t _type )
        : length( _length ), type( _type ) { }
    /** automatic method for upcasting to derived message type.
     * derived types defined by \ref PkMsgIntDef will have a TYPE
     * constant. this template method can be used to convert a 
     * pk_msg_int pointer to the derived message type.
     * \param ptr   a pointer to a derived message type. this pointer
     *             will be written if the base message \ref type value
     *             matches the derived type's TYPE constant value,
     *             or NULL if it does not match.
     * \return true if the types match and ptr has been written,
     *          or false if the types do not match.
     *
     * consider the following messages and thread class:
     * \code
     * PkMsgIntDef(MyMessage1, 0x0001, 
     *             int field1;
     *             int field2;
     *     );
     * PkMsgIntDef(MyMessage2, 0x0002, 
     *             int field1;
     *             int field2;
     *     );
     * class MyThread : public PK_Thread {
     *    MyThread(void) { resume(); }
     *    void entry(void);
     * };
     * \endcode
     * the following two implementations of \a entry are identical.
     * \code
     * void MyThread::entry(void) {
     *     pk_msg_int * msg;
     *     while (1) {
     *         int rcvd_qid;
     *         msg = PK_Thread::msg_recv(1, &myqid, &rcvd_qid, -1);
     *         MyMessage1 * msg1;
     *         MyMessage2 * msg2;
     *         if (msg->convert(&msg1))
     *         {
     *             // handle msg1
     *         }
     *         else if (msg->convert(&msg2))
     *         {
     *             // handle msg2
     *         }
     *         delete msg;
     *     }
     * }
     * \endcode
     * \code
     * void MyThread::entry(void) {
     *     union {
     *         pk_msg_int * msg;
     *         MyMessage1 * msg1;
     *         MyMessage2 * msg2;
     *     } u;
     *     while (1) {
     *         int rcvd_qid;
     *         u.msg = PK_Thread::msg_recv(1, &myqid, &rcvd_qid, -1);
     *         switch (u.msg->type) {
     *         case MyMessage1::TYPE:
     *             // handle u.msg1
     *             break;
     *         case MyMessage2::TYPE:
     *             // handle u.msg2
     *             break;
     *         }
     *         delete u.msg;
     *     }
     * }
     * \endcode */
    template <class T> bool convert( T ** ptr ) {
        if ( type != T::TYPE )
            return false;
        *ptr = (T*)this;
        return true;
    }
};

/** \cond INTERNAL */
class PK_Message_Queue_List;

class PK_Message_Manager {
    pthread_mutex_t   mutex;
    PK_Message_Queue_List * queues;
    void   _lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void _unlock( void ) { pthread_mutex_unlock( &mutex ); }
public:
    PK_Message_Manager( void );
    ~PK_Message_Manager( void );
//
    int          create( char * name );  // returns qid
    int          lookup( char * name );  // returns qid or -1
    bool         destroy( int qid ); // returns false if qid does not exist
    bool         send( int qid, pk_msg_int * msg ); // returns false if error
    pk_msg_int * recv( int num_qids, int * qids,
                       int * retqid, int ticks );
};

extern PK_Message_Manager * PK_Messages_global;
/** \endcond */

#endif /* __MESSAGES_H__ */
