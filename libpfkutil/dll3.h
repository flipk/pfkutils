/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __DLL3_H__
#define __DLL3_H__

#include <iostream>
#include <pthread.h>

#include "LockWait.h"
#include "throwBacktrace.h"

namespace DLL3 {

struct ListError : PFK::ThrowBackTrace {
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

#define LIST List<T,uniqueIdentifer,lockWarn,validate>
#define LIST_TEMPL class T, int uniqueIdentifer, bool lockWarn, bool validate
#define LISTERR(e) throw ListError(ListError::e)

template <class T, int uniqueIdentifer,
          bool lockWarn=true, bool validate=true>
class List : public PFK::Lockable {
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

#include "dll3.tcc"

#undef  LIST
#undef  LIST_TEMPL
#undef  LISTERR

}; // namespace DLL3

#endif /* __DLL3_H__ */
