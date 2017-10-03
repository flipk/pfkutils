
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
        amf.parse(argv[1]);
//        cout << amf;
        amf.find_targets();
        cout << amf.targets;
    }

    return 0;
}
