
// rules:
//   - initial state:
//     - Entry event must always return TRANS
//     - no other events will be dispatched to initial
//   - all states:
//     - EntryEvent must return either HANDLED or TRANS
//     - ExitEvent must always return HANDLED
//     - default case must return SUPER or TOP
//     - all other events may return any of TOP/SUPER/TRANS/HANDLED

#include "thread2thread2.h"
#include "simpleRegex.h"
#include <unordered_map>

namespace t2t2 = Thread2Thread2;

// the HSM system reserves three event types for itself.
// user events should start at USER_START.
struct HSMEventType {
    enum { TRACE, ENTRY, EXIT, USER_START };
    int val;
    HSMEventType(int v) : val(v) { }
};

// all events are derived from this base type. active objects
// would use queue_t for handling incoming event streams.
// user event definitions should override the format() method below
// so that "cout << evt" works on all events.
// note multiple operator<< overloads are provided which handle
// shared pointers, pointers, and refs.
struct HSMEventBase : public t2t2::t2t2_message_base<HSMEventBase>
{
    // convenience
    typedef t2t2::t2t2_queue<HSMEventBase> queue_t;
    typedef pxfe_shared_ptr<HSMEventBase>  sp_t;
    HSMEventType  event_type;
    const char * event_name;

    // the user's event classes should provide a unique integer
    // ID (derived from HSMEventType enum) and an event name.
    HSMEventBase(HSMEventType _event_type,
                 const char *_event_name)
        : event_type(_event_type), event_name(_event_name) { }
    virtual ~HSMEventBase(void) { }
private:
    // format method should be const; don't allow accidental
    // override of the non-const version.
    virtual void format(std::ostream &str) = delete;
public:
    // this is meant to be overridden by the user's event classes.
    virtual void format(std::ostream &str) const
    {
        str << "HSMEventBase("
            << event_name << "(" << event_type.val << "))";
    }
    std::string format_str(void) const
    { std::ostringstream s; format(s); return s.str(); }
};
static inline std::ostream &operator<<(std::ostream &s,
                                       const HSMEventBase *evt)
{ evt->format(s); return s; }
static inline std::ostream &operator<<(std::ostream &s,
                                       const HSMEventBase::sp_t &evt)
{ s << evt.get(); return s; }
static inline std::ostream &operator<<(std::ostream &s,
                                       const HSMEventBase &evt)
{ s << &evt; return s; }

class HsmFuncRegex : public pxfe_regex<> {
    static const char * patt;

#define HSM_FUNC_REGEX_PATTERN                                          \
    "(^"  /* IND_PROPER_SIGNATURE */                                    \
       "_HSMBase::Action "                                              \
       "([_a-zA-Z][_a-zA-Z0-9]{0,30})" /* IND_CLASSNAME */              \
       "::"                                                             \
       "([_a-zA-Z][_a-zA-Z0-9]{0,30})" /* IND_FUNCNAME */               \
       "\\(HSMEventBase::sp_t\\)"                                       \
    "$)|(^" /* IND_WRONG_SIGNATURE */                                   \
       "(.*) " /* IND_WRONG_RETTYPE */                                  \
       "([_a-zA-Z][_a-zA-Z0-9]{0,30})" /* IND_WRONG_CLASSNAME */        \
       "::"                                                             \
       "([_a-zA-Z][_a-zA-Z0-9]{0,30})" /* IND_WRONG_FUNCNAME */         \
       "\\((.*)\\)" /* IND_WRONG_ARGTYPE */                             \
    "$)"

public:
    static const int IND_PROPER_SIGNATURE  = 1;
    static const int IND_CLASSNAME         = 2;
    static const int IND_FUNCNAME          = 3;
    static const int IND_WRONG_SIGNATURE   = 4;
    static const int IND_WRONG_RETTYPE     = 5;
    static const int IND_WRONG_CLASSNAME   = 6;
    static const int IND_WRONG_FUNCNAME    = 7;
    static const int IND_WRONG_ARGTYPE     = 8;

    HsmFuncRegex(void) : pxfe_regex<>(patt) { }
};

class _HSMBase {
protected:
    struct Action; // forward
    typedef Action (_HSMBase::*StateFuncBase_t)(HSMEventBase::sp_t  evt);
    struct Action {
        enum acttype { NONE, HANDLED, TOP, SUPER, TRANS, _NUM_ACTIONS } type;
        // NOTE the derived actions must not have any storage!
        // all storage must be in this class.
        // the current state's name, as provided by
        // the user in TOP or SUPER.
        const char * statename;
        // in SUPER, the pointer to the super state;
        // in TRANS, the pointer to the new state.
        StateFuncBase_t state;
        Action(acttype t = NONE) : type(t) { }
        Action(const Action &o) {
            type = o.type;
            statename = o.statename;
            state = o.state;
        }
        const char * action_name(void) {
            switch (type) {
            case HANDLED: return "Handled";
            case TOP:     return "Top";
            case SUPER:   return "Super";
            case TRANS:   return "Transition";
            default:      return "<UNSET>";
            }
        }
    };

    struct _HANDLED final : Action {
        _HANDLED(const char *_statename) : Action(Action::HANDLED)
        { statename = _statename; state = NULL; }
    };
    struct _TOP final : Action {
        _TOP(const char *_statename) : Action(Action::TOP)
        { statename = _statename; state = NULL; }
    };
// the above 2 actions are simple and in the base class;
// the next 2 actions require some casting magic in the template
// derived class and so have _INTERNAL versions here plus real
// versions down below.
    struct _INTERNAL_SUPER : Action {
        _INTERNAL_SUPER(const char *_statename, StateFuncBase_t _super)
            : Action(Action::SUPER)
        { statename = _statename; state = _super; }
    };
    struct _INTERNAL_TRANS : Action {
        _INTERNAL_TRANS(const char *_statename, StateFuncBase_t _newstate)
            : Action(Action::TRANS)
        { statename = _statename; state = _newstate; }
    };

    // built-in events
    struct _TraceEvent final : HSMEventBase {
        typedef pxfe_shared_ptr<_TraceEvent>  sp_t;
        static const int TYPE = HSMEventType::TRACE;
        _TraceEvent(void) : HSMEventBase(TYPE, "TraceEvent") { }
    };
    struct EntryEvent final : HSMEventBase {
        typedef pxfe_shared_ptr<EntryEvent>  sp_t;
        static const int TYPE = HSMEventType::ENTRY;
        EntryEvent(void) : HSMEventBase(TYPE, "EntryEvent") { }
        virtual ~EntryEvent(void) { }
    };
    struct ExitEvent final : HSMEventBase {
        typedef pxfe_shared_ptr<ExitEvent>  sp_t;
        static const int TYPE = HSMEventType::EXIT;
        ExitEvent(void) : HSMEventBase(TYPE, "ExitEvent") { }
    };

    bool debug;
    std::string  hsm_name;

private:
    typedef t2t2::t2t2_pool<HSMEventBase,
                            _TraceEvent,
                            EntryEvent,
                            ExitEvent> entryexitpool_t;
    entryexitpool_t      entryexitpool;
    _TraceEvent::sp_t    trace_evt;
    EntryEvent ::sp_t    entry_evt;
    ExitEvent  ::sp_t    exit_evt;

    // convert a function-signature returned by PRETTY_FUNCTION in the
    // HSM_* macros into just the short function name by means of a
    // regular expression. (all state functions should be compliant to
    // the StateFuncBase_t signature anyway, so the regex should
    // work.)  but also, since regex takes time to execute, only do
    // the regex the first time a state function is encountered, and
    // build a cache of the returned short names.
    HsmFuncRegex  func_regex;
    typedef std::unordered_map<uintptr_t, std::string> shortname_cache_t;
    shortname_cache_t shortname_cache;
    const char * short_statename(const char *statename);
    const char * statename(StateFuncBase_t state,
                           StateFuncBase_t *nextState = NULL,
                           bool * top = NULL);

    struct TraceEntry {
        StateFuncBase_t state;
        const char * statename;
        TraceEntry(void) { }
        TraceEntry(StateFuncBase_t _state, const char * _name)
        { state = _state; statename = _name; }
        TraceEntry(const TraceEntry &o)
        { state = o.state; statename = o.statename; }
    };
    typedef std::vector<TraceEntry>  StateTrace_t;
    StateTrace_t   trace1;
    StateTrace_t   trace2;
    StateTrace_t   temp_trace;
    StateTrace_t * currentTrace;
    StateTrace_t * oldTrace;

    void backtrace(StateTrace_t *traceret, StateFuncBase_t state);
    std::string trace2str(StateTrace_t *trace);

    StateFuncBase_t currentState;

    bool initialized;

public:
    _HSMBase(StateFuncBase_t _initial_func, bool _debug = false);
    virtual ~_HSMBase(void);
    const std::string &get_name(void) { return hsm_name; }
    bool init(void);
    void cleanup(void);
    void dispatch(HSMEventBase::sp_t  evt);

    // must keep this list in sync with log_event_strings[]
    // initialization in _HSMBase::_HSMBase constructor.
    enum log_event_t {
        INFO_MASK  = 0x10,
        ERROR_MASK = 0x20,

        // initial state calling and logging entry to resulting state.
        INFO_INIT          = INFO_MASK | 0x01,

        // info log messages concerning dispatch of events into
        // states and the actions those states return.
        INFO_DISPATCH      = INFO_MASK | 0x02,

        // whenever a state is backtraced, this dumps the trace.
        INFO_TRACE         = INFO_MASK | 0x03,

        // logging the transition from one state heirarchy to another.
        INFO_TRANS         = INFO_MASK | 0x04,

        // more verbose logging about transition: individual states being
        // exited and entered as the state stack is popped and pushed.
        INFO_TRANS_VERB    = INFO_MASK | 0x04,

        // logging when an event makes it all the way to TOP and still
        // hasn't been handled by anything.
        INFO_UNHANDLED_EVT = INFO_MASK | 0x05,

        // an HSM is being shut down.
        INFO_EXIT          = INFO_MASK | 0x06,

        // an internal error in this library that is not supposed to
        // happen and indicates a possible bug in HSM itself.
        INTERNAL_ERROR    = ERROR_MASK | 0x01,

        // dispatch() was called without calling init()
        ERR_UNINIT        = ERROR_MASK | 0x02,

        // the user's initial state method must return TRANS.
        // this error indicates it returned something other than TRANS.
        ERR_INIT_TRANS    = ERROR_MASK | 0x03,

        // when an EntryEvent is dispatched to a state function,
        // the only legal response is HSM_HANDLED().
        ERR_ENTRY_HANDLED = ERROR_MASK | 0x04,

        // when an ExitEvent is dispatched to a state function,
        // the only legal response is HSM_HANDLED().
        ERR_EXIT_HANDLED  = ERROR_MASK | 0x05,

        // all state functions must conform to the StateFuncBase_t
        // function signature. if it doesn't, that's illegal.
        ERR_INVALID_SIG   = ERROR_MASK | 0x06,
    };
private:
    typedef std::unordered_map<log_event_t, const char *> log_event_names_t;
    log_event_names_t log_event_names;
public:
    const char * get_log_event_name(log_event_t) const;
    // the user's HSM class may choose to override this.
    virtual void log_event(log_event_t type,
                           const char * file, int line,
                           const char * function,
                           const char * format, ... );
};

template <class T> class HSM : public _HSMBase {
public:
    typedef Action (T::*StateFunc_t)(HSMEventBase::sp_t  evt);

// we want all the user's state methods to be nonstatic methods of the
// user's HSM-derived class. we want to enforce at compile time the
// type-checking of these method pointers during HSM_SUPER and
// HSM_TRANS macro invocations. but to do that, we need a template so
// we can have (T::*funcptr)() constructs; this would end up forcing
// the entire implementation of HSM into inline code in this template
// class, and we don't want that. so the template derives from a base
// class which does not know the user's derived types.
//
// this then forces us to need to cast the StateFunc_t to
// StateFuncBase_t. we could do that in the HSM_SUPER/HSM_TRANS macro
// definitions; but if we did it in the macro, it would be a type
// coersion inline in the user's code, therefore no real checking
// would be done. but if we do the cast here, inside the method
// declaration (the constructors below which chain to the internal
// base class constructors), then we get full type checking in the
// user's code, and the cast only happens when we know it's good.
//
// that's why HANDLED and TOP can be implemented fully in the base
// class but SUPER and TRANS have to be split between the base class
// and this template, where we can do enforcement of (T::*funcptr)()
// but safe casts.
//
// one other nice thing the macro gives us is the ability to use
// tricky things like PRETTY_FUNCTION so the user doesn't have to
// retype the method name.

    struct _SUPER final : _INTERNAL_SUPER {
        _SUPER(const char *_statename, StateFunc_t _super)
            : _INTERNAL_SUPER(_statename, (StateFuncBase_t) _super) { }
    };
    struct _TRANS final : _INTERNAL_TRANS
    {
        _TRANS(const char *_statename, StateFunc_t _newstate)
            : _INTERNAL_TRANS(_statename, (StateFuncBase_t) _newstate) { }
    };

// provide macros which fill out the PRETTY_FUNCTION for the user.

#define HSM_HANDLED()       _HANDLED(__PRETTY_FUNCTION__)
#define HSM_TOP()               _TOP(__PRETTY_FUNCTION__)
#define HSM_SUPER(super)      _SUPER(__PRETTY_FUNCTION__, super)
#define HSM_TRANS(newstate)   _TRANS(__PRETTY_FUNCTION__, newstate)

protected:
    void set_name(const std::string &name) { hsm_name = name; }

public:
    HSM(StateFunc_t _initial_func, bool _debug = false)
        : _HSMBase((StateFuncBase_t) _initial_func, _debug)
    {
        hsm_name = "<HSM NAME UNSET>";
    }
    virtual ~HSM(void)
    {
    }
};
