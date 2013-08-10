/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
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

#ifndef __BYTESTREAM_H__
#define __BYTESTREAM_H__

// ByteStream interface (BST)

#include <string.h>
#include <stdio.h>
#include "types.H"

enum BST_OP {
//    BST_OP_NONE,       // stream is uninitialized.
    BST_OP_ENCODE = 1,   // encode data types into a byte stream.
    BST_OP_CALC_SIZE,    // calculate the amount of memory required by
    /**/                 //   an encode sequence but do not actually encode.
    BST_OP_DECODE,       // decode a byte stream and alloc dynamic memory.
    BST_OP_MEM_FREE      // free any dynamically allocated memory.
};

class BST_STREAM {
protected:
    BST_OP  op;
public:
    BST_STREAM(void) { op = (BST_OP)0; /* BST_OP_NONE */ }
    virtual ~BST_STREAM(void) { /*placeholder*/ }
    BST_OP get_op(void) { return op; }
    virtual UCHAR * get_ptr(int step) = 0;
};

class BST_STREAM_BUFFER : public BST_STREAM {
    UCHAR * buffer;
    int     buffer_size;
    UCHAR * ptr;
    int     size;
    int     remaining;
    bool    my_buffer;
public:
    // the user supplies the buffer and also takes care
    // of deleting it.
    BST_STREAM_BUFFER( UCHAR * _buffer, int _size ) {
        buffer = _buffer;
        buffer_size = _size;
        my_buffer = false;
    }
    // this object creates the buffer, which is deleted when
    // this object is deleted.
    BST_STREAM_BUFFER( int _size ) {
        buffer = new UCHAR[_size];
        buffer_size = _size;
        my_buffer = true;
    }
    ~BST_STREAM_BUFFER(void) {
        if (my_buffer)
            delete[] buffer;
    }
    // reset the current buffer position to the beginning, used
    // to reset the stream state for beginning a new operation.
    void start(BST_OP _op, UCHAR * _buffer=NULL, int _size=0) {
        op = _op; 
        if (_buffer && _size)
        {
            if (my_buffer)
                delete[] buffer;
            buffer = _buffer;
            buffer_size = _size;
            my_buffer = false;
        }
        ptr = buffer;
        size = 0;
        remaining = buffer_size;
    }
    // return the start of the buffer; used at the end of an
    // encode to collect the constructed bytestream.
    UCHAR * get_finished_buffer(void) { return buffer; }
    // return the amount of data in the buffer; used at the end
    // of an encode to collect the size of the constructed bytestream.
    int get_finished_size(void) { return size; }
    // return the current working pointer.  used by bst_op in all 
    // data types as the bytestream is processed.  specify the number
    // of bytes needed.  if there are not enough left in the buffer,
    // returns NULL.
    /*virtual*/ UCHAR * get_ptr(int step) {
        if (remaining < step)
            return NULL;
        UCHAR * ret = ptr;
        remaining -= step;
        size += step;
        if (op == BST_OP_CALC_SIZE)
            // return something, anything, that is not NULL,
            // because the user may have specified a NULL buffer ptr
            // for the calc op.
            return (UCHAR*)1;
        ptr += step;
        return ret;
    }
};

// base class for any type which can be represented as a ByteStream
class BST {
    BST * head;
    BST * tail;
    BST * next;
    friend class BST_UNION;
public:
    BST(BST *parent) {
        head = tail = next = NULL;
        if (parent) {
            if (parent->tail) {
                parent->tail->next = this;
                parent->tail = this;
            } else {
                parent->head = parent->tail = this;
            }
        }
    }
    virtual ~BST(void) { /*placeholder*/ }
    // return true if operation was successful; 
    // return false if some problem (out of buffer, etc)
    virtual bool bst_op( BST_STREAM *str ) {
        for (BST * b = head; b; b = b->next)
            if (!b->bst_op(str))
                return false;
        return true;
    }
    int bst_calc_size(void) {
        BST_STREAM_BUFFER str(NULL,65536);
        str.start(BST_OP_CALC_SIZE);
        bst_op(&str);
        return str.get_finished_size();
    }
    UCHAR * bst_encode( int * len ) {
        *len = bst_calc_size();
        UCHAR * buffer = new UCHAR[*len];
        if (bst_encode(buffer, len) == false)
        {
            *len = 0;
            delete[] buffer;
            return NULL;
        }
        return buffer;
    }
    bool bst_encode( UCHAR * buffer, int * len ) {
        BST_STREAM_BUFFER str(buffer,*len);
        str.start(BST_OP_ENCODE);
        if (bst_op(&str) == false)
            return false;
        *len = str.get_finished_size();
        return true;
    }
    bool bst_decode( UCHAR * buffer, int len ) {
        BST_STREAM_BUFFER str(buffer,len);
        str.start(BST_OP_DECODE);
        return bst_op(&str);
    }
    void bst_free(void) {
        BST_STREAM_BUFFER str(NULL,0);
        str.start(BST_OP_MEM_FREE);
        bst_op(&str);
    }
};

// BST base types follow.

struct BST_UINT64_t : public BST {
    BST_UINT64_t(BST *parent) : BST(parent) { }
    unsigned long long v;
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        UCHAR * ptr;
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
            ptr = str->get_ptr(8);
            if (!ptr)
                return false;
            ptr[0] = (v >> 56) & 0xFF;
            ptr[1] = (v >> 48) & 0xFF;
            ptr[2] = (v >> 40) & 0xFF;
            ptr[3] = (v >> 32) & 0xFF;
            ptr[4] = (v >> 24) & 0xFF;
            ptr[5] = (v >> 16) & 0xFF;
            ptr[6] = (v >>  8) & 0xFF;
            ptr[7] = (v >>  0) & 0xFF;
            return true;
        case BST_OP_CALC_SIZE:
            if (!str->get_ptr(8))
                return false;
            return true;
        case BST_OP_DECODE:
            ptr = str->get_ptr(8);
            if (!ptr)
                return false;
            v = ((unsigned long long)ptr[0] << 56) +
                ((unsigned long long)ptr[1] << 48) +
                ((unsigned long long)ptr[2] << 40) +
                ((unsigned long long)ptr[3] << 32) +
                ((unsigned long long)ptr[4] << 24) +
                ((unsigned long long)ptr[5] << 16) +
                ((unsigned long long)ptr[6] <<  8) +
                ((unsigned long long)ptr[7] <<  0);
            return true;
        case BST_OP_MEM_FREE:
            return true;
        }
        return false;
    }
};

struct BST_UINT32_t : public BST {
    BST_UINT32_t(BST *parent) : BST(parent) { }
    unsigned int v;
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        UCHAR * ptr;
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
            ptr = str->get_ptr(4);
            if (!ptr)
                return false;
            ptr[0] = (v >> 24) & 0xFF;
            ptr[1] = (v >> 16) & 0xFF;
            ptr[2] = (v >>  8) & 0xFF;
            ptr[3] = (v >>  0) & 0xFF;
            return true;
        case BST_OP_CALC_SIZE:
            if (!str->get_ptr(4))
                return false;
            return true;
        case BST_OP_DECODE:
            ptr = str->get_ptr(4);
            if (!ptr)
                return false;
            v = (ptr[0] << 24) + (ptr[1] << 16) +
                (ptr[2] <<  8) + (ptr[3] <<  0);
            return true;
        case BST_OP_MEM_FREE:
            return true;
        }
        return false;
    }
};

struct BST_UINT16_t : public BST {
    BST_UINT16_t(BST *parent) : BST(parent) { }
    unsigned short v;
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        UCHAR * ptr;
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
            ptr = str->get_ptr(2);
            if (!ptr)
                return false;
            ptr[0] = (v >>  8) & 0xFF;
            ptr[1] = (v >>  0) & 0xFF;
            return true;
        case BST_OP_CALC_SIZE:
            if (!str->get_ptr(2))
                return false;
            return true;
        case BST_OP_DECODE:
            ptr = str->get_ptr(2);
            if (!ptr)
                return false;
            v = (ptr[0] <<  8) + (ptr[1] <<  0);
            return true;
        case BST_OP_MEM_FREE:
            return true;
        }
        return false;
    }
};

struct BST_UINT8_t : public BST {
    BST_UINT8_t(BST *parent) : BST(parent) { }
    unsigned char v;
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        UCHAR * ptr;
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
            ptr = str->get_ptr(1);
            if (!ptr)
                return false;
            ptr[0] = (v >>  0) & 0xFF;
            return true;
        case BST_OP_CALC_SIZE:
            if (!str->get_ptr(1))
                return false;
            return true;
        case BST_OP_DECODE:
            ptr = str->get_ptr(1);
            if (!ptr)
                return false;
            v = (ptr[0] <<  0);
            return true;
        case BST_OP_MEM_FREE:
            return true;
        }
        return false;
    }
};

struct BST_BINARY : public BST {
    BST_BINARY(BST *parent) : BST(parent) { binary = NULL; size = 0; }
    ~BST_BINARY(void) { if (binary) delete[] binary; }
    int size;
    unsigned char * binary;
    void alloc(int c) {
        if (size == c)
            return;
        unsigned char * newbin = NULL;
        if (c > 0)
            newbin = new unsigned char[c];
        if (c > size)
        {
            memcpy(newbin, binary, size);
            memset(newbin+size, 0, c-size);
        }
        else // c < size
        {
            if (c > 0)
                memcpy(newbin, binary, c);
        }
        if (binary)
            delete[] binary;
        size = c;
        binary = newbin;
    }
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        UCHAR * ptr;
        BST_UINT16_t   count(NULL);
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
            if (!binary)
                return false;
            count.v = size;
            if (!count.bst_op(str))
                return false;
            ptr = str->get_ptr(size);
            if (!ptr)
                return false;
            memcpy(ptr, binary, size);
            return true;
        case BST_OP_CALC_SIZE:
            if (!binary)
                return false;
            if (!count.bst_op(str))
                return false;
            ptr = str->get_ptr(size);
            if (!ptr)
                return false;
            return true;
        case BST_OP_DECODE:
            if (!count.bst_op(str))
                return false;
            alloc(count.v);
            ptr = str->get_ptr(size);
            if (!ptr)
                return false;
            memcpy(binary, ptr, size);
            return true;
        case BST_OP_MEM_FREE:
            alloc(0);
            return true;
        }
        return false;
    }
};

// it is legal for string to be a null ptr; however
// it will be represented identically as a zero-length string.
struct BST_STRING : public BST {
    BST_STRING(BST *parent) : BST(parent) { string = NULL; }
    virtual ~BST_STRING(void) { if (string) delete[] string; }
    char * string;
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        UCHAR * ptr;
        BST_UINT16_t  len(NULL);
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
            if (string)
                len.v = strlen(string);
            else
                len.v = 0;
            if (!len.bst_op(str))
                return false;
            if (len.v > 0)
            {
                ptr = str->get_ptr(len.v);
                if (!ptr)
                    return false;
                memcpy(ptr, string, len.v);
            }
            return true;
        case BST_OP_CALC_SIZE:
            len.v = strlen(string);
            if (!len.bst_op(str))
                return false;
            if (len.v > 0)
                if (!str->get_ptr(len.v))
                    return false;
            return true;
        case BST_OP_DECODE:
            if (!len.bst_op(str))
                return false;
            if (string)
                delete[] string;
            string = new char[len.v + 1];
            if (len.v > 0)
            {
                ptr = str->get_ptr(len.v);
                if (!ptr)
                    return false;
                memcpy(string, ptr, len.v);
            }
            string[len.v] = 0;
            return true;
        case BST_OP_MEM_FREE:
            if (string)
                delete[] string;
            string = NULL;
            return true;
        };
        return false;
    }
    void set(char * str) {
        if (string)
            delete[] string;
        int len = strlen(str) + 1;
        string = new char[len];
        memcpy(string, str, len);
    }
};

// it is legal for pointer to be NULL pointer.
// it will be encoded as a NULL ptr and the receiver will
// set it as NULL (as expected).
template <class T>
class BST_POINTER : public BST {
public:
    BST_POINTER(BST *parent) : BST(parent) { pointer = NULL; }
    virtual ~BST_POINTER(void) { if (pointer) delete pointer; }
    T * pointer;
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        BST_UINT8_t  flag(NULL);
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
        case BST_OP_CALC_SIZE:
            flag.v = (pointer != NULL) ? 1 : 0;
            if (!flag.bst_op(str))
                return false;
            if (pointer)
                if (!pointer->bst_op(str))
                    return false;
            return true;
        case BST_OP_DECODE:
            if (!flag.bst_op(str))
                return false;
            if (flag.v == 0)
                pointer = NULL;
            else
            {
                if (!pointer)
                    pointer = new T(NULL);
                if (!pointer->bst_op(str))
                    return false;
            }
            return true;
        case BST_OP_MEM_FREE:
            if (pointer)
            {
                if (!pointer->bst_op(str))
                    return false;
                delete pointer;
            }
            pointer = NULL;
            return true;
        }
        return false;
    }
};

template <class T>
class BST_ARRAY : public BST {
public:
    BST_ARRAY(BST *parent) : BST(parent) { array = NULL; num_items = 0; }
    virtual ~BST_ARRAY(void) { bst_var_array_free(); }
    int num_items;
    T ** array;
    void alloc(int c) {
        if (c == num_items)
            return;
        int i;
        T ** narray = NULL;
        if (c > 0)
            narray = new T*[c];
        if (c > num_items) {
            for (i=0; i < num_items; i++)
                narray[i] = array[i];
            for (; i < c; i++)
                narray[i] = new T(NULL);
        } else {// c < num_items
            for (i=0; i < c; i++)
                narray[i] = array[i];
            for (;i < num_items; i++)
                delete array[i];
        }
        if (array)
            delete[] array;
        array = narray;
        num_items = c;
    }
    void bst_var_array_free(void) {
        if (array) {
            for (int i=0; i < num_items; i++)
                delete array[i];
            delete array;
            array = NULL;
            num_items = 0;
        }
    }
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        int i;
        BST_UINT32_t  count(NULL);
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
        case BST_OP_CALC_SIZE:
            count.v = num_items;
            if (!count.bst_op(str))
                return false;
            for (i=0; i < num_items; i++)
                if (!array[i]->bst_op(str))
                    return false;
            return true;
        case BST_OP_DECODE:
            if (!count.bst_op(str))
                return false;
            alloc(count.v);
            for (i=0; i < num_items; i++)
                if (!array[i]->bst_op(str))
                    return false;
            return true;
        case BST_OP_MEM_FREE:
            for (i=0; i < num_items; i++)
                if (!array[i]->bst_op(str))
                    return false;
            bst_var_array_free();
            return true;
        }
        return false;
    }
};

// base class for an object with a union in it.
#include <stdio.h>
#include <stdlib.h>

class BST_UNION : public BST {
    int max;
    BST ** fields;
    void flatten(void) {
        if (fields)
            return;
        fields = new BST*[max];
        BST * b; int i=0;
        for (b = head; b; b = b->next) {
            if (i >= max) {
            enum_mismatch:
                fprintf(stderr, "\nERROR IN UNION: enum mismatch!!\n\n");
                exit(1);
            }
            fields[i++] = b;
        }
        if (i != max) goto enum_mismatch;
    }
public:
    BST_UINT8_t  which;
    BST_UNION( BST *parent, int _max )
        : BST(parent), which(NULL) {
        max = _max; which.v = max; fields = NULL;
    }
    virtual ~BST_UNION(void) { delete[] fields; }
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        flatten();
        if (!which.bst_op(str))
            return false;
        if (which.v >= max)
            return false;
        if (!fields[which.v]->bst_op(str))
            return false;
        if (str->get_op() == BST_OP_MEM_FREE)
            which.v = max;
        return true;
    }
};

/* examples:


struct myUnion : public BST_UNION {
    enum { AYE, BEE, CEE, DEE, MAX };
    myUnion(BST *parent) :
        BST_UNION(parent,MAX),
        a(this), b(this), c(this), d(this) { }
    BST_UINT64_t  a;
    BST_UINT32_t  b;
    BST_UINT16_t  c;
    BST_UINT8_t   d;
};

struct myStruct1 : public BST {
    myStruct1(BST * parent) :
        BST(parent),
        one(this), two(this) { }
    BST_UINT32_t  one;
    BST_UINT32_t  two;
};

struct myStruct2 : public BST {
    myStruct2(BST *parent)
        : BST(parent),
          thing(this), three(this), un(this) {}
    myStruct1     thing;
    BST_UINT32_t  three;
    myUnion     un;
};

int main() {
    myStruct2  x(NULL);
    myStruct2  y(NULL);
    UCHAR * buffer;
    int len;

    x.thing.one.v = 4;
    x.thing.two.v = 8;
    x.three.v = 12;
    x.un.which.v = myUnion::CEE;
    x.un.c.v = 127;
    buffer = x.bst_encode(&len);
    if (!buffer) {
        printf("encode error\n");
        return 1;
    }
    for (int i=0; i < len; i++)
        printf("%02x ", buffer[i]);
    printf("\n");
    if (!y.bst_decode(buffer,len)) {
        printf("decode error\n");
        return 1;
    }
    printf("%d %d %d %d ", x.thing.one.v, x.thing.two.v,
           x.three.v, x.un.which.v);
    switch (x.un.which.v) {
    case myUnion::AYE:  printf("a=%lld\n", x.un.a.v); break;
    case myUnion::BEE:  printf("b=%d\n"  , x.un.b.v); break;
    case myUnion::CEE:  printf("c=%d\n"  , x.un.c.v); break;
    case myUnion::DEE:  printf("d=%d\n"  , x.un.d.v); break;
    }
    delete[] buffer;
    return 0;
}

 */

#endif /* __BYTESTREAM_H__ */
