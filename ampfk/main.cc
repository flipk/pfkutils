
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "condition.H"
#include "automake_parser.H"

using namespace std;

int
main(int argc, char ** argv)
{
    automake_file   amf;

    if (getenv("DEBUG") != NULL)
        amf.tokenize(argv[1]);
    else
    {
        if (amf.parse(argv[1]) == false)
        {
            cerr << "error: unable to parse " << argv[1] << endl;
            return 1;
        }

        amf.output_makefile("Makefile");
    }

    return 0;
}
