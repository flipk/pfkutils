#if 0
set -e -x
g++ -O3 -std=c++11 -c customAllocator1.cc
g++ -O3 -std=c++11 customAllocator1.o -lpthread -o ca1
./ca1
exit 0
#endif

#include "customAllocator1.h"

using namespace std;

void * operator new(size_t sz) {
    void * ret = ::malloc(sz);
    cout << "operator new allocating " << sz << " bytes at "
         << (unsigned long) ret << endl;
    return ret;
}

void operator delete(void * ptr) {
    cout << "operator delete freeing memory at "
         << (unsigned long) ptr << endl;
    ::free(ptr);
}

struct ThingType {
    ThingType(void) {
        val = -1;
        cout << "default construct ThingType at "
             << ((unsigned long)this)
             << " with value "
             << val << endl;
    }
    ThingType(const ThingType &other) {
        val = other.val;
        cout << "copy construct ThingType at "
             << ((unsigned long)this)
             << " from other ThingType at "
             << ((unsigned long)&other)
             << " with value "
             << val << endl;
    }
    ThingType(int _val) : val(_val) {
        cout << "constructing new ThingType at "
             << ((unsigned long)this)
             << " with value "
             << val << endl;
    }
    ~ThingType(void) {
        cout << "destroying ThingType at "
             << ((unsigned long)this)
             << " with value "
             << val << endl;
    }
    void init(int _val) { 
        val = _val;
        cout << "calling init on ThingType at "
             << ((unsigned long)this)
             << " with value "
             << val << endl;
    }
    int val;
};

typedef MAP_WITH_ALLOCATOR(char,ThingType) M_type;

int
main()
{
    M_type m;

    m.insert(M_type::value_type('a',ThingType(1)));
    m.insert(make_pair('b',ThingType(2)));
    m['c'].init(3);

    cout << "objects are now present" << endl;

    cout << "found obj for 'a' : " << m['a'].val << endl;
    cout << "found obj for 'b' : " << m['b'].val << endl;
    cout << "found obj for 'c' : " << m['c'].val << endl;

    return 0;
}
