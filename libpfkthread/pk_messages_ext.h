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

#ifndef __MESSAGES_EXT_H__
#define __MESSAGES_EXT_H__

#include "dll2.h"
#include <pthread.h>

#include "bst.h"

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>

class pk_msg_ext_hdr : public BST {
    BST_UINT32_t magic;
    BST_UINT16_t type;
    BST_UINT16_t length;
    static int length_offset(void) { return 6; }
    BST_UINT32_t checksum;
    static int checksum_offset(void) { return 8; }
public:
    static const uint32_t MAGIC = 0x819b8300UL;
    pk_msg_ext_hdr(BST *parent, uint16_t _type)
        : BST(parent),
          magic(this), type(this), length(this), checksum(this)
    {
        magic() = MAGIC;
        type() = _type;
    }
    bool valid_magic(void) { return magic() == MAGIC; }
    void set_length(uint16_t len) { length() = len; }
    uint16_t get_length(void) { return length(); }
    uint16_t get_type(void) { return type(); }
    uint32_t get_checksum(void) { return checksum(); }
    static void post_encode_set_checksum(uint8_t * buf,
                                         uint32_t checksum) {
        UINT32_t * checksum_location = (UINT32_t *) (buf + checksum_offset());
        checksum_location->set( checksum );
    }
    static void post_encode_set_len(uint8_t * buf, uint16_t len) {
        UINT16_t * length_location = (UINT16_t *) (buf + length_offset());
        length_location->set( len );
    }
    static uint32_t calc_checksum( uint8_t * buf, int len ) {
        uint32_t checksum = 0xc2db895cUL;
        for (int i=0; i < len; i++)
            checksum = ((checksum << 5) + checksum) + (buf[i] + i);
        return checksum;
    }
};

class pk_msg_ext : public BST {
public:
    pk_msg_ext_hdr hdr;
    LListLinks <pk_msg_ext> links[1];
    pk_msg_ext( uint16_t type )
        : BST(NULL), hdr(this, type) { }
    virtual ~pk_msg_ext( void ) { }
    virtual uint16_t get_TYPE(void) = 0;
};

template <class T, int typeValue> 
class pk_msg_ext_body : public pk_msg_ext {
public:
    static const uint16_t TYPE = typeValue;
    pk_msg_ext_body( void ) : pk_msg_ext(TYPE), body(this) { }
    T body;
    /*virtual*/ uint16_t get_TYPE(void) { return TYPE; }
};

#define PkMsgExtDef( classname, typevalue, bodytype )       \
    typedef pk_msg_ext_body<bodytype,typevalue> classname

// it is intended that a thread class can multiply inherit
// from both this and PK_Thread at the same time, and just put
// the handler methods right inside the thread class.
class PK_Message_Ext_Handler {
public:
    PK_Message_Ext_Handler(void) { }
    virtual ~PK_Message_Ext_Handler(void) { }
    // used by PK_Message_Ext_Manager for receiving
    virtual pk_msg_ext * make_msg( uint16_t type ) = 0;
};

class PK_Message_Ext_Link {
protected:
    PK_Message_Ext_Handler * handler;
    bool connected;
public:
    PK_Message_Ext_Link(PK_Message_Ext_Handler * _handler) {
        handler = _handler;
        connected = false;
    }
    virtual ~PK_Message_Ext_Link(void) { }
    // false means link closure
    virtual bool write( uint8_t * buf, int buflen ) = 0;
    // 0 means timeout, -1 means error
    virtual int  read ( uint8_t * buf, int buflen, int ticks ) = 0;
    bool Connected(void) { return connected; }
};

class PK_Message_Ext_Manager {
    static const int MAX_MSG_SIZE = 8192;
    uint8_t sendbuf[MAX_MSG_SIZE];
    uint8_t rcvbuf[MAX_MSG_SIZE];
    int   rcvbufpos;
    int   rcvbufsize;
    enum state {
        STATE_HEADER_HUNT_1,
        STATE_HEADER_HUNT_2,
        STATE_HEADER_HUNT_3,
        STATE_HEADER_HUNT_4,
        STATE_TYPE_READ_1,
        STATE_TYPE_READ_2,
        STATE_LEN_READ_1,
        STATE_LEN_READ_2,
        STATE_READ_BODY
    };
    state s;
    int read_remaining; // only valid in READ_BODY state
    // return 0xFFFFUS if ticks expires.
    uint16_t get_byte(int ticks, bool beginning=false);
protected:
    PK_Message_Ext_Handler * handler;
    PK_Message_Ext_Link * link;
public:
    // deleting this class will NOT delete the handler or the link.
    PK_Message_Ext_Manager( PK_Message_Ext_Handler * _handler,
                            PK_Message_Ext_Link * _link );
    ~PK_Message_Ext_Manager( void );
    // false means fail
    bool send( pk_msg_ext * msg );
    // null means link closure
    pk_msg_ext * recv(int ticks);
};

class PK_Message_Ext_Link_TCP
    : public PK_Message_Ext_Link, public PK_Message_Ext_Manager {
    int fd;
public:
    // deleting this class will close the connection, but will NOT
    // delete the handler.
    PK_Message_Ext_Link_TCP(PK_Message_Ext_Handler * _handler,
                            short port);
    PK_Message_Ext_Link_TCP(PK_Message_Ext_Handler * _handler,
                            char * host, short port);
    ~PK_Message_Ext_Link_TCP(void);
    /*virtual*/ bool write( uint8_t * buf, int buflen );
    /*virtual*/ int  read ( uint8_t * buf, int buflen, int ticks );
};

#endif /* __MESSAGES_EXT_H__ */
