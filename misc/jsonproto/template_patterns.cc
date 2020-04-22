
#include <stdio.h>
#include <stdlib.h>
#include "template_patterns.h"

std::string
find_pattern(const pattern_value_map &values,
             const std::string &pattern,
             const std::string &template_name)
{
    pattern_value_map::const_iterator  it;
    it = values.find(pattern);
    if (it == values.end())
    {
        fprintf(stderr, "processing template '%s': "
                "pattern '%s' is not defined!\n",
                template_name.c_str(), pattern.c_str());
        exit(1);
    }
    return it->second;
}
