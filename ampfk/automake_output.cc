
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "automake_parser.H"
#include "tokenizer.H"

using namespace std;

void
automake_file :: output_makefile(const string &filename)
{
    ofstream outfile(filename.c_str());

    if (0)
        cout << " **** input_variables ****" << endl
             << input_variables;
    if (0)
        cout << " **** input_rules ****" << endl
             << input_rules;
    find_targets();
    if (0)
        cout << " **** targets ****" << endl
             << targets;

    make_variables();

    // figure out srcdir
    set_srcdir(".."); // xxx

    outfile << input_variables;
    outfile << output_variables;
    make_allrule();
    make_depfilerules();
    make_targetlinkrules();
    make_targetobjrules();
    make_lexyaccrules();
    make_cleanrule();
    outfile << output_rules;
}
