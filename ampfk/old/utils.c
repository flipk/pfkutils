
#include <stdio.h>
#include <string.h>
#include "utils.h"

struct variable *
find_var(struct makefile *mf, const char *name)
{
    struct variable * v = mf->variables;
    while (v)
    {
        if (strcmp(v->variable, name) == 0)
            return v;
        v = v->next;
    }
    return NULL;
}
