
#include <stdio.h>
#include <stdarg.h>

// #include "y.tab.h"
#include "tuple.h"

char input_buf[] =
"(zero * (one:two three:(four:(five:six seven:eight)) nine:ten) eleven:twelve hex:#123456 string:\"string\")";

extern int yydebug;

int
main()
{
    struct tuple * parse_output;

    yydebug = 0;
    parse_output = tuple_parse(input_buf, sizeof(input_buf));
    print_tuples(parse_output);
    printf("\n");

    return 0;
}
