/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __MYTIMEVAL_H__
#define __MYTIMEVAL_H__

#include <sys/time.h>
#include <stdio.h>
#include <iostream>
#include <string>

struct myTimeval : public timeval
{
    myTimeval(void) { tv_sec = 0; tv_usec = 0; }
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
    const myTimeval& operator+=(const myTimeval &rhs) {
        tv_sec += rhs.tv_sec;
        tv_usec += rhs.tv_usec;
        if (tv_usec > 1000000)
        {
            tv_usec -= 1000000;
            tv_sec += 1;
        }
        return *this;
    }
    const bool operator==(const myTimeval &other) {
        if (tv_sec != other.tv_sec)
            return false;
        if (tv_usec != other.tv_usec)
            return false;
        return true;
    }
    const bool operator!=(const myTimeval &other) {
        return !operator==(other);
    }
    void getNow(void) {
        gettimeofday(this, NULL);
    }
    const std::string Format(void) {
        time_t seconds = tv_sec;
        struct tm t;
        localtime_r(&seconds, &t);
        char ymdhms[128], ms[12];
        strftime(ymdhms,sizeof(ymdhms),"%Y-%m-%d %H:%M:%S",&t);
        ymdhms[sizeof(ymdhms)-1] = 0;
        snprintf(ms,sizeof(ms),"%06d", tv_usec);
        ms[sizeof(ms)-1] = 0;
        return std::string(ymdhms) + "." + ms;
    }
};
static inline std::ostream& operator<<(std::ostream& ostr,
                                       const myTimeval &rhs)
{
    ostr << "myTimeval(" << rhs.tv_sec << "," << rhs.tv_usec << ")";
    return ostr;
}
static inline myTimeval operator-(const myTimeval &lhs, const myTimeval &rhs) {
   bool borrow = false;
   myTimeval tmp;
   tmp.tv_sec = lhs.tv_sec;
   tmp.tv_usec = lhs.tv_usec;
   if (rhs.tv_usec > lhs.tv_usec)
      borrow = true;
   tmp.tv_sec -= rhs.tv_sec;
   tmp.tv_usec -= rhs.tv_usec;
   if (borrow)
   {
      tmp.tv_sec -= 1;
      tmp.tv_usec += 1000000;
   }
   return tmp;
}
static inline myTimeval operator+(const myTimeval &lhs, const myTimeval &rhs) {
   myTimeval tmp;
   tmp.tv_sec = lhs.tv_sec + rhs.tv_sec;
   tmp.tv_usec = lhs.tv_usec + rhs.tv_usec;
   if (tmp.tv_usec > 1000000)
   {
      tmp.tv_usec -= 1000000;
      tmp.tv_sec += 1;
   }
   return tmp;
}
static inline bool operator>(const myTimeval &lhs, const myTimeval &other) {
   if (lhs.tv_sec > other.tv_sec) 
      return true;
   if (lhs.tv_sec < other.tv_sec) 
      return false;
   return lhs.tv_usec > other.tv_usec;
}

#endif /* __MYTIMEVAL_H__ */
