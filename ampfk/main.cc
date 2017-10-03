
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
//        cout << " **** input_variables ****" << endl;
//        cout << amf.input_variables;
//        cout << " **** input_rules ****" << endl;
//        cout << amf.input_rules;
        amf.find_targets();
//        cout << " **** targets ****" << endl;
//        cout << amf.targets;
        amf.make_variables();
        amf.set_srcdir(".."); // xxx
//        cout << " **** output_variables ****" << endl;
        cout << amf.input_variables;
        cout << amf.output_variables;
        amf.make_allrule();
        amf.make_depfilerules();
        amf.make_targetlinkrules();
        amf.make_targetobjrules();
        amf.make_lexyaccrules();
        amf.make_cleanrule();
        cout << amf.output_rules;

        // output file gets: output_variables, output_rules, input_rules

    }

    return 0;
}
