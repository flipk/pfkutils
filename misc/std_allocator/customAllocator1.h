/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <list>
#include <iostream>
#include <pthread.h>
#include <stdlib.h>

#define MYALLOCATORVERBOSE 0
#define MYALLOCATORUSELOCKS 0
#define MYALLOCATORKEEPSTATS 1

//
// based on something i got from:
// http://www.codeproject.com/Articles/4795/C-Standard-Allocator-An-Introduction-and-Implement
//

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
    int itemSize;
    struct freeItem {
        freeItem * next;
    };
    pthread_mutex_t mutex;
    freeItem * freeList;
    inline void   lock(void) {
        if (MYALLOCATORUSELOCKS)
            pthread_mutex_lock  (&mutex);
    }
    inline void unlock(void) {
        if (MYALLOCATORUSELOCKS)
            pthread_mutex_unlock(&mutex);
    }

public:
    inline CustomAllocator() {
        pthread_mutexattr_t mattr;
        pthread_mutexattr_init(&mattr);
        pthread_mutex_init(&mutex, &mattr);
        pthread_mutexattr_destroy(&mattr);
        freeList = NULL;
        itemSize = sizeof(T);
        if (MYALLOCATORVERBOSE)
            std::cout << "initializing CustomAllocator, item size = "
                      << itemSize << std::endl;
    }
    inline ~CustomAllocator() {
        if (MYALLOCATORVERBOSE)
            std::cout << "cleanup in CustomAllocator" << std::endl;
        for (freeItem * it = freeList; freeList; it = freeList)
        {
            freeList = it->next;
            pointer p = reinterpret_cast<pointer>(it);
            ::operator delete(p);
        }
        pthread_mutex_destroy(&mutex);
    }
    template<typename U>
    inline CustomAllocator(CustomAllocator<U> const &_other) {
        pthread_mutexattr_t mattr;
        pthread_mutexattr_init(&mattr);
        pthread_mutex_init(&mutex, &mattr);
        pthread_mutexattr_destroy(&mattr);
        freeList = NULL;
        itemSize = sizeof(T);
        if (MYALLOCATORVERBOSE)
            std::cout << "initializing CustomAllocator, item size = "
                      << itemSize << std::endl;
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
                          << " T = " << sizeof(T) << std::endl;
            return reinterpret_cast<pointer>(::malloc(cnt * sizeof(T)));
        }
        if (MYALLOCATORVERBOSE)
            std::cout << "CustomAllocator alloc cnt="
                      << cnt << " sizeof(T)=" << sizeof(T) << std::endl;
        lock();
        pointer ret = NULL;
        if (freeList == NULL)
        {
            if (MYALLOCATORVERBOSE)
                std::cout << "CustomAllocator allocating new obj" << std::endl;
            ret = reinterpret_cast<pointer>(::operator new(cnt * sizeof (T))); 
        }
        else
        {
            if (MYALLOCATORVERBOSE)
                std::cout << "CustomAllocator reusing existing obj "
                    "from freeList"
                          << std::endl;
            freeItem * it = freeList;
            freeList = it->next;
            ret = reinterpret_cast<pointer>(it);
        }
        unlock();
        return ret;
    }
    inline void deallocate(pointer p, size_type cnt) { 
        if (MYALLOCATORVERBOSE)
            std::cout << "CustomAllocator dealloc cnt="
                      << cnt << " sizeof(T)=" << sizeof(T) << std::endl;
        if (cnt != 1)
        {
            if (MYALLOCATORVERBOSE)
                std::cout << "CustomAllocator::deallocate "
                          << "cnt = " << cnt
                          << " T = " << sizeof(T) << std::endl;
            ::free(p);
            return;
        }
        lock();
        freeItem * it = (freeItem *) p;
        it->next = freeList;
        freeList = it;
        unlock();
    }

    //    construction/destruction
    inline void construct(pointer p, const T& t) { new(p) T(t); }
    inline void destroy(pointer p) { p->~T(); }

    inline bool operator==(CustomAllocator const&) { return true; }
    inline bool operator!=(CustomAllocator const& a) { return false; }
};

#define MAP_WITH_ALLOCATOR(keyType,dataType) \
    map<keyType,dataType, std::less<keyType>, \
        CustomAllocator<pair<keyType,dataType> > > 

#define UMAP_WITH_ALLOCATOR(keyType,dataType) \
    unordered_map<keyType,dataType,std::hash<keyType>, \
                  std::equal_to<keyType>, \
                  CustomAllocator<std::pair<keyType,dataType> > > 
