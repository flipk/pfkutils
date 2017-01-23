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
#include <unistd.h>
#include <limits.h>

#include "automake_parser.h"
#include "tokenizer.h"

using namespace std;

automake_file :: automake_file(void)
{
}

automake_file :: ~automake_file(void)
{
    amvariable * v;
    amrule * r;
    amtarget * t;
    while ((v = input_variables.dequeue_head()) != NULL)
        delete v;
    while ((r = input_rules.dequeue_head()) != NULL)
        delete r;
    while ((t = targets.dequeue_head()) != NULL)
        delete t;
    while ((v = output_variables.dequeue_head()) != NULL)
        delete v;
    while ((r = output_rules.dequeue_head()) != NULL)
        delete r;
}

bool
automake_file :: parse(const string &fname)
{
    char currpath[PATH_MAX];
    if (getcwd(currpath, sizeof(currpath)) == NULL)
    { /*quiet compiler*/ }
    builddir = currpath;
    srcdir = currpath;
    size_t pos = fname.find_last_of('/');
    if (pos != string::npos)
    {
        srcdir = fname;
        srcdir.erase(pos);
        if (chdir(srcdir.c_str()) < 0)
        { /*quiet compiler*/ }
        char srcpath[PATH_MAX];
        if (getcwd(srcpath, sizeof(srcpath)) == NULL)
        { /*quiet compiler*/ }
        srcdir = srcpath;
        if (chdir(currpath) < 0)
        { /*quiet compiler*/ }
    }

    FILE * in = fopen(fname.c_str(), "r");
    if (!in)
        return false;
    tokenizer_init(in);

    parser_amf = this;
    yyparse();
    parser_amf = NULL;

    fclose(in);
    return true;
}

void
automake_file :: tokenize(const string &fname)
{
    FILE * in = fopen(fname.c_str(), "r");
    if (!in)
        return;
    tokenizer_init(in);
    print_tokenized_file();
    fclose(in);
}
