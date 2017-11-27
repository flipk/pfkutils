/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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
    string filepart, dirtree, password;

    if (colon_pos != string::npos)
    {
        filepart = filepath.substr(0,colon_pos);
        // including the colon
        password = filepath.substr(colon_pos);
    }
    else
    {
        filepart = filepath;
    }

    if (filepart[filepart.length()-1] == '/')
    {
        dirtree = "/";
        filepart.resize(filepart.length()-1);
    }

#if SINGLE_FILE_BACKUP
    backupfile_data  = filepart            + dirtree + password;
#else
    backupfile_index = filepart + ".index" + dirtree + password;
    backupfile_data  = filepart + ".data"  + dirtree + password;
#endif

#if 0 // debug only
    cout << "filepath = " << filepath << endl;
    cout << "filepart = " << filepart << endl;
    cout << "dirtree = " << dirtree << endl;
    cout << "password = " << password << endl;
#if SINGLE_FILE_BACKUP
    cout << "backupfile_index = " << backupfile_index << endl;
#endif
    cout << "backupfile_data = " << backupfile_data << endl;
#endif

    string op_str = argv[1];

    for (uint32_t p = 0; p < op_str.length(); p++)
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
