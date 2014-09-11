/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __CIRCULAR_READER_H__
#define __CIRCULAR_READER_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <string>
#include <iostream>

namespace WebAppServer {

class CircularReaderError {
public:
    CircularReaderError(const char * desc);
};

class CircularReaderSubstr {
protected:
    uint8_t * buf;
    int bufsz;
    int startPos;
    int len;
    int realPos(int pos, bool validate=true) const;
    int circularDifference(int a, int b) const;
    int maxContigWritable(void) const;
public:
    CircularReaderSubstr(void);
    CircularReaderSubstr(const CircularReaderSubstr &other);
    CircularReaderSubstr(uint8_t * _buf, int _bufsz,
                         int _startPos = 0, int _len = 0);
    ~CircularReaderSubstr(void);
    static const int npos = 0xFFFFFFF;
    int size(void) const;
    int remaining(void) const;
    uint8_t &operator[] (int pos);
    const uint8_t& operator[] (int pos) const;
    CircularReaderSubstr substr(int _pos, int _len = npos) const;
    int find_first_of(char c, int pos=0) const;
    int find(const char *patt, int pos=0) const;
    int icaseFind(const char *patt, int pos=0) const;
    bool operator==(const char *patt) const;
    void operator=(const CircularReaderSubstr &other);
    bool operator<(const CircularReaderSubstr &other) const;
    bool compare(int comparePos, const char *patt) const;
    void erase(int erasePos,int eraseLen = npos);
    // npos is not an option for wantLen, because the user must
    // specify the max size of outBuffer. returns length copied.
    int copyOut(uint8_t * outBuffer, int wantPos, int wantLen) const;
    std::string toString(int wantPos = 0, int wantLen = npos) const;
};

class CircularReader : public CircularReaderSubstr
{
public:
    CircularReader(void);
    CircularReader(int _maxSize);
    CircularReader(const std::string &);
    ~CircularReader(void);
    void assign(const uint8_t *initBuf, int initLen);
    void operator=(const CircularReaderSubstr &other);
    void operator=(const std::string &);
    int readFd(int fd);
    void erase0(int eraseLen = npos);
};

std::ostream& operator<<(std::ostream &ostr,
                         const CircularReaderSubstr &str);


// below this line is all implementation of above classes.

inline int
CircularReaderSubstr :: realPos(int _pos, bool _validate /*=true*/) const
{
    if (_validate && _pos >= len) {
        throw CircularReaderError("invalid pos in realPos");
        return 0;
    }
    int ret = startPos + _pos;
    if (ret >= bufsz)
        ret -= bufsz;
    return ret;
}

inline int
CircularReaderSubstr :: circularDifference(int a, int b) const
{
    if (a >= b)
        return a-b;
    return (bufsz-b) + a;
}

inline int
CircularReaderSubstr :: maxContigWritable(void) const
{
    int endPos = realPos(len,false);
    if (endPos >= startPos)
        return bufsz - endPos;
    return startPos - endPos;
}

inline
CircularReaderSubstr :: CircularReaderSubstr(void)
    : buf(NULL), bufsz(0), startPos(0), len(0)
{
}

inline
CircularReaderSubstr :: CircularReaderSubstr(
    uint8_t * _buf, int _bufsz,
    int _startPos /*= 0*/, int _len /*= 0*/)
    : buf(_buf), bufsz(_bufsz), startPos(_startPos), len(_len)
{
}

inline
CircularReaderSubstr :: CircularReaderSubstr(const CircularReaderSubstr &other)
{
    operator=(other);
}

inline bool
CircularReaderSubstr :: operator<(const CircularReaderSubstr &other) const
{
    int otherSize = other.size();
    for (int pos = 0; pos < len; pos++)
    {
        if (pos >= otherSize)
            return true;
        uint8_t a = buf[realPos(pos,false)];
        uint8_t b = other[pos];
        if (a < b)
            return true;
        if (a > b)
            return false;
    }
    return false;
}

inline
CircularReaderSubstr :: ~CircularReaderSubstr(void)
{
}

inline int
CircularReaderSubstr :: size(void) const
{
    return len;
}

inline int
CircularReaderSubstr :: remaining(void) const
{
    return bufsz - len;
}

inline const uint8_t&
CircularReaderSubstr :: operator[] (int _pos) const
{
    return buf[realPos(_pos)];
}

inline uint8_t &
CircularReaderSubstr :: operator[] (int _pos)
{
    return buf[realPos(_pos)];
}

inline CircularReaderSubstr
CircularReaderSubstr :: substr(int _pos, int _len /*= npos*/) const
{
    if (_pos > len) {
        throw CircularReaderError("invalid pos in substr");
    }
    int newLen = len - _pos;
    if (_len > newLen)
        _len = newLen;
    return CircularReaderSubstr(buf, bufsz, realPos(_pos), _len);
}

inline int
CircularReaderSubstr :: find_first_of(char _c, int _pos/*=0*/) const
{
    for (;_pos < len; _pos++)
        if (buf[realPos(_pos,false)] == _c)
            return _pos;
    return npos;
}

inline int
CircularReaderSubstr :: find(const char *_patt, int _pos/*=0*/) const
{
    int pattLen = ::strlen(_patt);
    if (pattLen == 0)
        return _pos;
    for (;_pos <= (len-pattLen); _pos++) {
        int pattPos;
        for (pattPos = 0; pattPos < pattLen; pattPos++)
            if (_patt[pattPos] != buf[realPos(_pos+pattPos,false)])
                break;
        if (pattPos == pattLen)
            return _pos;
    }
    return npos;
}

inline int
CircularReaderSubstr :: icaseFind(const char *_patt, int _pos /*=0*/) const
{
    int pattLen = ::strlen(_patt);
    if (pattLen == 0)
        return _pos;
    for (;_pos <= (len-pattLen); _pos++) {
        int pattPos;
        for (pattPos = 0; pattPos < pattLen; pattPos++)
            if (_patt[pattPos] != tolower(buf[realPos(_pos+pattPos,false)]))
                break;
        if (pattPos == pattLen)
            return _pos;
    }
    return npos;
}

inline bool
CircularReaderSubstr :: operator==(const char *_patt) const
{
    int pattLen = ::strlen(_patt);
    if (pattLen != len)
        return false;
    for (int pos = 0; pos < len; pos++)
        if (_patt[pos] != buf[realPos(pos,false)])
            return false;
    return true;
}

inline void
CircularReaderSubstr :: operator=(const CircularReaderSubstr &other)
{
    buf = other.buf;
    bufsz = other.bufsz;
    startPos = other.startPos;
    len = other.len;
}

inline bool
CircularReaderSubstr :: compare(int _comparePos, const char *_patt) const
{
    int pattLen = ::strlen(_patt);
    if ((_comparePos + pattLen) > len)
        return false;
    for (int pos = 0; pos < pattLen; pos++)
        if (_patt[pos] != buf[realPos(_comparePos+pos,false)])
            return false;
    return true;
}

inline void
CircularReaderSubstr :: erase(int _erasePos, int _eraseLen /*= npos*/)
{
    if ((_erasePos + _eraseLen) >= len)
    {
        _eraseLen = len - _erasePos;
    }
    else
    {
        int copySource = _erasePos + _eraseLen;
        int copyDest = _erasePos;
        int copyLen = len - _erasePos - _eraseLen;
        while (copyLen-- > 0)
        {
            buf[realPos(copyDest,false)] = buf[realPos(copySource,false)];
            copyDest++; copySource++;
        }
    }
    len -= _eraseLen;
}


// npos is not an option for wantLen, because the user must
// specify the max size of outBuffer. return length copied.
inline int
CircularReaderSubstr :: copyOut(uint8_t * _outBuffer,
                                int _wantPos, int _wantLen) const
{
    if (_wantPos >= len || _wantLen == 0)
        return 0;
    if (_wantLen > (len - _wantPos))
        _wantLen =  len - _wantPos;
    int realStart = startPos + _wantPos;
    if (realStart >= bufsz)
        realStart -= bufsz;
    int realEnd = realStart + _wantLen;
    if (realEnd >= bufsz)
        realEnd -= bufsz;
    if (realEnd > realStart) {
        memcpy(_outBuffer, buf + realStart, _wantLen);
        return _wantLen;
    }
    //else
    memcpy(_outBuffer, buf + realStart, bufsz - realStart);
    memcpy(_outBuffer + bufsz - realStart, buf, realEnd);
    return _wantLen;
}

inline std::string
CircularReaderSubstr :: toString(int _wantPos /*= 0*/,
                                 int _wantLen /*= npos*/) const
{
    if (_wantPos >= len || _wantLen == 0)
        return std::string("");
    if (_wantLen > (len - _wantPos))
        _wantLen =  len - _wantPos;
    int realStart = startPos + _wantPos;
    if (realStart >= bufsz)
        realStart -= bufsz;
    int realEnd = realStart + _wantLen;
    if (realEnd >= bufsz)
        realEnd -= bufsz;
    if (realEnd > realStart)
        return std::string((char*)(buf + realStart), _wantLen);
    //else
    return std::string((char*)(buf + realStart),
                       bufsz - realStart).append((char*)buf, realEnd);
}

inline
CircularReader :: CircularReader(void)
{
    buf = NULL;
    startPos = bufsz = len = 0;
}

inline
CircularReader :: CircularReader(int _maxSize)
    : CircularReaderSubstr(new uint8_t[_maxSize], _maxSize)
{
}

inline
CircularReader :: CircularReader(const std::string &initStr)
{
    buf = NULL;
    operator=(initStr);
}

inline
CircularReader :: ~CircularReader(void)
{
    delete[] buf;
}

inline void
CircularReader :: assign(const uint8_t *initBuf, int initLen)
{
    if (buf != NULL)
        delete[] buf;
    startPos = 0;
    bufsz = len = initLen;
    buf = new uint8_t[len];
    memcpy(buf, initBuf, len);
}

inline void
CircularReader :: operator=(const CircularReaderSubstr &other)
{
    if (buf != NULL)
        delete[] buf;
    startPos = 0;
    bufsz = len = other.size();
    buf = new uint8_t[len];
    other.copyOut(buf,0,len);
}

inline void
CircularReader :: operator=(const std::string &initStr)
{
    if (buf != NULL)
        delete[] buf;
    startPos = 0;
    bufsz = len = initStr.size();
    buf = new uint8_t[len];
    memcpy(buf, initStr.c_str(), len);
}

inline int
CircularReader :: readFd(int _fd)
{
    int readMax = maxContigWritable();
    if (readMax == 0)
        throw CircularReaderError("read: full buffer");
    uint8_t * readPos = buf + realPos(len,false);
    int cc = ::read(_fd, readPos, readMax);
    if (cc > 0)
        len += cc;
    return cc;
}

inline void
CircularReader :: erase0(int _eraseLen /*= npos*/)
{
    if (_eraseLen > len)
        _eraseLen = len;
    startPos += _eraseLen;
    if (startPos > bufsz)
        startPos -= bufsz;
    len -= _eraseLen;
}

inline CircularReaderError :: CircularReaderError(const char * desc)
{
    std::cerr << desc << std::endl;
}

inline std::ostream& operator<<(std::ostream &ostr,
                                const CircularReaderSubstr &str)
{
    for (int pos = 0; pos < str.size(); pos++)
        ostr << str[pos];
    return ostr;
}

} // namespace WebAppServer

#endif /* __CIRCULAR_READER_H__ */
