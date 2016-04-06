
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <iostream>

using namespace std;

// Wichmann-Hill  algorithm AS183
// or https://en.wikipedia.org/wiki/Wichmann-Hill

class wichmann_hill_as183
{
    int s1;
    int s2;
    int s3;
public:
    wichmann_hill_as183(void) {
        s1 = random() % 30000;
        s2 = random() % 30000;
        s3 = random() % 30000;
    }
    int operator() () {
        s1 = (s1 * 171) % 30269;
        s2 = (s2 * 172) % 30307;
        s3 = (s3 * 170) % 30323;
        return (s1 % 30269 + s2 % 30307 + s3 % 30323);
    }
};

int main()
{
    srandom( getpid() * time(NULL) );

    wichmann_hill_as183   r;

    for (int ind = 0; ind < 1000000; ind++)
        cout << r() << endl;

    return 0;
}
