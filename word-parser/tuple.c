
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
#if 0
    dump_tokens();
#else
    if (yyparse() == 0)
        return parser_output;
    else
#endif
        return NULL;
}

void
print_tuples(struct tuple * t)
{
    printf("{");
    while (t)
    {
        printf("%s", t->word);
        switch (t->type)
        {
        case TUPLE_TYPE_NONE:
            break;
        case TUPLE_TYPE_WORD:
            printf("=%s", t->u.word);
            break;
        case TUPLE_TYPE_HEX:
            printf("=0x%s", t->u.word);
            break;
        case TUPLE_TYPE_STRING:
            printf("=\"%s\"", t->u.word);
            break;
        case TUPLE_TYPE_TUPLE:
            printf("=");
            print_tuples(t->u.tuples);
            break;
        }
        t = t->next;
        if (t)
            printf(" ");
    }
    printf("}");
}
