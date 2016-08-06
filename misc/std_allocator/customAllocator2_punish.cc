#if 0
set -e -x
opts="-O6 -std=c++11"
g++ -Wall -Werror $opts -c customAllocator2.cc
g++ -Wall -Werror $opts -c customAllocator2_punish.cc
g++ $opts customAllocator2.o customAllocator2_punish.o -lpthread -o ca21
exit 0
    ;
#endif

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/time.h>
#include <list>
#include <map>
#include <unordered_map>
#include <iostream>
#include "customAllocator2.h"
#include "pfkposix.h"

using namespace std;

#define ITEMS 1000000
#define ITERS 10000000

char present[ITEMS];
int count;
int creates;
int erases;
int finds;

ostream& operator<<(ostream& ostr, const pfk_timeval &tv) {
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
//    map<uint32_t,uint32_t,std::less<uint32_t>,
//        CustomAllocator<pair<uint32_t,uint32_t> > >  m;
#if 0
    unordered_map<uint32_t,uint32_t>  m;
    m.reserve(ITEMS);
#else
    unordered_map<uint32_t, // key type
                  uint32_t, // T
                  std::hash<uint32_t>, // hash
                  std::equal_to<uint32_t>, // predicate
                  CustomAllocator<std::pair<uint32_t,uint32_t>> // allocator
        > m;
    m.reserve(ITEMS);
#endif
    pfk_timeval startTime, endTime;

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

    __customAllocatorBackend.dumpStats();

    return 0;
}
