
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "automake_parser.H"
#include "tokenizer.H"

using namespace std;

void
automake_file :: output_makefile(const string &input_filename, 
                                 const string &output_filename)
{
    ofstream outfile(output_filename.c_str());

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
    outfile << input_variables;
    outfile << output_variables;
    make_allrule(input_filename);
    make_depfilerules();
    make_targetlinkrules();
    make_targetobjrules();
    make_lexyaccrules();
    make_cleanrule();
    outfile << output_rules;
}
