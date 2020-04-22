
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "protobuf_tokenize_and_parse.h"

int
main(int argc, char ** argv)
{
    if (argc != 2)
    {
        printf("usage: test <file.proto>\n");
        printf(" DBG=1 test <file.proto>\n");
        return 1;
    }
    char * dbg = getenv("DBG");
    if (dbg != NULL)
    {
        protobuf_parser_debug_tokenize(argv[1]);
    }
    else
    {
        ProtoFile * pf = protobuf_parser(argv[1], NULL);
        if (pf)
        {
            std::cout << pf;
            delete pf;
        }
    }

    return 0;
}
