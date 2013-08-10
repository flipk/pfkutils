
#include <stdio.h>

#include <list>

struct mything
{
    int a;
};

typedef std::list<mything*>  mythingList;
typedef std::list<mything*>::iterator  mythingListIter;

int
main()
{
    mythingList l;


    mything x, y;
    x.a = 4;
    y.a = 5;

    l.push_back(&x);
    l.push_back(&y);


    printf("list has %d items\n", l.size());

    mythingListIter  it;

    for (it = l.begin();
         it != l.end();
         it++)
    {
        printf(" item : %d\n", (*it)->a);
    }
    printf("\n");

    mything z;
    z.a = 6;

    it = l.begin();
    it++;

    l.insert(it, &z);

    for (it = l.begin();
         it != l.end();
         it++)
    {
        printf(" item : %d\n", (*it)->a);
    }
    printf("\n");

    l.remove(&x);

    for (it = l.begin();
         it != l.end();
         it++)
    {
        printf(" item : %d\n", (*it)->a);
    }
    printf("\n");

}
