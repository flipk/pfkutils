/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __logfile_h__
#define __logfile_h__

#include "childprocessmanager.h"
#include "options.h"

#include <string>
#include <iostream>
#include <vector>

void printError(const std::string &func);

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
    void closeFile(void);
    void trimFiles(void);
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
    typedef std::map<pid_t,ZipProcessHandle*> zipList;
    zipList zipHandles;
public:
    LogFile(const Options &_opts);
    ~LogFile(void);
    void periodic(void);
    void addData(const char * data, size_t len);
    bool isError;
};

#endif /* __logfile_h__ */
