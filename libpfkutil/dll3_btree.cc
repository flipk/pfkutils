
#include "dll3_btree.h"

using namespace DLL3;

const std::string BtreeError::errStrings[__NUMERRS] = {
    "item already on another list",
    "item is duplicate",
    "item not found, btree is empty",
    "item not found",
    "didn't find the same item",
    "screwed up root indicator",
    "screwed up left sib pointer",
    "screwed up right sib pointer",
    "can't steal or coalesce! debug me",
    "nonroot node shrunk! debug me"
};

//virtual
const std::string
BtreeError::_Format(void) const
{
    std::string ret = "BTREE ERROR: ";
    ret += errStrings[err];
    ret += " at:\n";
    return ret;
}
