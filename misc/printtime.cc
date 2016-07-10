
#include <sys/time.h>
#include <iostream>
#include <stdlib.h>

using namespace std;

int
main(int argc, char ** argv)
{
    struct timeval tv;
    struct tm local_time;
    char * end;

    tv.tv_sec = strtoul(argv[1], &end, 0);
    tv.tv_usec = 0;

    cout << "tv_sec " << tv.tv_sec << " is:" << endl;

    localtime_r(&tv.tv_sec, &local_time);

    cout << "year: " << local_time.tm_year + 1900
         << " month: " << local_time.tm_mon + 1
         << " day: " << local_time.tm_mday
         << " dow: " << local_time.tm_wday
         << " hour: " << local_time.tm_hour
         << " min: " << local_time.tm_min
         << " sec: " << local_time.tm_sec
         << endl;

    return 0;
}
