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

#include <stdio.h>

using namespace std;

bool
Options :: outOfArgs(void)
{
    if (ind == argc)
        return true;
    return false;
}

const char *
Options :: nextArg(void)
{
    if (ind == argc)
        return NULL;
    return argv[ind++];
}

bool
Options :: getInteger(LargeInt &value)
{
    if (outOfArgs())
        return false;
    const char * valueStr = nextArg();
    char * valueEnd = NULL;
    value = strtoull(valueStr, &valueEnd, 0);
    if (valueEnd == valueStr)
        // none of the chars were digits
        return false;
    if (*valueEnd == 0)
        // all the chars were digits
        return true;
    // there's one or more chars left
    if (valueEnd[1] == 0)
    {
        // there's only one char left
        switch (valueEnd[0])
        {
        case 'k': case 'K': // kilobytes
            value *= 1024;
            return true;
        case 'm': case 'M': // megabytes
            value *= 1024 * 1024;
            return true;
        case 'g': case 'G': // gigabytes
            value *= 1024 * 1024 * 1024;
            return true;
        }
    }
    // remaining chars are bogus
    return false;
}

bool
Options :: getString(std::string &str)
{
    if (outOfArgs())
        return false;
    str.assign(nextArg());
    return true;
}

Options :: Options( int _argc, char ** _argv )
    : argc(_argc), argv(_argv), ind(0), isError(true)
{
    maxSizeSpecified = false;
    zipSpecified = false;
    zipProgram = ZIP_NONE;
    maxFilesSpecified = false;
    backgroundSpecified = false;
    commandSpecified = false;
    noReadSpecified = false;
    noOutputSpecified = false;
    listenPortSpecified = false;
    isRemoteCmd = false;

    if (outOfArgs())
        return;

    if (argc == 2 && string(argv[0]) == "-r")
    {
        remoteCmd = argv[1];
        isRemoteCmd = true;
        isError = false;
        return;
    }

    logfileBase.assign(nextArg());

    while (!outOfArgs())
    {
        string  arg = nextArg();

        if (arg == "-s")
        {
            LargeInt v;
            if (getInteger(v) == false)
                return;
            maxSize = (size_t) v;
            maxSizeSpecified = true;
        }
        else if (arg == "-zg")
        {
            zipSpecified = true;
            zipProgram = ZIP_GZIP;
        }
        else if (arg == "-zb")
        {
            zipSpecified = true;
            zipProgram = ZIP_BZIP2;
        }
        else if (arg == "-zx")
        {
            zipSpecified = true;
            zipProgram = ZIP_XZ;
        }
        else if (arg == "-m")
        {
            LargeInt v;
            if (getInteger(v) == false)
                return;
            maxFiles = (int) v;
            maxFilesSpecified = true;
        }
        else if (arg == "-n")
        {
            noReadSpecified = true;
        }
        else if (arg == "-O")
        {
            noOutputSpecified = true;
        }
        else if (arg == "-b")
        {
            if (getString(pidFile) == false)
                return;
            backgroundSpecified = true;
        }
        else if (arg == "-l")
        {
            LargeInt v;
            if (getInteger(v) == false)
                return;
            if (v < 50 || v > 32767)
            {
                cerr << "invalid listen port number " << v << endl;
                return;
            }
            listenPort = (short) v;
            listenPortSpecified = true;
        }
        else if (arg == "-c")
        {
            while (!outOfArgs())
            {
                const char * arg = nextArg();
                command.push_back(arg);
            }
            command.push_back(NULL);
            commandSpecified = true;
        }
        else
        {
            return;
        }
    }

    // now enforce the required/mutually exclusive rules

    // (note: allow -z without -s, for support of remote "close" command.
    if (maxFilesSpecified && !maxSizeSpecified)
    {
        cerr << "-m requires -s\n";
        return;
    }
    if (backgroundSpecified && !commandSpecified)
    {
        cerr << "-b requires -c\n";
        return;
    }
    if (!commandSpecified)
    {
        // if no -c, assume $SHELL
        const char * shellEnv = getenv("SHELL");
        if (!shellEnv)
            shellEnv = "/bin/sh";
        string shell = shellEnv;
        command.push_back(shellEnv);
        command.push_back(NULL);
    }
    isError = false;
}

void
Options :: printOptions(void)
{
    if (isError)
    {
        cout << "isError = true;\n";
        return;
    }
    cout << "logfileBase = " << logfileBase << endl;
    if (maxSizeSpecified)
        cout << "maxSize = " << maxSize << endl;
    if (zipSpecified)
        cout << "zipProgram = " << zipProgram << endl;
    if (maxFilesSpecified)
        cout << "maxFiles = " << maxFiles << endl;
    if (backgroundSpecified)
        cout << "pidFile = " << pidFile << endl;
    if (noReadSpecified)
        cout << "noRead" << endl;
    if (noOutputSpecified)
        cout << "noOutput" << endl;
    if (listenPortSpecified)
        cout << "listenPort = " << listenPort << endl;
    cout << "command = ";
    // command vector ends in NULL so stop 1 before end
    for (int ind = 0; ind < (command.size()-1); ind++)
        cout << command[ind] << " ";
    cout << endl;
}

void
Options :: printHelp(void)
{
    cerr <<
"\n"
"pfkscript logfile [-b pid_path] [-s max_size_in_mb] [-m max_files] \n"
"                  [-zg|-zb|-zx] [-c command....]\n"
"\n"
"   logfile      : required. if -s is not present, this is the file name all\n"
"                  command output will be logged to. if -s is present, this is\n"
"                  the basename of the file (see -s).\n"
"   -s max_size  : when this option is present, 'logfile' is a basename,\n"
"                  appended by '.%%04d', counter starting at 0001. when each\n"
"                  file reaches max_size (specified in MiB) it is closed,\n"
"                  the counter is incremented and a new file is opened.\n"
"                  the chars k, m, and g are supported as suffixes for\n"
"                  kilobyte, megabyte, and gigabyte.\n"
"   -zX          : requires -s. when each log file is closed, it is compressed\n"
"                  by forking the specified program; X can be:\n"
"                  g = gzip, b = bzip2, or x = xz\n"
"   -n           : no read from stdin\n"
"   -O           : write only to log, no copy to stdout\n"
"   -m max_files : requires -s. when each log file is closed, an old file\n"
"                  may be removed, if the number of files matching the\n"
"                  glob pattern 'logfile*' exceeds max_files.\n"
"   -b pid_path  : when present, pfkscript forks into the background, detaches\n"
"                  stdin and stdout, and runs the specified command (requires\n"
"                  -c). the output of the command is collected, but no input\n"
"                  to the command is possible (since pfkscript has detached\n"
"                  and gone background). the process id of the command is\n"
"                  written to the file specified by pid_path. when -b is not\n"
"                  present, pfkscript runs in the foreground and passes all\n"
"                  user input to the running command.\n"
"   -l port      : tcp port number for listening to peek in.\n"
"   -r cmd       : issue remote cmd to parent pfkscript.\n"
"                  <cmd> is one of: close open rollover getfile\n"
"   -c command   : must be the last command line parameter. all command line\n"
"                  parameters following -c are assumed to be for the command\n"
"                  and will not be otherwise interpreted by pfkscript.\n"
"                  if -c not present, the command is assumed to be $SHELL\n"
"                  (in which case -b is illegal).\n"
"\n";
}
