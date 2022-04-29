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

#ifndef __options_h__
#define __options_h__

#include <string>
#include <vector>

#include "childprocessmanager.h"

class Options {
    int argc;
    char ** argv;
    int ind;
    bool outOfArgs(void);
    const char * nextArg(void);
    typedef long long unsigned int LargeInt;
    bool getInteger(LargeInt &value); // return false if error
    bool getString(std::string &str); // return false if error
public:
    Options(int argc, char ** argv);
    void printHelp(void) const;
    void printOptions(void) const;

    bool debug;
    bool isError;
    bool isRemoteCmd;
    std::string remoteCmd;
    std::string logfileBase;
    bool maxSizeSpecified;
    size_t maxSize;
    bool zipSpecified;
    enum { ZIP_NONE, ZIP_GZIP, ZIP_BZIP2, ZIP_XZ } zipProgram;
    bool maxFilesSpecified;
    int maxFiles;
    bool backgroundSpecified;
    bool noReadSpecified;
    bool noOutputSpecified;
    std::string pidFile;
    bool commandSpecified;
    bool listenPortSpecified;
    short listenPort;
    ChildProcessManager::commandVector command;
};

#endif /* __options_h__ */
