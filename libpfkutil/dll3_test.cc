#if 0
set -e -x
g++ -Wall -Werror -g3 -c dll3.cc
g++ -Wall -Werror -g3 -c dll3_test.cc
g++ dll3_test.o dll3.o -o nlt -rdynamic -lpthread
./nlt
exit 0
#endif

#include "dll3.h"

#include <stdio.h>
#include <iostream>

class myItem;
class myItemHashCompa;
class myItemHashCompz;
typedef DLL3::List<myItem,1> myItemList1_t;
typedef DLL3::List<myItem,2> myItemList2_t;
typedef DLL3::Hash<myItem,int,myItemHashCompa,3> myItemHash3_t;
typedef DLL3::Hash<myItem,int,myItemHashCompz,4> myItemHash4_t;

class someBase {
public:
    int z;
    someBase(int _z) : z(_z) { }
};
std::ostream& operator<< (std::ostream &ostr, const someBase *it)
{
    ostr << "z=" << it->z;
    return ostr;
}

class myItem : public someBase,
               public myItemList1_t::Links,
               public myItemList2_t::Links,
               public myItemHash3_t::Links,
               public myItemHash4_t::Links
{
public:
    myItem(int _a, int _z) : someBase(_z), a(_a) { }
    int a;
};

class myItemHashCompa {
public:
    static uint32_t obj2hash(const myItem &item)
    { return (uint32_t) item.a; }
    static uint32_t key2hash(const int &key)
    { return (uint32_t) key; }
    static bool hashMatch(const myItem &item, const int &key)
    { return (item.a == key); }
};
class myItemHashCompz {
public:
    static uint32_t obj2hash(const myItem &item)
    { return (uint32_t) item.z; }
    static uint32_t key2hash(const int &key)
    { return (uint32_t) key; }
    static bool hashMatch(const myItem &item, const int &key)
    { return (item.z == key); }
};

std::ostream& operator<< (std::ostream &ostr, const myItem *it)
{
    ostr << "a=" << it->a << ", " << (const someBase *)it;
    return ostr;
}

#define DO_TRY 0

int
main()
{
#if DO_TRY
    try
#endif
    {
        myItemList1_t  lst1;
        myItemList2_t  lst2;
        myItemHash3_t  hash3;
        myItemHash4_t  hash4;
        myItem * i, * ni;

        {
            WaitUtil::Lock lck1(&lst1);
            WaitUtil::Lock lck2(&lst2);
            WaitUtil::Lock lck3(&hash3);
            WaitUtil::Lock lck4(&hash4);
            i = new myItem(4,1);
            lst1.add_tail(i);
            lst2.add_head(i);
            hash3.add(i);
            hash4.add(i);

            i = new myItem(5,2);
            lst1.add_tail(i);
            lst2.add_head(i);
            hash3.add(i);
            hash4.add(i);

            i = new myItem(6,3);
            lst1.add_tail(i);
            lst2.add_head(i);
            hash3.add(i);
            hash4.add(i);
        }

        {
            WaitUtil::Lock lck3(&hash3);
            i = hash3.find(6);
            if (i)
                std::cout << "search for a=6: " << i << std::endl;

            i = hash3.find(4);
            if (i)
                std::cout << "search for a=4: " << i << std::endl;
        }

        {
            WaitUtil::Lock lck4(&hash4);
            i = hash4.find(2);
            if (i)
                std::cout << "search for z=2: " << i << std::endl;

            i = hash4.find(3);
            if (i)
                std::cout << "search for z=3: " << i << std::endl;
        }

        {
            WaitUtil::Lock lck1(&lst1);
            for (i = lst1.get_head(); i; i = ni)
            {
                ni = lst1.get_next(i);
                std::cout << "lst1 item : " << i << std::endl;
                lst1.remove(i);
                WaitUtil::Lock lck3(&hash3);
                hash3.remove(i);
            }
        }

        {
            WaitUtil::Lock lck2(&lst2);
            while ((i = lst2.dequeue_head()) != NULL)
            {
                std::cout << "lst2 item : " << i << std::endl;
                WaitUtil::Lock lck4(&hash4);
                hash4.remove(i);
                delete i;
            }
        }
    }
#if DO_TRY
    catch (DLL3::ListError le)
    {
        std::cout << "got ListError:\n"
                  << le.Format();
    }
#endif

    return 0;
}
