/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

template <__DLL3_LIST_TEMPL>
__DLL3_LIST::Links::Links(void) throw ()
{
    next = prev = NULL;
    lst = NULL;
    magic = MAGIC;
}

template <__DLL3_LIST_TEMPL>
__DLL3_LIST::Links::~Links(void) throw (ListError)
{
    if (validate && lst != NULL)
        __DLL3_LISTERR(STILL_ON_A_LIST);
    magic = 0;
}

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::Links::checkvalid(__DLL3_LIST * _lst) throw (ListError)
{
    if (!validate)
        return;
    if (magic != MAGIC)
        __DLL3_LISTERR(ITEM_NOT_VALID);
    if (_lst == NULL && lst != NULL)
        __DLL3_LISTERR(ALREADY_ON_LIST);
    else if (lst != _lst)
        __DLL3_LISTERR(NOT_ON_THIS_LIST);
}

template <__DLL3_LIST_TEMPL>
__DLL3_LIST::List(void) throw ()
{
    head = tail = NULL;
    cnt = 0;
}

template <__DLL3_LIST_TEMPL>
__DLL3_LIST::~List(void) throw (ListError)
{
    if (validate && head != NULL)
        __DLL3_LISTERR(LIST_NOT_EMPTY);
}

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::lockwarn(void) throw (ListError)
{
    if (lockWarn == true && isLocked() == false)
        __DLL3_LISTERR(LIST_NOT_LOCKED);
}

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::add_head(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid(NULL);
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

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::add_tail(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid(NULL);
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

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::add_before(Links * item, Links * existing) throw (ListError)
{
    lockwarn();
    item->checkvalid(NULL);
    existing->checkvalid(this);
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

template <__DLL3_LIST_TEMPL>
T * __DLL3_LIST::get_head(void) throw (ListError)
{
    lockwarn();
    return static_cast<T*>(head);
}

template <__DLL3_LIST_TEMPL>
T * __DLL3_LIST::get_tail(void) throw (ListError)
{
    lockwarn();
    return static_cast<T*>(tail);
}

template <__DLL3_LIST_TEMPL>
T * __DLL3_LIST::get_next(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid(this);
    return static_cast<T*>(item->next);
}

template <__DLL3_LIST_TEMPL>
T * __DLL3_LIST::get_prev(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid(this);
    return static_cast<T*>(item->prev);
}

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::_remove(Links * item) throw ()
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

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::remove(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid(this);
    _remove(item);
}

template <__DLL3_LIST_TEMPL>
T * __DLL3_LIST::dequeue_head(void) throw (ListError)
{
    lockwarn();
    Links * item = head;
    if (item)
        _remove(item);
    return static_cast<T*>(item);
}

template <__DLL3_LIST_TEMPL>
T * __DLL3_LIST::dequeue_tail(void) throw (ListError)
{
    lockwarn();
    Links * item = head;
    if (item)
        _remove(item);
    return static_cast<T*>(item);
}


template <__DLL3_HASH_TEMPL>
void __DLL3_HASH::lockwarn(void) throw (ListError)
{
    if (lockWarn == true && isLocked() == false)
        __DLL3_LISTERR(LIST_NOT_LOCKED);
}

template <__DLL3_HASH_TEMPL>
__DLL3_HASH::Links::Links(void) throw ()
{
    magic = MAGIC;
}

template <__DLL3_HASH_TEMPL>
void __DLL3_HASH::Links::checkvalid(__DLL3_HASH * _hsh) throw (ListError)
{
    if (validate && (magic != MAGIC))
        __DLL3_LISTERR(ITEM_NOT_VALID);
    if (_hsh == NULL && hsh != NULL)
        __DLL3_LISTERR(ALREADY_ON_LIST);
    else if (hsh != _hsh)
        __DLL3_LISTERR(NOT_ON_THIS_LIST);
}

template <__DLL3_HASH_TEMPL>
__DLL3_HASH::Links::~Links(void) throw (ListError)
{
    if (validate && hsh != NULL)
        __DLL3_LISTERR(LIST_NOT_EMPTY);
    magic = 0;
}

template <__DLL3_HASH_TEMPL>
__DLL3_HASH :: Hash(void) throw ()
{
    hashorder = 0;
    hashsize = dll3_hash_primes[hashorder];
    hash = new std::vector<theHash>;
    hash->reserve(hashsize);
    cnt = 0;
}

template <__DLL3_HASH_TEMPL>
__DLL3_HASH :: ~Hash(void) throw (ListError)
{
    delete hash;
}

template <__DLL3_HASH_TEMPL>
void __DLL3_HASH :: add(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid(NULL);
    T * derived = dynamic_cast<T*>(item);
    item->h = HashT::obj2hash(*derived) % hashsize;
    (*hash)[item->h].add_tail(derived);
    item->hsh = this;
    cnt++;
    rehash();
}

template <__DLL3_HASH_TEMPL>
void __DLL3_HASH :: remove(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid(NULL);
    T * derived = dynamic_cast<T*>(item);
    (*hash)[item->h].remove(derived);
    item->hsh = NULL;
    cnt--;
    rehash();
}

template <__DLL3_HASH_TEMPL>
T * __DLL3_HASH :: find(const KeyT &key) throw (ListError)
{
    lockwarn();
    uint32_t h = HashT::key2hash(key) % hashsize;
    theHash &lst = (*hash)[h];
    for (T * item = lst.get_head();
         item != NULL;
         item = lst.get_next(item))
    {
        if (HashT::hashMatch(*item,key))
            return item;
    }
    return NULL;
}

template <__DLL3_HASH_TEMPL>
void __DLL3_HASH :: rehash(void) throw (ListError)
{
    int average = cnt / hashsize;
    if (average  > 5 && hashorder < dll3_num_hash_primes)
        _rehash(hashorder+1);
    if (average == 0 && hashorder > 0)
        _rehash(hashorder-1);
}

template <__DLL3_HASH_TEMPL>
void __DLL3_HASH :: _rehash(int newOrder)
{
    std::vector<theHash> * oldHash = hash;
    int oldHashSize = hashsize;
    hashorder = newOrder;
    hashsize = dll3_hash_primes[newOrder];
    hash = new std::vector<theHash>;
    hash->reserve(hashsize);
    for (int h = 0; h < oldHashSize; h++)
        while ( T * t = (*oldHash)[h].dequeue_head())
            add( t );
    delete oldHash;
}


template <__DLL3_HASHLRU_TEMPL>
__DLL3_HASHLRU :: Links :: Links(void) throw ()
{
    magic = MAGIC;
}

template <__DLL3_HASHLRU_TEMPL>
__DLL3_HASHLRU :: Links :: ~Links(void) throw (ListError)
{
    if (validate && (hlru != NULL))
        __DLL3_LISTERR(LIST_NOT_EMPTY);
    magic = 0;
}

template <__DLL3_HASHLRU_TEMPL>
void __DLL3_HASHLRU :: Links :: checkvalid(__DLL3_HASHLRU * _hlru)
    throw (ListError)
{
    if (validate && (magic != MAGIC))
        __DLL3_LISTERR(ITEM_NOT_VALID);
    if (_hlru == NULL && hlru != NULL)
        __DLL3_LISTERR(ALREADY_ON_LIST);
    else if (hlru != _hlru)
        __DLL3_LISTERR(NOT_ON_THIS_LIST);
}

template <__DLL3_HASHLRU_TEMPL>
void __DLL3_HASHLRU :: lockwarn(void) throw (ListError)
{
    if (lockWarn == true && isLocked() == false)
        __DLL3_LISTERR(LIST_NOT_LOCKED);
}

template <__DLL3_HASHLRU_TEMPL>
void __DLL3_HASHLRU :: add(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid(NULL);
    T * derived = dynamic_cast<T*>(item);
    list.add_tail(derived);
    hash.add(derived);
    item->hlru = this;
}

template <__DLL3_HASHLRU_TEMPL>
void __DLL3_HASHLRU :: remove(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid(this);
    T * derived = dynamic_cast<T*>(item);
    list.remove(derived);
    hash.remove(derived);
    item->hlru = NULL;
}

template <__DLL3_HASHLRU_TEMPL>
void __DLL3_HASHLRU :: promote(Links * item) throw (ListError)
{
    lockwarn();
    item->checkvalid(this);
    T * derived = dynamic_cast<T*>(item);
    list.remove(derived);
    list.add_tail(derived);
}

template <__DLL3_HASHLRU_TEMPL>
T * __DLL3_HASHLRU :: find(const KeyT &key) throw (ListError)
{
    lockwarn();
    return hash.find(key);
}

template <__DLL3_HASHLRU_TEMPL>
T * __DLL3_HASHLRU :: get_oldest(void) throw (ListError)
{
    lockwarn();
    return list.get_head();
}
