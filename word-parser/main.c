
#include <stdio.h>
#include <stdarg.h>

// #include "y.tab.h"
#include "tuple.h"

char input_buf[] =
"(zero * one:two three:(four:(five:six seven:eight) nine:ten) eleven:twelve)";

int
main()
{
    struct tuple * parse_output;

    parse_output = tuple_parse(input_buf, sizeof(input_buf));
    print_tuples(parse_output);
    printf("\n");

    return 0;
}
