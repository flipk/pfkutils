/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>

 */

#include <stdio.h>
#include <iostream>
#include <fstream>

#include "automake_parser.h"
#include "tokenizer.h"

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
