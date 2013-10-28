
#include <stdio.h>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <pthread.h>

using namespace std;

#define DYNAMIC_FREE_LIST_DEFN()                                        \
    struct free_item {                                                  \
        struct free_item * next;                                        \
    };                                                                  \
    static free_item * dynamicFreeListHead;                             \
    static int dynamicFreeListAllocatedPeak;                            \
    static pthread_mutex_t dynamicFreeListMutex;                        \
    static void * operator new(size_t sz) {                             \
        pthread_mutex_lock(&dynamicFreeListMutex);                      \
        if (dynamicFreeListHead == NULL)                                \
        {                                                               \
            pthread_mutex_unlock(&dynamicFreeListMutex);                \
            dynamicFreeListAllocatedPeak ++;                            \
            cout << "allocator at " << __FILE__ << ":" << __LINE__      \
                 << " new peak " << dynamicFreeListAllocatedPeak << endl; \
            return (void*) ::malloc(sz);                                \
        }                                                               \
        /*else*/                                                        \
        free_item * item = dynamicFreeListHead;                         \
        dynamicFreeListHead = item->next;                               \
        pthread_mutex_unlock(&dynamicFreeListMutex);                    \
        return (void*) item;                                            \
    }                                                                   \
    static void operator delete(void *ptr) {                            \
        pthread_mutex_lock(&dynamicFreeListMutex);                      \
        free_item * item = (free_item *) ptr;                           \
        item->next = dynamicFreeListHead;                               \
        dynamicFreeListHead = item;                                     \
        pthread_mutex_unlock(&dynamicFreeListMutex);                    \
    }

#define DYNAMIC_FREE_LIST_DECLS(className)                              \
    int className::dynamicFreeListAllocatedPeak = 0;                    \
    className::free_item * className::dynamicFreeListHead = NULL;       \
    pthread_mutex_t className::dynamicFreeListMutex = PTHREAD_MUTEX_INITIALIZER;

class myBaseObj {
    int a;
public:
    myBaseObj(int _a) : a(_a) { }
    ~myBaseObj(void) { }
    void baseMethod(void) { 
        cout << "myBaseObj a = " << a << endl;
    }
};

class myDerived : public myBaseObj {
    int b;
public:
    myDerived(int _a, int _b) : myBaseObj(_a), b(_b) { }
    ~myDerived(void) { }
    void derivedMethod(void) {
        baseMethod();
        cout << "myDerived b = " << b << endl;
    }
    DYNAMIC_FREE_LIST_DEFN();
};
DYNAMIC_FREE_LIST_DECLS(myDerived);

int
main()
{
    myDerived * a, *b;

    a = new myDerived(1,2);
    b = new myDerived(3,4);

    a->derivedMethod();
    b->derivedMethod();

    delete a;
    delete b;

    a = new myDerived(1,2);
    b = new myDerived(3,4);

    a->derivedMethod();
    b->derivedMethod();

    delete a;
    delete b;

    return 0;
}
