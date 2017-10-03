
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "condition.H"
#include "automake_parser.H"

using namespace std;

extern "C" int ampfk_main(int argc, char ** argv);

int
ampfk_main(int argc, char ** argv)
{
    automake_file   amf;

    if (argc != 2)
    {
        cerr << "usage: ampfk <full path to Makefile.am>" << endl;
        return 1;
    }

    if (getenv("DEBUG") != NULL)
        amf.tokenize(argv[1]);
    else
    {
        if (amf.parse(argv[1]) == false)
        {
            cerr << "error: unable to parse " << argv[1] << endl;
            return 1;
        }

        amf.output_makefile(argv[1], "Makefile");
    }

    return 0;
}
