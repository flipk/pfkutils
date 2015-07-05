/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */


#ifndef __CONDITION_H__
#define __CONDITION_H__

#include <regex.h>
#include <string>

class ConditionSet {
    static const char condition_regex_expr[];
    static const int max_conditions = 100;
    regex_t condition_regex;
    int num_conditions;
    std::string * conditions[max_conditions];
public:
    ConditionSet(void);
    ~ConditionSet(void);
    void set(std::string text);
    bool check(const char *text, int len);
};

extern ConditionSet conditions;

#endif /* __CONDITION_H__ */
