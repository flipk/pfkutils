
#include <stdio.h>

#include "tuple.h"
#include "y.tab.h"
#include "tokenizer.h"

extern int yyparse(void);
extern struct tuple * parser_output;

struct tuple *
tuple_parse(char * input_buf, int len)
{
    parser_line_number = 1;
    tokenizer_set_input_buf(input_buf, len);
    if (yyparse() == 0)
        return parser_output;
    else
        return NULL;
}

void
print_tuples(struct tuple * t)
{
    while (t)
    {
        printf("%s,", t->word);
        if (t->type == TUPLE_TYPE_WORD)
            printf("%s", t->u.word);
        else
        {
            printf("{");
            print_tuples(t->u.tuples);
            printf("}");
        }
        t = t->next;
        if (t)
            printf(" ");
    }
}
