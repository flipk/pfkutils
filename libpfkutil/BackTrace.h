/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __BACKTRACE_H__
#define __BACKTRACE_H__

#ifndef __CYGWIN__

// NOTE NOTE NOTE
//
// proper use of the backtrace functionality requires use
// of the "-rdynamic" flag to gcc.
//
// NOTE NOTE NOTE

#include <string>
#include <execinfo.h>

/** handy utilities for throwing exceptions and getting
 * function backtraces of them */
namespace BackTraceUtil {

/** base class for errors you can throw. constructor
 * takes a snapshot of the stack and the Format method
 * prints out the symbol names. derive all your throwable
 * errors from this base class. */
struct BackTrace {
    static const int MAX_ADDRESSES = 20;
    void * traceAddresses[MAX_ADDRESSES];
    size_t  numAddresses;
    /** constructor takes a stack snapshot */
    BackTrace(void);
    /** look up the symbols associated with the stack trace
     * and return a multi-line string listing them.
     * \return multi-line string containing the stack backtrace. */
    const std::string Format(void) const;
protected:
    virtual const std::string _Format(void) const = 0;
};

// use this if you dont want to derive another
// class from BackTrace.
struct BackTraceBasic : public BackTrace {
    /*virtual*/ const std::string _Format(void) const {
        return "";
    }
};

// inline impls below this line

inline BackTrace::BackTrace(void)
{
    numAddresses = backtrace(traceAddresses, MAX_ADDRESSES);
}

inline const std::string
BackTrace::Format(void) const
{
    std::string ret = _Format();
    char ** symbols = backtrace_symbols(traceAddresses, numAddresses);
    for (size_t ind = 0; ind < numAddresses; ind++)
    {
        ret += "[bt] ";
        ret += std::string(symbols[ind]);
        ret += "\n";
    }
    return ret;
}

}; // namespace BackTraceUtil

#else /* __CYGWIN__ */

#include <string>

namespace BackTraceUtil {

struct BackTrace {
    const std::string Format(void) const { return ""; }
};

}; // namespace BackTraceUtil

#endif /* __CYGWIN__ */

#endif /* __BACKTRACE_H__ */
