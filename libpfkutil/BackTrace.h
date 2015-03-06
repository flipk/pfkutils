/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __BACKTRACE_H__
#define __BACKTRACE_H__

#include <string>
#include <execinfo.h>

namespace BackTraceUtil {

struct BackTrace {
    static const int MAX_ADDRESSES = 20;
    void * traceAddresses[MAX_ADDRESSES];
    size_t  numAddresses;
    BackTrace(void);
    const std::string Format(void) const;
};

inline BackTrace::BackTrace(void)
{
    numAddresses = backtrace(traceAddresses, MAX_ADDRESSES);
}

inline const std::string
BackTrace::Format(void) const
{
    std::string ret;
    char ** symbols = backtrace_symbols(traceAddresses,
                                        numAddresses);
    for (size_t ind = 0; ind < numAddresses; ind++)
    {
        ret += std::string(symbols[ind]);
        ret += "\n";
    }
    return ret;
}

}; // namespace HSM

#endif /* __BACKTRACE_H__ */
