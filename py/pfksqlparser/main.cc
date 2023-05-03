
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "pfksql_tokenize_and_parse.h"

const char * cmd =
"  from SOME_TABLE \n"
"  select field1,field2,field3 \n"
"  where ((field4 ~ \"^four.*five\") or (4g = false)) \n"
"    and ((field3 > 4) or (thing like \"26*\"))\n"
"    and (field6 exists) and (field7 !exists)"
    ;

void usage(void)
{
    printf("usage : \n"
           "   test t   : show tokenized string\n"
           "   test p   : show parse tree\n");
}

int main(int argc, char ** argv)
{
    if (argc != 2)
    {
        usage();
        return 1;
    }
    printf("query:\n%s\n", cmd);
    if (argv[1][0] == 't')
    {
        pfksql_parser_debug_tokenize(cmd);
    }
    else if (argv[1][0] == 'p')
    {
        SelectCommand * sc = pfksql_parser(cmd);
        if (sc)
        {
            sc->print();
            delete sc;
        }
        else
            printf("pfksql_parser returned NULL!\n");
    }
    else
    {
        usage();
        return 1;
    }

    return 0;
}
