
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "condition.h"
#include "automake.h"

int
main(int argc, char ** argv )
{
    condition_init();
    condition_set("someflag");
    condition_set("PFK_AM_BUILD_doxygen");

    if (argc == 2)
    {
        if (getenv("DEBUG") == NULL)
        {
            // xxx parse conditions off of command line

            struct makefile * mf;
            mf = parse_makefile(argv[1]);
            if (mf)
            {
//            printmakefile(mf);
                automake(mf);
            }
        }
        else
            print_tokenized_file(argv[1]);
    }

    return 0;
}
