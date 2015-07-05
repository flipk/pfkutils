
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "condition.h"

using namespace std;

extern int yylineno;

// match[1] is for the "!"
// match[2] is for the word
const char ConditionSet::condition_regex_expr[] =
    "if[ \t]+(\\!{0,1})[ \t]*([a-zA-Z0-9_-]+)[ \t]*";

ConditionSet conditions;

ConditionSet :: ConditionSet(void)
{
    int err;
    err = regcomp(&condition_regex, condition_regex_expr, REG_EXTENDED);
    if (err != 0)
    {
        cerr << "regcomp failed, returns " << err << endl;
        exit(1);
    }
    for (int ind = 0; ind < max_conditions; ind++)
        conditions[ind] = NULL;
    num_conditions = 0;
}

ConditionSet :: ~ConditionSet(void)
{
    for (int ind = 0; ind < max_conditions; ind++)
        if (conditions[ind])
            delete conditions[ind];
}

void
ConditionSet :: set(string text)
{
    if (num_conditions == max_conditions)
    {
        cerr << "ConditionSet is out of space!" << endl;
        exit(1);
    }
    conditions[num_conditions++] = new string(text);
}

bool
ConditionSet :: check(const char * text, int len)
{
    int negated = 0;
    int cc, ind, set = 0;
    regmatch_t matches[8];
    char str[256];
    const char * cond = "";
    if (len > 256)
        len = 256;
    memcpy(str, text, len);
    str[len-1] = 0; // strip newline
    cc = regexec(&condition_regex, str, 8, matches, 0);
    if (cc == REG_NOMATCH)
    {
        cerr << "syntax error on 'if' statement line " << (yylineno-1) << endl;
        exit(1);
    }
    else
    {
        if (matches[1].rm_so != matches[1].rm_eo)
        {
            negated = 1;
        }
        str[matches[2].rm_eo] = 0;
        cond = str + matches[2].rm_so;
    }

    for (ind = 0; ind < num_conditions; ind++)
        if (*conditions[ind] == cond)
            set = 1;

    return set ^ negated;
}
