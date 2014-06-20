
#include "dll3.h"

using namespace DLL3;

const std::string ListError::errStrings[__NUMERRS] = {
    "item not valid",
    "item already on list",
    "item still on a list",
    "list not empty",
    "list not locked",
    "item not on this list"
};

const std::string
ListError::Format(void) const
{
    std::string ret = "LIST ERROR: ";
    ret += errStrings[err];
    ret += " at:\n";
    ret += BackTraceFormat();
    return ret;
}

