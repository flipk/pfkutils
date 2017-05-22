
// ** read stdin and produce stdout
//  hilite config.hilite

// ** read input file and produce stdout
//  hilite config.hilite inputfile

// ** read input file and produce output file
//  hilite config.hilite inputfile outputfile

// config.hilite:
//   # comment
//   "regexp_pattern" color [colormod...]
//   [one per line]
// where color is one of:
//     black red green yellow blue purple cyan white
// where colormod is one of:
//     bold inverse bright

#include "config_file.h"
#include <stdio.h>
#include <iostream>

using namespace std;

extern "C" int
hilite_main()
{
    Hilite_config  config;

    if (!config.parse_file("example.hilite"))
    {
        printf("config file parse fail\n");
        return 1;
    }

    int lineno = 1;
    while (1)
    {
        string line;
        getline(cin, line);
        if (!cin.good())
            break;
        config.colorize_line(line, lineno);
        printf("%s\n", line.c_str());
        lineno++;
    }

    return 0;
}
