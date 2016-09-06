
#include "LockWait.h"

using namespace WaitUtil;

const std::string
LockableError::errStrings[__NUMERRS] = {
    "mutex locked in destructor",
    "lock recursion error"
};

//virtual
const std::string
LockableError::_Format(void) const
{
    std::string ret = "LOCKABLE ERROR: ";
    ret += errStrings[err];
    ret += " at:\n";
    return ret;
}
