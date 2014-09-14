/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __FDTHREADLAUNCHER_H__
#define __FDTHREADLAUNCHER_H__

#include <iostream>
#include <inttypes.h>

class fdThreadLauncher {
public:
    enum fdThreadState {
        INIT, STARTING, RUNNING, STOPPING, DEAD
    } state;
private:
    bool threadRunning;
    enum { CMD_CLOSE, CMD_CHANGEPOLL };
    int cmdFds[2];
    static void * _threadEntry(void *);
    void threadEntry(void);
    int pollInterval;
protected:
    // return false to close
    int fd;
    virtual bool doSelect(bool *forRead, bool *forWrite) = 0;
    virtual bool handleReadSelect(int fd) = 0;
    virtual bool handleWriteSelect(int fd) = 0;
    virtual bool doPoll(void) = 0;
    virtual void done(void) = 0;
public:
    fdThreadLauncher(void);
    virtual ~fdThreadLauncher(void);
    // poll interval units are in milliseconds.
    void startFdThread(int _fd, int _pollInterval = -1);
    void stopFdThread(void);
    void setPollInterval(int _pollInterval);
    int acceptConnection(void);
    static int makeListeningSocket(int port);
    static int makeConnectingSocket(uint32_t ip, int port);
    static int makeConnectingSocket(const std::string &host, int port);
};
std::ostream &operator<<(std::ostream &ostr,
                         const fdThreadLauncher::fdThreadState state);

#endif /* __FDTHREADLAUNCHER_H__ */
