/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/** \file  HSM.h */
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
#include "signal_backtrace.h"
#include "hsmthread.h"
#include "thread_slinger.h"

namespace HSM {

/** HSM-related errors that may be encountered */
struct HSMError : BackTraceUtil::BackTrace {
    /** list of HSM-related errors that might be thrown */
    enum HSMErrorType {
        HSMErrorHandleProbe,  //!< states must NOT handle __HSM_PROBE
        HSMErrorBogusAct,     //!< must return HANDLED, TRANS, TOP, SUPER only
        HSMErrorInitialTrans, //!< initial transition must return TRANS
        HSMErrorEntryHandler, //!< entry handlers must only return HANDLED
        HSMErrorExitHandler,  //!< exit handlers must only return HANDLED
        __NUMERRS
    } type;
    static const std::string errStrings[__NUMERRS];
    std::string str;
    HSMError(HSMErrorType t) : type(t) { }
    /** utility function to format printable string of error and backtrace */
    /*virtual*/ const std::string _Format(void) const;
};
   
/** base class for an HSMEvent, see HSMEventT and HSM_EVENT_DECLARE. */
struct HSMEvent : public ThreadSlinger::thread_slinger_message {
    HSMEvent(int _type) : type((HSMEvent::Type)_type) { }
    virtual ~HSMEvent(void) { }
    /** event name, user events should override this, although
     * this is done automatically by HSM_EVENT_DECLARE */
    virtual const std::string evtName(void) const { return "HSMEvent"; }
    /** type of the event, actually HSMEvent::Type */
    int type;
    /*virtual*/ const std::string msgName(void) { return evtName(); }
    /** some event types are reserved by the HSM library */
    enum Type {
        __HSM_PROBE,
        HSM_ENTRY,    //!< states handle this on entry, return HANDLED
        HSM_EXIT,     //!< states handle this on exit, return HANDLED
        HSM_TERMINATE,
        HSM_USER_START  //!< define your own event types starting at this value
    };
};

/** base template for a user event, see \ref HSM_EVENT_DECLARE for a simpler
 * way to define your own events.
 * \param T   the user's data type
 * \param typeVal  the enum value of your type starting at HSM_USER_START */
template <class T, int typeVal>
struct HSMEventT : public HSMEvent {
    HSMEventT(void) : HSMEvent(typeVal) { }
    virtual ~HSMEventT(void) { }
    /** allocate a new instance of an object from a memory pool; 
     * the memory pool grows automatically, see
     * \ref ThreadSlinger::thread_slinger_pools::report_pools */
    static T * alloc(void);
};

/** this is how to declare your own event, an easier way than HSMEventT.
 * \param __className  the name of the class of the event you're creating
 * \param __typeValue  a type value from an enum starting at HSM_USER_START
 * \param __body   optionally define contents of the event */
#define HSM_EVENT_DECLARE(__className,__typeValue,__body)           \
struct __className : ::HSM::HSMEventT<__className,__typeValue>      \
{                                                                   \
    const std::string evtName(void) const override                  \
    { return #__className; }                                        \
    __body                                                          \
}

/** base class for a state machine without a thread */
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
    /** a state will receive this when it is entered */
    static const HSMEvent::Type HSM_ENTRY = HSMEvent::HSM_ENTRY;
    /** a state will receive this when it is exited */
    static const HSMEvent::Type HSM_EXIT = HSMEvent::HSM_EXIT;
    /** a state should return this to indicate what is supposed
     * to be done; it should be one of 
     * \ref HANDLED
     * \ref TRANS
     * \ref SUPER
     * \ref TOP */
    struct Action {
        ActType act;
        State state;
        char const * name;
        Action(void) { }
        Action(ActType _act, State _state, char const *_name);
    };
    /** return this to indicate event handling is complete with
     * no action to be taken. */
    struct HANDLED : Action { HANDLED(void ); };
    /** return this to indicate the state machine should transition
     * to another state. argument is &myClass::nextState. */
    struct   TRANS : Action {   TRANS(State s); };
    /** return this to indicate this state function will not handle
     * this event, but the super (parent) state may handle this state.
     * this is also used to determine a string name for the current state.
     * arguments are &myClass::superState, std::string thisStateName */
    struct   SUPER : Action {   SUPER(State s, char const * __name); };
    /** return this from the top state to indicate this is the top state
     * (there is no super). */
    struct     TOP : Action {     TOP(void ); };
    HSM( bool _debug = false );
    virtual ~HSM(void) { }
    /** if debug is set to true, printouts will occur as messages
     * are dispatched and states transition */
    void set_debug(bool _debug) { debug = _debug; }
    /** call this to initialize this state machine */
    void HSMInit(void);
    /** user must implement an initial method. you may do
     * whatever you wish, except it must return TRANS to the first state. */
    virtual Action initial(void) = 0;
    /** invoke this method to pass an event into the state machine. */
    void dispatch(HSMEvent const * evt);
};

#include "HSM.tcc"

}; //namespace HSM

#endif /* __HSM_H__ */
