
#ifndef __DLL3_LIST_H__
#define __DLL3_LIST_H__

namespace DLL3 {

struct ListError {
    enum ListErrValue {
        ITEM_NOT_VALID,
        ALREADY_ON_LIST,
        STILL_ON_A_LIST,
        LIST_NOT_EMPTY,
        LIST_NOT_LOCKED,
        NOT_ON_THIS_LIST
    } err;
    ListError(ListErrValue _e) : err(_e) { }
};

#define LIST List<T,uniqueIdentifer,lockWarn,validate>
#define LIST_TEMPL class T, int uniqueIdentifer, bool lockWarn, bool validate
#define LISTERR(e) throw ListError(ListError::e)

template <class T, int uniqueIdentifer,
          bool lockWarn=true, bool validate=true>
class List : public Lockable {
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

// TCC impl below this line

template <LIST_TEMPL>
LIST::Links::Links(void) throw ()
{
    next = prev = NULL;
    lst = NULL;
    magic = MAGIC;
}

template <LIST_TEMPL>
LIST::Links::~Links(void) throw (ListError)
{
    if (validate && lst != NULL)
        LISTERR(STILL_ON_A_LIST);
    magic = 0;
}

template <LIST_TEMPL>
void LIST::Links::checkvalid(void) throw (ListError)
{
    if (validate && (magic != MAGIC))
        LISTERR(ITEM_NOT_VALID);
}

template <LIST_TEMPL>
LIST::List(void) throw ()
{
    head = tail = NULL;
    cnt = 0;
}

template <LIST_TEMPL>
LIST::~List(void) throw (ListError)
{
    if (validate && head != NULL)
        LISTERR(LIST_NOT_EMPTY);
}

template <LIST_TEMPL>
void LIST::lockwarn(void) throw (ListError)
{
    if (lockWarn == true && isLocked() == false)
        LISTERR(LIST_NOT_LOCKED);
}

template <LIST_TEMPL>
void LIST::add_head(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid();
    if (validate && item->lst != NULL)
        LISTERR(ALREADY_ON_LIST);
    item->next = head;
    item->prev = NULL;
    if (head)
        head->prev = item;
    else
        tail = item;
    head = item;
    cnt++;
    item->lst = this;
}

template <LIST_TEMPL>
void LIST::add_tail(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid();
    if (validate && item->lst != NULL)
        LISTERR(ALREADY_ON_LIST);
    item->next = NULL;
    item->prev = tail;
    if (tail)
        tail->next = item;
    else
        head = item;
    tail = item;
    cnt++;
    item->lst = this;
}

template <LIST_TEMPL>
void LIST::add_before(Links * item, Links * existing) throw (ListError)
{
    lockwarn();
    item->checkvalid();
    if (validate && item->lst != NULL)
        LISTERR(ALREADY_ON_LIST);
    if (validate && existing->lst != this)
        LISTERR(NOT_ON_THIS_LIST);
    item->prev = existing->prev;
    item->next = existing;
    existing->prev = item;
    if (item->prev)
        item->prev->next = item;
    else
        head = item;
    cnt++;
    item->lst = this;
}

template <LIST_TEMPL>
T * LIST::get_head(void) throw (ListError)
{
    lockwarn();
    return static_cast<T*>(head);
}

template <LIST_TEMPL>
T * LIST::get_tail(void) throw (ListError)
{
    lockwarn();
    return static_cast<T*>(tail);
}

template <LIST_TEMPL>
T * LIST::get_next(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid();
    if (validate && item->lst != this)
        LISTERR(NOT_ON_THIS_LIST);
    return static_cast<T*>(item->next);
}

template <LIST_TEMPL>
T * LIST::get_prev(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid();
    if (validate && item->lst != this)
        LISTERR(NOT_ON_THIS_LIST);
    return static_cast<T*>(item->prev);
}

template <LIST_TEMPL>
void LIST::_remove(Links * item) throw ()
{
    if (item->next)
        item->next->prev = item->prev;
    else
        tail = item->prev;
    if (item->prev)
        item->prev->next = item->next;
    else
        head = item->next;
    item->next = item->prev = NULL;
    cnt--;
    item->lst = NULL;
}

template <LIST_TEMPL>
void LIST::remove(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid();
    if (validate && item->lst != this)
        LISTERR(NOT_ON_THIS_LIST);
    _remove(item);
}

template <LIST_TEMPL>
T * LIST::dequeue_head(void) throw (ListError)
{
    lockwarn();
    Links * item = head;
    if (item)
        _remove(item);
    return static_cast<T*>(item);
}

template <LIST_TEMPL>
T * LIST::dequeue_tail(void) throw (ListError)
{
    lockwarn();
    Links * item = head;
    if (item)
        _remove(item);
    return static_cast<T*>(item);
}

#undef  LIST
#undef  LIST_TEMPL
#undef  LISTERR

}; // namespace DLL3

#endif /* __DLL3_LIST_H__ */
