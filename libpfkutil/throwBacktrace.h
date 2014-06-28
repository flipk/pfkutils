/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __THROWBACKTRACE_H__
#define __THROWBACKTRACE_H__

#include <string>
#include <execinfo.h>

namespace ThrowUtil {

struct ThrowBackTrace {
    static const int MAX_ADDRESSES = 20;
    void * traceAddresses[MAX_ADDRESSES];
    size_t  numAddresses;
    ThrowBackTrace(void);
    const std::string BackTraceFormat(void) const;
};

inline ThrowBackTrace::ThrowBackTrace(void)
{
    numAddresses = backtrace(traceAddresses, MAX_ADDRESSES);
}

inline const std::string
ThrowBackTrace::BackTraceFormat(void) const
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

#endif /* __THROWBACKTRACE_H__ */
