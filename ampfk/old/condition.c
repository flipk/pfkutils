
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "regex.h"

extern int yylineno;

// match[1] is for the "!"
// match[2] is for the word
static const char condition_regex_expr[] =
    "if[ \t]+(\\!{0,1})[ \t]*([a-zA-Z0-9_-]+)[ \t]*";
static regex_t condition_regex;

static int num_conditions = 0;
static char * conditions[100];

void
condition_init(void)
{
    int err;
    err = regcomp(&condition_regex, condition_regex_expr, REG_EXTENDED);
    if (err != 0)
        printf("regcomp failed, returns %d\n", err);
    num_conditions = 0;
}

void
condition_set(const char *text)
{
    if (num_conditions == 100)
    {
        printf("can only handle 100 conditions\n");
        exit(1);
    }
    conditions[num_conditions++] = strdup(text);
}

int
check_condition_name(const char *text, int len)
{
    int negated = 0;
    int cc, ind, set = 0;
    regmatch_t matches[8];
    char str[256];
    char * cond = "";
    if (len > 256)
        len = 256;
    memcpy(str, text, len);
    str[len-1] = 0; // strip newline
    cc = regexec(&condition_regex, str, 8, matches, 0);
    if (cc == REG_NOMATCH)
    {
        printf("syntax error on 'if' statement line %d\n", yylineno-1);
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
        if (strcmp(cond, conditions[ind]) == 0)
            set = 1;

    return set ^ negated;
}
