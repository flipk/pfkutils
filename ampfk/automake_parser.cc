
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
