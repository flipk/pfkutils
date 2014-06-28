/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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


template <HASH_TEMPL>
void HASH::lockwarn(void) throw (ListError)
{
    if (lockWarn == true && isLocked() == false)
        LISTERR(LIST_NOT_LOCKED);
}

template <HASH_TEMPL>
HASH::Links::Links(void) throw ()
{
    magic = MAGIC;
}

template <HASH_TEMPL>
void HASH::Links::checkvalid(void) throw (ListError)
{
    if (validate && (magic != MAGIC))
        LISTERR(ITEM_NOT_VALID);
}

template <HASH_TEMPL>
HASH::Links::~Links(void) throw (ListError)
{
    if (validate && hsh != NULL)
        LISTERR(LIST_NOT_EMPTY);
    magic = 0;
}

template <HASH_TEMPL>
HASH :: Hash(void) throw ()
{
    hashsize = dll3_hash_primes[0];
    hash.reserve(hashsize);
}

template <HASH_TEMPL>
HASH :: ~Hash(void) throw (ListError)
{
}

template <HASH_TEMPL>
void HASH :: add(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid();
    T * derived = dynamic_cast<T*>(item);
    item->h = HashT::obj2hash(*derived) % hashsize;
    hash[item->h].add_tail(derived);
    item->hsh = this;
    // xxx rehashing 
}

template <HASH_TEMPL>
void HASH :: remove(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid();
    T * derived = dynamic_cast<T*>(item);
    hash[item->h].remove(derived);
    item->hsh = NULL;
    // xxx rehashing
}

template <HASH_TEMPL>
T * HASH :: find(const KeyT &key) throw (ListError)
{
    lockwarn();
    uint32_t h = HashT::key2hash(key) % hashsize;
    theHash &lst = hash[h];
    for (T * item = lst.get_head();
         item != NULL;
         item = lst.get_next(item))
    {
        if (HashT::hashMatch(*item,key))
            return item;
    }
    return NULL;
}
