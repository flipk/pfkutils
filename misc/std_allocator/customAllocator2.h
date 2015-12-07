/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <list>
#include <map>
#include <pthread.h>

#define MYALLOCATORVERBOSE 0
#define MYALLOCATORUSELOCKS 0
#define MYALLOCATORKEEPSTATS 1
#define MYALLOCATORPAGESIZE 65536

class MyAllocator {
public:
    class MyAllocatorPool {
        int bufSize;
        int peak;
        int freeSize;
        MyAllocator *mya;
        struct poolItem {
            struct poolItem * next;
        };
        poolItem * freeList;
        void init(int _bufSize, MyAllocator *_mya) {
            bufSize = _bufSize;
            mya = _mya;
        }
        friend class MyAllocator;
        pthread_mutex_t mutex;
        void   lock(void) {
            if (MYALLOCATORUSELOCKS)
                pthread_mutex_lock  (&mutex);
        }
        void unlock(void) {
            if (MYALLOCATORUSELOCKS)
                pthread_mutex_unlock(&mutex);
        }
    public:
        MyAllocatorPool(void) {
            bufSize = 0;
            freeList = NULL;
            mya = NULL;
            peak = freeSize = 0;
            pthread_mutexattr_t mattr;
            pthread_mutexattr_init(&mattr);
            pthread_mutex_init(&mutex, &mattr);
            pthread_mutexattr_destroy(&mattr);
        }
        ~MyAllocatorPool(void) {
            pthread_mutex_destroy(&mutex);
        }
        int getBufSize(void) { return bufSize; }
        void * alloc(void) {
            void * ret;
            lock();
            if (freeList) {
                poolItem * it = freeList;
                freeList = it->next;
                ret = (void*) it;
                if (MYALLOCATORKEEPSTATS)
                    freeSize--;
                unlock();
                if (MYALLOCATORVERBOSE)
                    std::cout << "alloc from pool " << bufSize 
                              << "reuses buffer at " << (uintptr_t) ret
                              << std::endl;
            } else {
                if (MYALLOCATORKEEPSTATS)
                    peak++;
                unlock();
                ret = mya->getMemory(bufSize);
                if (MYALLOCATORVERBOSE)
                    std::cout << "alloc from pool " << bufSize 
                              << " gets new buffer at " << (uintptr_t) ret
                              << std::endl;
            }
            return ret;
        }
        void free(void *ptr) {
            lock();
            poolItem * it = (poolItem *) ptr;
            it->next = freeList;
            freeList = it;
            if (MYALLOCATORKEEPSTATS)
                freeSize++;
            unlock();
            if (MYALLOCATORVERBOSE)
                std::cout << "buffer at " << (uintptr_t) ptr
                          << " released to pool " << bufSize
                          << std::endl;
        }
        void dumpStats(void) {
            std::cout << "pool " << bufSize
                      << " peak " << peak
                      << " freeSize " << freeSize << std::endl;
        }
    };
private:
    struct pageInfo {
        char addr[MYALLOCATORPAGESIZE];
        int pos;
        int remaining;
        pageInfo(void) {
            remaining = MYALLOCATORPAGESIZE;
            pos = 0;
        }
        void dumpStats(void) {
            std::cout << "page at " << (uintptr_t) addr
                      << " pos " << pos << " remaining " << remaining
                      << std::endl;
        }
    };
    std::list<pageInfo> pagesList;
    pageInfo * currentPage;
    std::map<int,MyAllocatorPool> poolsMap;
    pthread_mutex_t mutex;
    void   lock(void) {
        if (MYALLOCATORUSELOCKS)
            pthread_mutex_lock  (&mutex);
    }
    void unlock(void) {
        if (MYALLOCATORUSELOCKS)
            pthread_mutex_unlock(&mutex);
    }
    void * getMemory(int size) {
        lock();
        if (currentPage == NULL  ||
            currentPage->remaining < size)
        {
            pagesList.push_back(pageInfo());
            currentPage = &(pagesList.back());
        }
        int pos = currentPage->pos;
        char * ret = currentPage->addr + pos;
        currentPage->pos += size;
        currentPage->remaining -= size;
        unlock();
        if (MYALLOCATORVERBOSE)
            std::cout << "getMemory of size " << size 
                      << " at pos " << pos
                      << " of page at " << (uintptr_t) currentPage->addr
                      << " gives memory at " << (uintptr_t) ret
                      << std::endl;
        return (void *)ret;
    }
public:
    MyAllocator(void)
        : currentPage(NULL) {
        pthread_mutexattr_t mattr;
        pthread_mutexattr_init(&mattr);
        pthread_mutex_init(&mutex, &mattr);
        pthread_mutexattr_destroy(&mattr);
    }
    ~MyAllocator(void) {
        pthread_mutex_destroy(&mutex);
    }
    MyAllocatorPool * getPool(int bufSize) {
        if (MYALLOCATORVERBOSE)
            std::cout << "request for pool of bufSize " << bufSize
                      << std::endl;
        lock();
        MyAllocator::MyAllocatorPool * ret = &(poolsMap[bufSize]);
        if (ret->getBufSize() == 0)
            ret->init(bufSize, this);
        unlock();
        return ret;
    }
    void dumpStats(void) {
        std::list<pageInfo>::iterator pageIt;
        std::map<int,MyAllocatorPool>::iterator poolIt;
        lock();
        std::cout << pagesList.size() << " pages : " << std::endl;
        for (pageIt = pagesList.begin(); pageIt != pagesList.end(); pageIt++)
            (*pageIt).dumpStats();
        std::cout << poolsMap.size() << " pools : " << std::endl;
        for (poolIt = poolsMap.begin(); poolIt != poolsMap.end(); poolIt++)
            poolIt->second.dumpStats();

        unlock();
    }
};

extern MyAllocator __customAllocatorBackend;

template<typename T>
class CustomAllocator {
public : 
    //    typedefs
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    //    convert an allocator<T> to allocator<U>
    template<typename U>
    struct rebind {
        typedef CustomAllocator<U> other;
    };

private:
    int allocSize;
    MyAllocator::MyAllocatorPool * customPool;

public:
    inline CustomAllocator() {
        allocSize = sizeof(T);
        customPool = __customAllocatorBackend.getPool(allocSize);
    }
    inline ~CustomAllocator() { }
    template<typename U>
    inline CustomAllocator(CustomAllocator<U> const &_other) {
        allocSize = sizeof(T);
        customPool = __customAllocatorBackend.getPool(allocSize);
        if (MYALLOCATORVERBOSE)
            std::cout << "sizeof T = " << sizeof(T)
                      << " but sizeof U = " << sizeof(U) << std::endl;
    }

    //    address
    inline pointer address(reference r) { return &r; }
    inline const_pointer address(const_reference r) { return &r; }

    //    memory allocation
    inline pointer allocate(size_type cnt, 
                            typename std::allocator<void>::const_pointer = 0) { 
        if (cnt != 1)
        {
            if (MYALLOCATORVERBOSE)
                std::cout << "CustomAllocator::allocate "
                          << "cnt = " << cnt
                          << " T = " << allocSize << std::endl;
            return reinterpret_cast<pointer>(::malloc(cnt * allocSize));
        }
        return reinterpret_cast<pointer>(customPool->alloc());
    }
    inline void deallocate(pointer p, size_type cnt) { 
        if (cnt != 1)
        {
            if (MYALLOCATORVERBOSE)
                std::cout << "CustomAllocator::deallocate "
                          << "cnt = " << cnt
                          << " T = " << allocSize << std::endl;
            ::free(p);
        }
        else
            customPool->free(p);
    }

    //    construction/destruction
    inline void construct(pointer p, const T& t) { new(p) T(t); }
    inline void destroy(pointer p) { p->~T(); }

    inline bool operator==(CustomAllocator const&) { return true; }
    inline bool operator!=(CustomAllocator const& a) { return false; }
};
