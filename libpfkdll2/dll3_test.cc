#if 0
set -e -x
g++ -Wall -Werror -O6 -S newlist_test.cc
g++ -Wall -Werror -O6 newlist_test.cc -o nlt -lpthread
./nlt
c++filt < newlist_test.s > newlist_test.2.s
exit 0
#endif

#include "dll3.H"

#include <stdio.h>

class myItem;
typedef DLL3::List<myItem,1> myItemList1_t;
typedef DLL3::List<myItem,2> myItemList2_t;

class someBase {
public:
    int z;
};

class myItem : public someBase,
               public myItemList1_t::Links,
               public myItemList2_t::Links
{
public:
    myItem(int _a) : a(_a) { }
    int a;
};

int
main()
{
    myItemList1_t  lst1;
    myItemList2_t  lst2;
    myItem * i, * ni;

    {
        DLL3::Lock lck1(&lst1);
        DLL3::Lock lck2(&lst2);
        i = new myItem(4); lst1.add_tail(i); lst2.add_head(i);
        i = new myItem(5); lst1.add_tail(i); lst2.add_head(i);
        i = new myItem(6); lst1.add_tail(i); lst2.add_head(i);
    }

    {
        DLL3::Lock lck(&lst1);
        for (i = lst1.get_head(); i; i = ni)
        {
            ni = lst1.get_next(i);
            printf("lst1 item : %d\n", i->a);
            lst1.remove(i);
        }
    }

    {
        DLL3::Lock lck(&lst2);
        while ((i = lst2.dequeue_head()) != NULL)
        {
            printf("lst2 item : %d\n", i->a);
            delete i;
        }
    }

    return 0;
}
