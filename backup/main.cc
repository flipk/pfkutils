/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "options.h"
#include "bakfile.h"

#include <string>
#include <iostream>

using namespace std;

static int
usage(void)
{
    return 1;
}

extern "C" int
pfkbak_main(int argc, char ** argv)
{
    bkOptions  opts;

    if (opts.parse(argc, argv) == false)
        return 1;

    bakFile   file(opts);
    file.operate();

    return 0;
}
