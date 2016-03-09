/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "options.h"

#include <stdlib.h>

#include <string>
#include <iostream>

using namespace std;

bkOptions::bkOptions(void)
{
    op = OP_NONE;
    verbose = false;
}

void
bkOptions::printUsage(void)
{
    cout << 
     "pfkbak c[vv] file.bak:pass dir                       : create backup\n"
     "pfkbak u[vv] file.bak:pass                           : update backup\n"
     "pfkbak t[vv] file.bak:pass                           : list contents\n"
     "pfkbak d[vv] file.bak:pass ver [ver...]              : delete vers\n"
     "pfkbak x[vv] file.bak:pass ver [path...]             : extract ver\n"
     "pfkbak X[vv] file.bak:pass ver outfile.tar [path...] : export ver to tar\n"
        ;
}

bool
bkOptions::parse(int argc, char ** argv)
{
    bool ret = _parse(argc,argv);
    if (ret == false)
        printUsage();
    return ret;
}

bool
bkOptions::_parse(int argc, char ** argv)
{
    if (argc < 3)
        return false;

    string filepath = argv[2];

    size_t colon_pos = filepath.find_first_of(':');
    if (colon_pos != string::npos)
    {
        backupfile_index =
            filepath.substr(0,colon_pos) + ".index" +
            filepath.substr(colon_pos);
        backupfile_data  =
            filepath.substr(0,colon_pos) + ".data" +
            filepath.substr(colon_pos);
    }
    else
    {
        backupfile_index = filepath + ".index";
        backupfile_data  = filepath + ".data";
    }

    string op_str = argv[1];

    for (int p = 0; p < op_str.length(); p++)
    {
        char c = op_str[p];
        switch (c)
        {
        case 'c':
            op = OP_CREATE;
            if (argc != 4)
                return false;
            sourcedir = argv[3];
            break;
        case 'u':
            op = OP_UPDATE;
            if (argc != 3)
                return false;
            break;
        case 't':
            op = OP_LIST;
            if (argc != 3)
                return false;
            break;
        case 'd':
            op = OP_DELETE;
            if (argc < 4)
                return false;
            for (int ctr = 3; ctr < argc; ctr++)
                versions.push_back(atoi(argv[ctr]));
            break;
        case 'x':
            op = OP_EXTRACT;
            if (argc < 4)
                return false;
            versions.push_back(atoi(argv[3]));
            for (int ctr = 4; ctr < argc; ctr++)
                paths.push_back(argv[ctr]);
            break;
        case 'X':
            op = OP_EXPORT;
            if (argc < 5)
                return false;
            versions.push_back(atoi(argv[3]));
            tarfile = argv[4];
            for (int ctr = 5; ctr < argc; ctr++)
                paths.push_back(argv[ctr]);
            break;
        case 'v':
            verbose ++;
            break;
        default:
            return false;
        }
    }
    
    return true;
}
