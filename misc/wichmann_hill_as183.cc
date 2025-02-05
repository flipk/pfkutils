
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

using namespace std;

// Wichmann-Hill  algorithm AS183
// or https://en.wikipedia.org/wiki/Wichmann-Hill

// produces uniformly distributed values between 0.0 and 1.0.
// The overall period is the least common multiple of these:
//   30268 × 30306 × 30322 / 4 = 6,953,607,871,644

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
    float operator() () {
        s1 = (s1 * 171) % 30269;
        s2 = (s2 * 172) % 30307;
        s3 = (s3 * 170) % 30323;
        return fmod(((float)s1 / 30269.0 +
                     (float)s2 / 30307.0 +
                     (float)s3 / 30323.0), 1.0);
    }
};

int main()
{
    srandom( getpid() * time(NULL) );

    wichmann_hill_as183   r;

    for (int ind = 0; ind < 10000; ind++)
        printf("%f\n", r());

    return 0;
}
