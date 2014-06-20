/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __PFK_HSM_H__
#define __PFK_HSM_H__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

namespace PFKHSM {

enum HSMErrorType {
    HSMErrorHandleProbe,
    HSMErrorBogusAct,
    HSMErrorInitialTrans,
    HSMErrorEntryHandler,
    HSMErrorExitHandler
};

struct HSMError {
    HSMErrorType type;
    std::string str;
    HSMError(HSMErrorType t) : type(t) { }
};

enum HSMEventType {
    __HSM_PROBE,
    HSM_ENTRY,
    HSM_EXIT,
    HSM_USER_START
};

struct HSMEvent {
    HSMEvent(int _type)
        : type(_type) { }
    virtual ~HSMEvent(void) { }
    virtual const std::string evtName(void) const { return "HSMEvent"; }
    int type; // actually HSMEventType
};

template <class T>
class HSM {
public:
    struct Action;
private:
    bool debug;
    enum ActType { ACT_HANDLED, ACT_TRANS, ACT_SUPER, ACT_TOP, NUMACTS };
    typedef Action (T::*State)(HSMEvent const *);
    static const std::string ActTypeNames[NUMACTS];
    State currentState;
    struct StateTraceEntry {
        State state;
        char const * name;
        StateTraceEntry(State _state, char const * _name);
    };
    typedef std::vector<StateTraceEntry> StateTrace;
    StateTrace     trace1;
    StateTrace     trace2;
    StateTrace  *  currentTrace;
    StateTrace  *  oldTrace;
    char const * stateName(State state,
                           State *nextState = NULL,
                           bool *top = NULL);
    void backtrace(StateTrace *traceret, State state);
    std::string trace2str(StateTrace *trace);
public:
    struct Action {
        ActType act;
        State state;
        char const * name;
        Action(void) { }
        Action(ActType _act, State _state, char const *_name);
    };
    struct HANDLED : Action { HANDLED(void ); };
    struct   TRANS : Action {   TRANS(State s); };
    struct   SUPER : Action {   SUPER(State s, char const * __name); };
    struct     TOP : Action {     TOP(void ); };
    HSM( bool _debug = false );
    virtual ~HSM(void) { }
    void set_debug(bool _debug) { debug = _debug; }
    void HSMInit(void);
    virtual Action initial(void) = 0;
    void dispatch(HSMEvent const * evt);
};

#include "HSM.tcc"

class ActiveHSMBase;

class HSMScheduler {
public:
    HSMScheduler(void);
    ~HSMScheduler(void);
    void registerHSM(ActiveHSMBase *);
    void deregisterHSM(ActiveHSMBase *);
    void subscribe(ActiveHSMBase *, int type);
};

// xxx rendevous object that manages subscriptions and queues
// xxx message pool objects
// xxx messages come from pool and are garbage-collected
// xxx pools and messages from thread_slinger
// xxx make HSM<T> methods into TCC 

class ActiveHSMBase
{
protected:
    HSMScheduler * sched;
public:
    ActiveHSMBase(HSMScheduler * _sched)
        : sched(_sched)
    {
        sched->registerHSM(this);
    }
    virtual ~ActiveHSMBase(void)
    {
        sched->deregisterHSM(this);
    }
};

template <class T>
class ActiveHSM : public ActiveHSMBase, public HSM<T>
{
protected:
    void subscribe(int type) { sched->subscribe(this,type); }
    void publish(HSMEvent const * event);
public:
    ActiveHSM( HSMScheduler * __sched, bool __debug = false )
        : ActiveHSMBase(__sched),
          HSM<T>(__debug)
    {
        // xxx
    }
    virtual ~ActiveHSM(void)
    {
        // xxx
    }
};

};

#endif /* __PFK_HSM_H__ */
