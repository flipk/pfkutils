
#include <iostream>
#include <list>
#include <map>

#include <sys/time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

struct myTimeval : public timeval
{
    myTimeval(void) {
        tv_sec = 0;
        tv_usec = 0;
    }
    myTimeval(time_t s, suseconds_t us) {
        tv_sec = s;
        tv_usec = us;
    }
    const myTimeval& operator-=(const myTimeval &rhs) {
        bool borrow = false;
        if (rhs.tv_usec > tv_usec)
            borrow = true;
        tv_sec -= rhs.tv_sec;
        tv_usec -= rhs.tv_usec;
        if (borrow)
        {
            tv_sec -= 1;
            tv_usec += 1000000;
        }
        return *this;
    }
    const myTimeval& operator-(const myTimeval &rhs) {
        return myTimeval(*this) -= rhs;
    }
    bool operator>(const myTimeval &other) {
        if (tv_sec > other.tv_sec) 
            return true;
        if (tv_sec < other.tv_sec) 
            return false;
        return tv_usec > other.tv_usec;
    }
    bool operator<(const myTimeval &other) {
        if (tv_sec < other.tv_sec) 
            return true;
        if (tv_sec > other.tv_sec) 
            return false;
        return tv_usec < other.tv_usec;
    }
};
ostream& operator<<(ostream& ostr, const myTimeval &rhs) {
    ostr << "myTimeval(" << rhs.tv_sec << "," << rhs.tv_usec << ")";
    return ostr;
}

int
main()
{
    myTimeval a;
    myTimeval b(0,900000);
    myTimeval tv(1,0);

    a.tv_sec = 1;
    a.tv_usec = 0;

    cout << "before:"
         << " a:" << a
         << " b:" << b
         << " tv:" << tv << endl;
    tv = a;
    tv -= b;
    cout << " after:"
         << " a:" << a
         << " b:" << b
         << " tv:" << tv << endl;
    tv = a - b;
    cout << " after:"
         << " a:" << a
         << " b:" << b
         << " tv:" << tv << endl;

    if (a > b)
        cout << "greater" << endl;
    if (a < b)
        cout << "less" << endl;

    select(1,NULL,NULL,NULL,&tv);

    cout << " after:"
         << " a:" << a
         << " b:" << b
         << " tv:" << tv << endl;

    return 0;
}
