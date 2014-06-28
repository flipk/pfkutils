/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __HSM_H__
#define __HSM_H__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include "dll3.h"
#include "throwBacktrace.h"
#include "hsmthread.h"
#include "thread_slinger.h"

namespace HSM {

struct HSMError : ThrowUtil::ThrowBackTrace {
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
    HSM_TERMINATE,
    HSM_USER_START
};

struct HSMEvent : public ThreadSlinger::thread_slinger_message {
    HSMEvent(int _type) : type((HSMEventType)_type) { }
    virtual ~HSMEvent(void) { }
    virtual const std::string evtName(void) const { return "HSMEvent"; }
    int type; // actually HSMEventType
    /*virtual*/ const std::string msgName(void) { return evtName(); }
};

template <class T, int typeVal>
struct HSMEventT : public HSMEvent {
    HSMEventT(void) : HSMEvent(typeVal) { }
    virtual ~HSMEventT(void) { }
    static T * alloc(void);
};

#define HSM_EVENT_DECLARE(__className,__typeValue,__body) \
struct __className : HSMEventT<__className,__typeValue>     \
{ \
    const std::string evtName(void) const { return #__className; } \
    __body \
}

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
    HSMEvent  exitEvt;
    HSMEvent  entryEvt;
    HSMEvent  probeEvt;
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

class HSMScheduler;

struct HSMEventEnvelope : public ThreadSlinger::thread_slinger_message {
    HSMEvent * evt;
    const std::string msgName(void) const { return "HSMEventEnvelope"; }
};

class ActiveHSMBase : public HSMThread::Thread,
                      public ActiveHSMList_t::Links
{
protected:
    HSMScheduler * sched;
    /*virtual*/ void entry(void);
    /*virtual*/ void stopReq(void);
    ThreadSlinger::thread_slinger_queue<HSMEventEnvelope>  eventQueue;
    HSMEvent termEvent;
public:
    ActiveHSMBase(HSMScheduler * _sched, const std::string &name);
    virtual ~ActiveHSMBase(void);
    virtual void init(void) = 0;
    virtual void AHSMdispatch(HSMEvent const * evt) = 0;
    void enqueue(HSMEvent * env);
};

template <class T>
class ActiveHSM : public ActiveHSMBase, public HSM<T>
{
public:
    ActiveHSM( HSMScheduler * __sched, const std::string &name,
               bool __debug = false );
    virtual ~ActiveHSM(void);
protected:
    void init(void);
    void AHSMdispatch(HSMEvent const * evt);
    void subscribe(int type);
    void publish(HSMEvent * evt);
};

#define ACTIVE_HSM_BASE(__className)  ActiveHSM<__className>
#define ACTIVE_HSM_DECLARE(__className) \
    class __className : public ACTIVE_HSM_BASE(__className)

struct HSMSubEntry;
class HSMSubEntryHash;
typedef DLL3::Hash<HSMSubEntry,HSMEventType,HSMSubEntryHash,1> HSMSubHash_t;

struct HSMSubEntry : public HSMSubHash_t::Links
{
    HSMEventType type;
    std::vector<ActiveHSMBase*> activeObjs;
    HSMSubEntry(HSMEventType _t) : type(_t) { }
    ~HSMSubEntry(void) throw () { }
};
class HSMSubEntryHash {
public:
    static uint32_t obj2hash(const HSMSubEntry &item)
    { return (uint32_t) item.type; }
    static uint32_t key2hash(const HSMEventType key)
    { return (uint32_t) key; }
    static bool hashMatch(const HSMSubEntry &item, const HSMEventType key)
    { return (item.type == key); }
};

class HSMScheduler {
    ActiveHSMList_t  active_hsms;
    HSMSubHash_t  subHash;
    ThreadSlinger::thread_slinger_pool<HSMEventEnvelope>  eventEnvelopePool;
public:
    HSMScheduler(void);
    ~HSMScheduler(void);
    void registerHSM(ActiveHSMBase *);
    void deregisterHSM(ActiveHSMBase *);
    void subscribe(ActiveHSMBase *, HSMEventType type);
    void publish(HSMEvent * evt);
    void start(void);
    void stop(void);
    HSMEventEnvelope *allocEnv(void);
    void releaseEnv(HSMEventEnvelope *env);
};

#include "HSM.tcc"

}; // namespace HSM

#endif /* __HSM_H__ */
