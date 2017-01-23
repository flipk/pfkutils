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

#ifndef __logfile_h__
#define __logfile_h__

#include "childprocessmanager.h"
#include "options.h"

#include <string>
#include <iostream>
#include <vector>

void printError(int e /*=errno*/, const std::string &func);

class ZipProcessHandle : public ChildProcessManager::Handle {
    const Options &opts;
    const std::string fname;
    /*virtual*/ void handleOutput(const char *buffer, size_t len);
    /*virtual*/ void processExited(int status);
    bool done;
    std::string tempInputFileName;
    std::string tempOutputFileName;
    std::string finalOutputFileName;
public:
    ZipProcessHandle(const Options &_opts, const std::string &_fname);
    ~ZipProcessHandle(void);
    bool getDone(void) { return done; }
    const std::string &outputFilename(void) { return finalOutputFileName; }
};

class LogFile {
    const Options &opts;
    std::string logDir;
    std::string logFilebase;
    int counter;
    std::string currentLogFile;
    void nextLogFileName(void);
    size_t currentSize;
    bool openFile(void);
    // return true if a zip process was started
    bool closeFile(void);
    // return true if a zip process was started
    bool trimFiles(void);
    struct logFileEnt {
        logFileEnt(const std::string &_fname, time_t _t, bool _isOrig);
        std::string filename;
        time_t timestamp;
        bool isOriginal; // has ".%04d" suffix
        static bool sortTimestamp(const logFileEnt &a,
                                  const logFileEnt &b);
    };
    typedef std::vector<logFileEnt> LfeList;
    void globLogFiles(LfeList &list);
    std::ostream * currentStream;
    bool stayClosed;
    typedef std::map<pid_t,ZipProcessHandle*> zipList;
    zipList zipHandles;
    bool initialized;
public:
    LogFile(const Options &_opts);
    ~LogFile(void);
    void init(void);
    typedef std::vector<std::string> FilenameList_t;
    // returns list of zip files completed.
    void periodic(FilenameList_t &list);
    void addData(const char * data, size_t len);
    const bool isOpen(void) const;
    // dont call this if isOpen return false.
    const std::string &getFilename(void) const;
    // return true if this caused a zip to start
    bool rolloverNow(void);
    // return true if this caused a zip to start
    bool closeNow(void);
    void openNow(void);
};

#endif /* __logfile_h__ */
