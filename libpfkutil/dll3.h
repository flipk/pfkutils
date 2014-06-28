/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __DLL3_H__
#define __DLL3_H__

#include <iostream>
#include <vector>
#include <pthread.h>
#include <inttypes.h>

#include "LockWait.h"
#include "throwBacktrace.h"

namespace DLL3 {

struct ListError : ThrowUtil::ThrowBackTrace {
    enum ListErrValue {
        ITEM_NOT_VALID,
        ALREADY_ON_LIST,
        STILL_ON_A_LIST,
        LIST_NOT_EMPTY,
        LIST_NOT_LOCKED,
        NOT_ON_THIS_LIST,
        __NUMERRS
    } err;
    static const std::string errStrings[__NUMERRS];
    ListError(ListErrValue _e) : err(_e) { }
    const std::string Format(void) const;
};

#define LIST List<T,uniqueIdentifier,lockWarn,validate>
#define LIST_TEMPL class T, int uniqueIdentifier, bool lockWarn, bool validate
#define LISTERR(e) throw ListError(ListError::e)

template <class T, int uniqueIdentifier,
          bool lockWarn=true, bool validate=true>
class List : public WaitUtil::Lockable {
public:
    class Links {
        friend class LIST;
        static const int MAGIC = 0x5e061ed;
        int magic;
        Links * next;
        Links * prev;
        LIST * lst;
    public:
        Links(void) throw ();
        ~Links(void) throw (ListError);
        void checkvalid(void) throw (ListError);
    };
private:
    Links * head;
    Links * tail;
    int cnt;
    void lockwarn(void) throw (ListError);
    void _remove(Links * item) throw ();
public:
    List(void) throw ();
    ~List(void) throw (ListError);
    void add_head(Links * item) throw (ListError);
    void add_tail(Links * item) throw (ListError);
    void add_before(Links * item, Links * existing) throw (ListError);
    T * get_head(void) throw (ListError);
    T * get_tail(void) throw (ListError);
    T * get_next(Links * item) throw (ListError);
    T * get_prev(Links * item) throw (ListError);
    T * dequeue_head(void) throw (ListError);
    T * dequeue_tail(void) throw (ListError);
    void remove(Links * item) throw (ListError);
};

#define HASH Hash<T,KeyT,HashT,uniqueIdentifier,lockWarn,validate>
#define HASH_TEMPL class T, class KeyT, class HashT, int uniqueIdentifier, bool lockWarn, bool validate

static const int dll3_num_hash_primes = 16;
extern const int dll3_hash_primes[dll3_num_hash_primes];

// HashT must look like this:
//     class HashT {
//     public:
//        static uint32_t obj2hash  (const T &)               { /*stuff*/ }
//        static uint32_t key2hash  (const KeyT &)            { /*stuff*/ }
//        static bool     hashMatch (const T &, const KeyT &) { /*stuff*/ }
//     }
// (where T and KeyT are replaced by the real T and KeyT types.

template <class T, class KeyT, class HashT, int uniqueIdentifier,
          bool lockWarn=true, bool validate=true>
class Hash : public WaitUtil::Lockable {
    typedef List<T,uniqueIdentifier,false,true> theHash;
    std::vector<theHash> hash;
    int hashsize;
    void lockwarn(void) throw (ListError);
public:
    class Links : public List<T,uniqueIdentifier,false,true>::Links {
        friend class HASH;
        static const int MAGIC = 0x68ddd8d;
        int magic;
        HASH * hsh;
        uint32_t h;
        void checkvalid(void) throw (ListError);
    public:
        Links(void) throw ();
        virtual ~Links(void) throw (ListError);
    };
    Hash(void) throw ();
    ~Hash(void) throw (ListError);
    void add(Links * item) throw (ListError);
    void remove(Links * item) throw (ListError);
    T * find(const KeyT &key) throw (ListError);
};

#include "dll3.tcc"

#undef  HASH
#undef  HASH_TEMPL
#undef  LIST
#undef  LIST_TEMPL
#undef  LISTERR

}; // namespace DLL3

#endif /* __DLL3_H__ */
