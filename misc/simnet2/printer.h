/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "thread_slinger.h"
#include "pfkposix.h"
#include "bufprintf.h"
#include <vector>
#include <string>

class PrintMessage : public ThreadSlinger::thread_slinger_message {
public:
    const std::string msg;
    PrintMessage(const std::string &_msg) : msg(_msg) { }
};

class Printer : public pfk_pthread {
    ThreadSlinger::thread_slinger_queue<PrintMessage>   q;
    /*virtual*/ void * entry(void);
    /*virtual*/ void send_stop(void);
    static Printer * _instance;
public:
    static Printer &instance(void) { return *_instance; }
    Printer(void);
    ~Printer(void);
    void print(const std::string &msg) {
        PrintMessage *m = new PrintMessage(msg);
        q.enqueue(m);
    }
};

extern Printer printer;
