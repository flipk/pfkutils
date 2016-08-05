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
    myTimeval(time_t s, long u) { set(s,u); }
    void set(time_t s, long u) { tv_sec = s; tv_usec = u; }
    const myTimeval& operator=(const timeval &rhs) {
        tv_sec = rhs.tv_sec;
        tv_usec = rhs.tv_usec;
        return *this;
    }
    const myTimeval& operator=(const timespec &rhs) {
        tv_sec = rhs.tv_sec;
        tv_usec = rhs.tv_nsec / 1000;
        return *this;
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
    const myTimeval &getNow(void) {
        gettimeofday(this, NULL);
        return *this;
    }
    const std::string Format(const char *format = NULL) {
        if (format == NULL)
            format = "%Y-%m-%d %H:%M:%S";
        time_t seconds = tv_sec;
        struct tm t;
        localtime_r(&seconds, &t);
        char ymdhms[128], ms[12];
        strftime(ymdhms,sizeof(ymdhms),format,&t);
        ymdhms[sizeof(ymdhms)-1] = 0;
        snprintf(ms,sizeof(ms),"%06ld", tv_usec);
        ms[sizeof(ms)-1] = 0;
        return std::string(ymdhms) + "." + ms;
    }
    uint32_t msecs(void) {
        return (tv_sec * 1000) + (tv_usec / 1000);
    }
    uint64_t usecs(void) {
        return ((uint64_t)tv_sec * 1000000) + tv_usec;
    }
    uint64_t nsecs(void) {
        return ((uint64_t)tv_sec * 1000000000) + (tv_usec * 1000);
    }
    timeval *operator()(void) { return this; }
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
static inline bool operator<(const myTimeval &lhs, const myTimeval &other) {
   if (lhs.tv_sec < other.tv_sec) 
      return true;
   if (lhs.tv_sec > other.tv_sec) 
      return false;
   return lhs.tv_usec < other.tv_usec;
}

struct myTimespec : public timespec
{
    myTimespec(void) { tv_sec = 0; tv_nsec = 0; }
    myTimespec(time_t s, long n) { set(s,n); }
    void set(time_t s, long n) { tv_sec = s; tv_nsec = n; }
    const myTimespec &operator=(const myTimespec &rhs) {
        tv_sec = rhs.tv_sec;
        tv_nsec = rhs.tv_nsec;
        return *this;
    }
    const myTimespec &operator=(const timeval &rhs) {
        tv_sec = rhs.tv_sec;
        tv_nsec = rhs.tv_usec * 1000;
        return *this;
    }
    const myTimespec &operator-=(const myTimespec &rhs) {
        bool borrow = false;
        if (rhs.tv_nsec > tv_nsec)
            borrow = true;
        tv_sec -= rhs.tv_sec;
        tv_nsec -= rhs.tv_nsec;
        if (borrow)
        {
            tv_sec -= 1;
            tv_nsec += 1000000000;
        }
        return *this;
    }
    const myTimespec &operator+=(const myTimespec &rhs) {
        tv_sec += rhs.tv_sec;
        tv_nsec += rhs.tv_nsec;
        if (tv_nsec > 1000000000)
        {
            tv_nsec -= 1000000000;
            tv_sec += 1;
        }
        return *this;
    }
    const bool operator==(const myTimespec &rhs) {
        return (tv_sec == rhs.tv_sec) && (tv_nsec == rhs.tv_nsec);
    }
    const bool operator!=(const myTimespec &rhs) {
        return !operator==(rhs);
    }
    const myTimespec &getNow(clockid_t clk_id = CLOCK_REALTIME) {
        clock_gettime(clk_id, this);
        return *this;
    }
    const myTimespec &getMonotonic(void) {
        return getNow(CLOCK_MONOTONIC);
    }
    const std::string Format(const char *format = NULL) {
        if (format == NULL)
            format = "%Y-%m-%d %H:%M:%S";
        time_t seconds = tv_sec;
        struct tm t;
        localtime_r(&seconds, &t);
        char ymdhms[128], ns[12];
        strftime(ymdhms,sizeof(ymdhms),format,&t);
        ymdhms[sizeof(ymdhms)-1] = 0;
        snprintf(ns,sizeof(ns),"%09ld", tv_nsec);
        ns[sizeof(ns)-1] = 0;
        return std::string(ymdhms) + "." + ns;
    }
    uint32_t msecs(void) {
        return (tv_sec * 1000) + (tv_nsec / 1000000);
    }
    uint64_t usecs(void) {
        return ((uint64_t)tv_sec * 1000000) + (tv_nsec / 1000);
    }
    uint64_t nsecs(void) {
        return ((uint64_t)tv_sec * 1000000000) + tv_nsec;
    }
};
static inline std::ostream& operator<<(std::ostream& ostr,
                                       const myTimespec &rhs)
{
    ostr << "myTimespec(" << rhs.tv_sec << "," << rhs.tv_nsec << ")";
    return ostr;
}
static inline myTimespec operator-(const myTimespec &lhs,
                                   const myTimespec &rhs) {
   bool borrow = false;
   myTimespec tmp;
   tmp.tv_sec = lhs.tv_sec;
   tmp.tv_nsec = lhs.tv_nsec;
   if (rhs.tv_nsec > lhs.tv_nsec)
      borrow = true;
   tmp.tv_sec -= rhs.tv_sec;
   tmp.tv_nsec -= rhs.tv_nsec;
   if (borrow)
   {
      tmp.tv_sec -= 1;
      tmp.tv_nsec += 1000000000;
   }
   return tmp;
}
static inline myTimespec operator+(const myTimespec &lhs,
                                   const myTimespec &rhs) {
   myTimespec tmp;
   tmp.tv_sec = lhs.tv_sec + rhs.tv_sec;
   tmp.tv_nsec = lhs.tv_nsec + rhs.tv_nsec;
   if (tmp.tv_nsec > 1000000000)
   {
      tmp.tv_nsec -= 1000000000;
      tmp.tv_sec += 1;
   }
   return tmp;
}
static inline bool operator>(const myTimespec &lhs,
                             const myTimespec &other) {
   if (lhs.tv_sec > other.tv_sec) 
      return true;
   if (lhs.tv_sec < other.tv_sec) 
      return false;
   return lhs.tv_nsec > other.tv_nsec;
}
static inline bool operator<(const myTimespec &lhs,
                             const myTimespec &other) {
   if (lhs.tv_sec < other.tv_sec) 
      return true;
   if (lhs.tv_sec > other.tv_sec) 
      return false;
   return lhs.tv_nsec < other.tv_nsec;
}


#endif /* __MYTIMEVAL_H__ */
