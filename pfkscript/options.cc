/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "options.h"
#include <stdlib.h>
#include <string>
#include <iostream>

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
    maxFilesSpecified = false;
    backgroundSpecified = false;
    commandSpecified = false;
    noReadSpecified = false;
    noOutputSpecified = false;

    if (outOfArgs())
        return;

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
        else if (arg == "-z")
        {
            if (getString(zipProgram) == false)
                return;
            zipSpecified = true;
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

    if (zipSpecified && !maxSizeSpecified)
    {
        cerr << "-z requires -s\n";
        return;
    }
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
"                  [-z [bzip2|gzip|xz|etc]] [-c command....]\n"
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
"   -z program   : requires -s. when each log file is closed, it is compressed\n"
"                  by forking the specified program. the compression program\n"
"                  as run as a low-priority background process.\n"
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
"   -c command   : must be the last command line parameter. all command line\n"
"                  parameters following -c are assumed to be for the command\n"
"                  and will not be otherwise interpreted by pfkscript.\n"
"                  if -c not present, the command is assumed to be $SHELL\n"
"                  (in which case -b is illegal).\n"
"\n";
}
