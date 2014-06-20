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

#include "dll3.H"
#include "throwBacktrace.h"

namespace PFKHSM {

struct HSMError : PFK::ThrowBackTrace {
    enum HSMErrorType {
        HSMErrorHandleProbe,
        HSMErrorBogusAct,
        HSMErrorInitialTrans,
        HSMErrorEntryHandler,
        HSMErrorExitHandler,
        __NUMERRS
    } type;
    static const std::string errStrings[__NUMERRS];
    std::string str;
    HSMError(HSMErrorType t) : type(t) { }
    const std::string Format(void) const;
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

class ActiveHSMBase;

typedef DLL3::List<ActiveHSMBase,1> ActiveHSMList_t;

class HSMScheduler {
    ActiveHSMList_t  active_hsms;
public:
    HSMScheduler(void);
    ~HSMScheduler(void);
    void registerHSM(ActiveHSMBase *);
    void deregisterHSM(ActiveHSMBase *);
    void subscribe(ActiveHSMBase *, int type);
    void publish(HSMEvent const * evt);
};

// xxx rendevous object that manages subscriptions and queues
// xxx message pool objects
// xxx messages come from pool and are garbage-collected
// xxx pools and messages from thread_slinger

class ActiveHSMBase : public ActiveHSMList_t::Links
{
protected:
    HSMScheduler * sched;
public:
    ActiveHSMBase(HSMScheduler * _sched);
    virtual ~ActiveHSMBase(void);
};

template <class T>
class ActiveHSM : public ActiveHSMBase, public HSM<T>
{
public:
    ActiveHSM( HSMScheduler * __sched, bool __debug = false );
    virtual ~ActiveHSM(void);
protected:
    void subscribe(int type);
    void publish(HSMEvent const * event);
};

#include "HSM.tcc"

}; // namespace PFKHSM

#endif /* __PFK_HSM_H__ */
