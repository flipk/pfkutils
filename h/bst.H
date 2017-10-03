/*
 * This code is originally written by Phillip F Knaack.
 * There are absolutely no restrictions on the use of this code.  It is
 * NOT GPL!  If you use this code in a product, you do NOT have to release
 * your alterations, nor do you have to acknowledge in any way that you
 * are using this software.
 * The only thing I ask is this: do not claim you wrote it.  My name
 * should appear in this file forever as the original author, and every
 * person who modifies this file after me should also place their names
 * in this file (so those that follow know who broke what).
 * This restriction is not enforced in any way by any license terms, it
 * is merely a personal request from me to you.  If you wanted, you could
 * even completely remove this entire comment and place a new one with your
 * company's confidential proprietary license on it-- but if you are a good
 * internet citizen, you will comply with my request out of the goodness
 * of your heart.
 * If you do use this code and you make a buttload of money off of it,
 * I would appreciate a little kickback-- but again this is a personal
 * request and is not required by any licensing of this code.  (Of course
 * in the offchance that anyone every actually DOES make a lot of money 
 * using this code, I know I'm going to regret that statement, but at
 * this point in time it feels like the right thing to say.)
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

#ifndef __BYTESTREAM_H__
#define __BYTESTREAM_H__

// ByteStream interface (BST)

#include <string.h>
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
    BST_OP  op;
    UCHAR * buffer;
    int     buffer_size;
    UCHAR * ptr;
    int     size;
    int     remaining;
    bool    my_buffer;
public:
    // the user supplies the buffer and also takes care
    // of deleting it.
    BST_STREAM( UCHAR * _buffer, int _size ) {
        op = (BST_OP)0; // BST_OP_NONE;
        buffer = _buffer;
        buffer_size = _size;
        my_buffer = false;
    }
    // this object creates the buffer, which is deleted when
    // this object is deleted.
    BST_STREAM( int _size ) {
        op = (BST_OP)0; // BST_OP_NONE;
        buffer = new UCHAR[_size];
        buffer_size = _size;
        my_buffer = true;
    }
    ~BST_STREAM(void) {
        if (my_buffer)
            delete[] buffer;
    }
    // reset the current buffer position to the beginning, used
    // to reset the stream state for beginning a new operation.
    void start(BST_OP _op) {
        op = _op; 
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
    // return the inprogress operation.  used by bst_op in all data types.
    BST_OP get_op(void) { return op; }
    // return the current working pointer.  used by bst_op in all 
    // data types as the bytestream is processed.  specify the number
    // of bytes needed.  if there are not enough left in the buffer,
    // returns NULL.
    UCHAR * get_ptr(int step) {
        if (remaining < step)
            return NULL;
        UCHAR * ret = ptr;
        remaining -= step;
        size += step;
        ptr += step;
        return ret;
    }
};

// base class for any type which can be represented as a ByteStream
class BST {
protected:
    bool bst_do_fields( BST_STREAM *str, BST ** fields ) {
        for (int i=0; fields[i] != NULL; i++)
            if (!fields[i]->bst_op(str))
                return false;
        return true;
    }
public:
    virtual ~BST(void) { /* placeholder */ }
    // return true if operation was successful; 
    // return false if some problem (out of buffer, etc)
    virtual bool bst_op( BST_STREAM *str ) = 0;
};

// BST base types follow.

struct BST_UINT64_t : public BST {
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

// it is legal for string to be a null ptr; however
// it will be represented identically as a zero-length string.
struct BST_STRING : public BST {
    BST_STRING(void) { string = NULL; }
    virtual ~BST_STRING(void) { if (string) delete[] string; }
    char * string;
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        UCHAR * ptr;
        BST_UINT32_t  len;
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
};

// it is legal for pointer to be NULL pointer.
// it will be encoded as a NULL ptr and the receiver will
// set it as NULL (as expected).
template <class T>
class BST_POINTER : public BST {
public:
    BST_POINTER(void) { pointer = NULL; }
    virtual ~BST_POINTER(void) { if (pointer) delete pointer; }
    T * pointer;
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        BST_UINT8_t  flag;
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
                pointer = new T;
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

template <class T, int dim>
class BST_FIXED_ARRAY : public BST {
public:
    T array[dim];
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        for (int i=0; i < dim; i++)
            if (!array[i].bst_op(str))
                return false;
        return true;
    }
};

template <class T>
class BST_VAR_ARRAY : public BST {
public:
    BST_VAR_ARRAY(void) { array = NULL; num_items = 0; }
    virtual ~BST_VAR_ARRAY(void) { free(); }
    int num_items;
    T ** array;
    void alloc(int c) {
        if (c == num_items)
            return;
        int i;
        T ** narray = new T*[c];
        if (c > num_items)
        {
            for (i=0; i < num_items; i++)
                narray[i] = array[i];
            for (; i < c; i++)
                narray[i] = new T;
        }
        else // c < num_items
        {
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
    void free(void) {
        if (array)
        {
            for (int i=0; i < num_items; i++)
                delete array[i];
            delete array;
            array = NULL;
            num_items = 0;
        }
    }
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        int i;
        BST_UINT32_t  count;
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
            free();
            return true;
        }
        return false;
    }
};

// base class for an object with a union in it.

class BST_UNION : public BST {
public:
    bool bst_do_union( BST_STREAM *str, BST_UINT8_t *which,
                       int max_which, BST ** fields ) {
        if (!which->bst_op(str))
            return false;
        if (which->v >= max_which)
            return false;
        if (!fields[which->v]->bst_op(str))
            return false;
        return true;
    }
};

/* example class using a union:

   class example_union : public BST_UNION {
   public:
   enum { ONE, TWO, MAX };  // values assigned to 'which'
   BST_UINT8_t   which; // indicates which field in the union is in use
   // union fields start here; note we don't actually use "union" keyword;
   // downside is this uses more space on the transmitter and receiver,
   // upside is bytestream does not.
   BST_UINT32_t  one;
   BST_STRING    two;
   // union fields end here
   /@virtual@/ bool bst_op( BST_STREAM *str ) {
   // values in enum correspond to indexes in this array.
   BST * fields[] = { &one, &two };
   return bst_do_union(str,&which,MAX,fields);
   }
   };

*/

#endif /* __BYTESTREAM_H__ */
