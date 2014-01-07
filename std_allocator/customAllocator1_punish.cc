#if 0
set -e -x
opts="-O3"
g++ $opts -c customAllocator1_punish.cc
g++ $opts customAllocator1_punish.o -lpthread -o ca11
./ca11
exit 0
    ;
#endif

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/time.h>
#include <map>
//#include <unordered_map>   // requires -std=c++0x or -std=c++11
#include "customAllocator1.h"

using namespace std;

#define ITEMS 1000000
#define ITERS 10000000

char present[ITEMS];
int count;
int creates;
int erases;
int finds;

struct myTimeval : public timeval {
    void operator-=(const myTimeval &other) {
        if (tv_usec < other.tv_usec)
        {
            tv_usec += 1000000;
            tv_sec -= 1;
        }
        tv_usec -= other.tv_usec;
        tv_sec -= other.tv_sec;
    }
};

ostream& operator<<(ostream& ostr, const myTimeval &tv) {
    ostr << tv.tv_sec;
    ostr << ".";
    ostr.width(6);
    ostr.fill('0');
    ostr << tv.tv_usec;
    return ostr;
}

int
main()
{
//    map<uint32_t,uint32_t>  m;
    MAP_WITH_ALLOCATOR(uint32_t,uint32_t) m;

//    unordered_map<uint32_t,uint32_t>  m;
//    m.reserve(ITEMS);

//    UMAP_WITH_ALLOCATOR(uint32_t,uint32_t) m;
//    m.reserve(ITEMS);

    myTimeval startTime, endTime;

    srandom(0xb12d5ca4);
    memset(present,0,sizeof(present));
    count = 0;

    gettimeofday(&startTime,NULL);
    for (int iter = 0; iter < ITERS; iter++)
    {
        uint32_t r = random();
        uint32_t item = r % ITEMS;

        if (present[item] == 0)
        {
            count ++;
            creates++;
            present[item] = 1;
            m[item] = item;
        }
        else
        {
            if ((r & 0x40000000) != 0)
            {
                finds++;
                if (m[item] != item)
                    cout << "ERROR" << endl;
            }
            else
            {
                count --;
                erases++;
                present[item] = 0;
                m.erase(item);
            }
        }
    }
    gettimeofday(&endTime,NULL);

    endTime -= startTime;

    cout << "ended up with " << count << " items("
         << m.size() << "), "
         << creates << " creates, "
         << erases << " erases, "
         << finds << " finds in " << endTime << endl;

    return 0;
}
