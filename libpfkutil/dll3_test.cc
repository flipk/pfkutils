#if 0
set -e -x
g++ -Wall -Werror -g3 dll3_test.cc -o nlt -rdynamic -lpthread
./nlt
exit 0
#endif

#include "dll3.H"

#include <stdio.h>
#include <iostream>

class myItem;
typedef DLL3::List<myItem,1> myItemList1_t;
typedef DLL3::List<myItem,2> myItemList2_t;

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
               public myItemList2_t::Links
{
public:
    myItem(int _a, int _z) : someBase(_z), a(_a) { }
    int a;
};
std::ostream& operator<< (std::ostream &ostr, const myItem *it)
{
    ostr << "a=" << it->a << ", " << (const someBase *)it;
    return ostr;
}

int
main()
{
    myItemList1_t  lst1;
    myItemList2_t  lst2;
    myItem * i, * ni;

    try {
    {
        PFK::Lock lck1(&lst1);
        PFK::Lock lck2(&lst2);
        i = new myItem(4,1); lst1.add_tail(i); lst2.add_head(i);
        i = new myItem(5,2); lst1.add_tail(i); lst2.add_head(i);
        i = new myItem(6,3); lst1.add_tail(i); lst2.add_head(i);
    }

    {
        PFK::Lock lck1(&lst1);
        for (i = lst1.get_head(); i; i = ni)
        {
            ni = lst1.get_next(i);
            std::cout << "lst1 item : " << i << std::endl;
            lst1.remove(i);
        }
    }

    {
        PFK::Lock lck1(&lst2);
        while ((i = lst2.dequeue_head()) != NULL)
        {
            std::cout << "lst2 item : " << i << std::endl;
            delete i;
        }
    }
    }
    catch (DLL3::ListError le)
    {
        std::cout << "got ListError:\n"
                  << le.Format();
    }

    return 0;
}
