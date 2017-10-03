
#include <stdio.h>
#include "tokenizer.h"
#include "parser.h"
#include "condition.h"
#include "y.tab.h"

int
main(int argc, char ** argv )
{
    condition_init();
    condition_set("someflag");
    condition_set("PFK_AM_BUILD_doxygen");

    if (argc == 2)
    {
#if 1
        // xxx parse conditions off of command line

        struct makefile * mf;
        mf = parse_makefile(argv[1]);
        if (mf)
            printmakefile(mf);
#else
        print_tokenized_file(argv[1]);
#endif
    }

    return 0;
}
