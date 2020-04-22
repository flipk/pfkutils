/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __TEMPLATE_PATTERNS_H__
#define __TEMPLATE_PATTERNS_H__

#include <map>
#include <string>

// pattern : value
typedef std::map<std::string,std::string> pattern_value_map;

extern std::string find_pattern(const pattern_value_map &values,
                                const std::string &pattern,
                                const std::string &template_name);

#endif // __TEMPLATE_PATTERNS_H__
