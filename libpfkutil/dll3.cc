
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

const int DLL3::dll3_hash_primes[dll3_num_hash_primes] = {
    97,     251,     499,
    997,    1999,    4999,
    9973,   24989,   49999,
    99991,  249989,  499979,
    999983, 2499997, 4999999,
    9999991
};
