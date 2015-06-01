/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

template <int maxsize>
class Bufprintf {
    char buf[maxsize];
    int len;
public:
    Bufprintf(void) {
        memset(buf,0,maxsize);
        len = 0;
    }
    ~Bufprintf(void) { }
    void print(const char *format...) {
        va_list ap;
        va_start(ap,format);
        int cc = vsnprintf(buf+len,maxsize-len-1,format,ap);
        len += cc;
        if (len > (maxsize-1))
            len = (maxsize-1);
        va_end(ap);
    }
    void write(int fd) { ::write(fd,buf,len); }
    char * getBuf(void) { return buf; }
    int getLen(void) { return len; }
};
