
#include <stdio.h>
#include <string.h>
#include <map>

struct mykey
{
    unsigned char buf[2];
    bool operator<(const mykey &other) const {
        if (memcmp(buf, other.buf, 2) < 0)
            return true;
        return false;
    }
};

struct mything
{
    int a;
};

typedef std::map <mykey,mything*> mythingMap;
typedef std::pair<mykey,mything*> mythingMapPair;
typedef std::map <mykey,mything*>::iterator mythingMapIter;

int
main()
{
    mythingMap m;

    mykey one, two, three;
    one  .buf[0] = 1; one  .buf[1] = 2;
    two  .buf[0] = 2; two  .buf[1] = 3;
    three.buf[0] = 3; three.buf[1] = 4;

    mything x, y, z;
    x.a = 4;
    y.a = 5;
    z.a = 6;

    m.insert(mythingMapPair(one,   &x));
    m.insert(mythingMapPair(two,   &y));
    m.insert(mythingMapPair(three, &z));

    printf("map has %d items\n", m.size());

    mythingMapIter  it;

    for (it = m.begin();
         it != m.end();
         it++)
    {
        printf(" item : %d,%d -> %d\n", 
               it->first.buf[0],
               it->first.buf[1],
               it->second->a);
    }
    printf("\n");

    three.buf[0] = 1; three.buf[1] = 4;
    it = m.find(three);
    if (it == m.end())
    {
        printf("not found\n");
    }
    else
    {
        printf("found : %d\n", it->second->a);
    }

    two.buf[0] = 3; two.buf[1] = 4;
    it = m.find(two);
    if (it == m.end())
    {
        printf("not found\n");
    }
    else
    {
        printf("found : %d\n", it->second->a);
    }

    // equivalent
    // m.erase(it);
    m.erase(two);

    for (it = m.begin();
         it != m.end();
         it++)
    {
        printf(" item : %d,%d -> %d\n", 
               it->first.buf[0],
               it->first.buf[1],
               it->second->a);
    }
    printf("\n");
   

}
