/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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
    void printHelp(void);
    void printOptions(void);

    bool isError;
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
