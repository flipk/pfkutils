
#include "LockWait.h"

using namespace WaitUtil;

const std::string
LockableError::errStrings[__NUMERRS] = {
    "mutex locked in destructor",
    "lock recursion error"
};

const std::string
LockableError::Format(void) const
{
    std::string ret = "LOCKABLE ERROR: ";
    ret += errStrings[err];
    ret += " at:\n";
    ret += BackTraceFormat();
    return ret;
}
