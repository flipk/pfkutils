
#include "hsm.h"
#include <stdarg.h>

#define LOG_EVT(evttype, fmt...) \
    log_event(evttype, \
              __FILE__, __LINE__, \
              __PRETTY_FUNCTION__, fmt)

const char * HsmFuncRegex :: patt = HSM_FUNC_REGEX_PATTERN;

_HSMBase :: _HSMBase(StateFuncBase_t _initial_func,
                     bool _debug /*= false*/ )
    : currentState(_initial_func), debug(_debug),
      entryexitpool(4,1)
{
    if (func_regex.ok() == false)
    {
        LOG_EVT(INTERNAL_ERROR,
                "ERROR: HsmFuncRegex failed to init! (%s)",
                func_regex.err());
    }

    log_event_names[INFO_INIT]          = "INIT";
    log_event_names[INFO_DISPATCH]      = "DISPATCH";
    log_event_names[INFO_TRACE]         = "TRACE";
    log_event_names[INFO_TRANS]         = "TRANS";
    log_event_names[INFO_TRANS_VERB]    = "TRANS-VERB";
    log_event_names[INFO_UNHANDLED_EVT] = "UNHANDLED-EVT";
    log_event_names[INFO_EXIT]          = "EXIT";
    log_event_names[INTERNAL_ERROR]     = "INTERNAL";
    log_event_names[ERR_UNINIT]         = "UNINITIALIZED";
    log_event_names[ERR_INIT_TRANS]     = "INIT-TRANS";
    log_event_names[ERR_ENTRY_HANDLED]  = "ENTRY-NOT-HANDLED";
    log_event_names[ERR_EXIT_HANDLED]   = "EXIT-NOT-HANDLED";
    log_event_names[ERR_INVALID_SIG]    = "INVALID-FUNC-SIG";

    entryexitpool.alloc(&trace_evt,      t2t2::GROW);
    entryexitpool.alloc(&entry_evt,      t2t2::GROW);
    entryexitpool.alloc(&exit_evt,       t2t2::GROW);

    currentTrace = &trace1;
    oldTrace = &trace2;

    initialized = false;
}

const char *
_HSMBase :: get_log_event_name(log_event_t t) const
{
    log_event_names_t::const_iterator it = log_event_names.find(t);
    if (it != log_event_names.end())
        return it->second;
    return "unknown";
}

_HSMBase :: ~_HSMBase(void)
{
    if (initialized)
    {
        fprintf(stderr, ">>>\n>>>\n"
                ">>>  HSM ERROR: HSM '%s' was not "
                "properly cleaned up prior to destruction!\n"
                ">>>\n>>>\n", hsm_name.c_str());
        cleanup();
    }
}

void
_HSMBase :: cleanup(void)
{
    if (debug)
        LOG_EVT(INFO_EXIT, "shutdown: HSM terminating");
    for (int ind = currentTrace->size()-1; ind >= 0; ind--)
    {
        if (debug)
            LOG_EVT(INFO_EXIT, "shutdown: exiting state '%s'",
                    short_statename((*currentTrace)[ind].statename));

        Action a = (this->*((*currentTrace)[ind].state))(exit_evt);
        if (a.type != Action::HANDLED)
        {
            LOG_EVT(ERR_EXIT_HANDLED,
                    "state '%s' ExitEvent should return HANDLED",
                    short_statename((*currentTrace)[ind].statename));
        }
    }

    initialized = false;
}

// build a cache of full statename to short statename translation.
// the cache is an unordered_hash indexed by the pointer value of
// the statename.
//
// ASSUMPTION: the statename pointer is a constant, which is true if
//             the user is using the HSM_* action macros, which insert
//             PRETTY_FUNCTION string constants.
//
// ASSUMPTION: an unordered_hash lookup is faster than a regex exec.
//
// NOTE: the only reason we use a regex to extract the function name
//       is that i really don't want to place a burden on the
//       programmer of typing the function name into the TRANS and
//       SUPER returns; i'd rather let CPP insert PRETTY_FUNNCTION in
//       the HSM_* macros at compile time so it is never wrong.
//
const char * _HSMBase :: short_statename(const char *statename)
{
    shortname_cache_t::iterator it;
    uintptr_t key = (uintptr_t) statename;
    int pos, len;

    it = shortname_cache.find(key);
    if (it != shortname_cache.end())
        return it->second.c_str();

    if (!func_regex.exec(statename))
    {
        LOG_EVT(INTERNAL_ERROR, "failed to exec regex: %s",
                func_regex.err());
        return statename;
    }
    const char * starting_char = NULL;
    if (func_regex.match(HsmFuncRegex::IND_FUNCNAME, &pos, &len))
        starting_char = statename + pos;
    else if (func_regex.match(HsmFuncRegex::IND_WRONG_FUNCNAME, &pos, &len))
    {
        LOG_EVT(ERR_INVALID_SIG,
                "state function '%s' does not follow the signature "
                "of HSM<T>::StateFunc_t!\n", statename);
        starting_char = statename + pos;
    }
    if (starting_char)
    {
        std::string &shortname = shortname_cache[key];
        shortname.resize(len);
        memcpy((void*) shortname.c_str(), starting_char, len);
        return shortname.c_str();
    }
    return statename;
}

const char * _HSMBase :: statename(StateFuncBase_t state,
                                   StateFuncBase_t *nextState,
                                   bool *top)
{
    if (state == NULL)
    {
        LOG_EVT(INTERNAL_ERROR, "why is state NULL here?");
        if (nextState)
            *nextState = NULL;
        if (top)
            *top = true;
        return "<STATE NAME UNSET>";
    }
    if (top) *top = false;
    Action a = (this->*state)(trace_evt);
    switch (a.type)
    {
    case Action::TOP:
        if (top)
            *top = true;
        //FALLTHRU
    case Action::SUPER:
    case Action::TRANS:
        if (nextState)
            *nextState = a.state;
        //FALLTHRU
    case Action::HANDLED:
        return a.statename;
    default:;
    }
    return "<invalid Action type>";
}

void _HSMBase :: backtrace(StateTrace_t *traceret, StateFuncBase_t state)
{
    temp_trace.resize(0);
    bool done = false;
    do {
        StateFuncBase_t  nextState = NULL;
        const char * name = statename(state, &nextState, &done);
        temp_trace.push_back(TraceEntry(state, name));
        state = nextState;
    } while (done == false);
    traceret->clear();
    // reverse order of trace
    for (int ind = temp_trace.size() - 1; ind >= 0; ind--)
        traceret->push_back(temp_trace[ind]);

    if (debug)
    {
        std::ostringstream str;
        str << "trace of state "
            << trace2str(traceret).c_str();
        for (int ind = 0; ind < traceret->size(); ind++)
            str << "\n  [" << ind << "]: "
                << (*traceret)[ind].statename;
        LOG_EVT(INFO_TRACE, "%s", str.str().c_str());
    }
}

std::string _HSMBase :: trace2str(StateTrace_t *trace)
{
    std::ostringstream ostr;
    for (int ind = 0; ind < trace->size(); ind++)
    {
        const char * short_name = short_statename((*trace)[ind].statename);
        if (ind != 0)
            ostr << ".";
        ostr << short_name;
    }
    return ostr.str();
}

bool _HSMBase :: init(void)
{
    if (debug)
        LOG_EVT(INFO_INIT, "entering initial state");

    Action a = (this->*currentState)(entry_evt);

    if (a.type == Action::TRANS)
    {
        currentState = a.state;
        backtrace(currentTrace, currentState);

        // enter all the states in the stack.
        if (debug)
            LOG_EVT(INFO_INIT, "entering initial state '%s'",
                    trace2str(currentTrace).c_str());
        for (int ind = 0; ind < currentTrace->size(); ind++)
        {
            TraceEntry * te = &((*currentTrace)[ind]);
            Action a = (this->*(te->state))(entry_evt);
            if (a.type != Action::HANDLED)
                LOG_EVT(ERR_ENTRY_HANDLED,
                        "state '%s' EnterEvent should return HANDLED",
                        short_statename((*currentTrace)[ind].statename));
        }
    }
    else
    {
        LOG_EVT(ERR_INIT_TRANS, "initial method should always return TRANS");
        return false;
    }

    initialized = true;
    return true;
}

void _HSMBase :: dispatch(HSMEventBase::sp_t  evt)
{
    if (!initialized)
    {
        LOG_EVT(ERR_UNINIT, "init() method was never called");
        return;
    }

    if (debug)
        LOG_EVT(INFO_DISPATCH, "dispatching '%s' to state '%s'",
                evt->format_str().c_str(),
                trace2str(currentTrace).c_str());

    bool done = false;
    StateFuncBase_t newState = NULL;
    StateFuncBase_t state = currentState;
    do {
        done = true;

        Action a = (this->*state)(evt);

        if (debug)
            LOG_EVT(INFO_DISPATCH, "state '%s' returned action '%s'",
                    short_statename(a.statename), a.action_name());

        switch (a.type)
        {
        case Action::HANDLED:
            // do nothing.
            break;

        case Action::SUPER:
            state = a.state;
            done = false;
            break;

        case Action::TRANS:
            newState = a.state;
            break;

        case Action::TOP:
            if (debug)
                LOG_EVT(INFO_UNHANDLED_EVT, "state '%s' unhandled event '%s'",
                        trace2str(currentTrace).c_str(),
                        evt->format_str().c_str());
            break;
        }

    } while (done == false);

    if (newState != NULL && newState != currentState)
    {
        int ind;
        StateTrace_t * newTrace = oldTrace;
        backtrace(newTrace, newState);
        if (debug)
            LOG_EVT(INFO_TRANS, "transition from %s to %s",
                    trace2str(currentTrace).c_str(),
                    trace2str(newTrace).c_str());
        for (ind = currentTrace->size()-1; ind >= 0; ind--)
        {
            if (ind < newTrace->size() &&
                (*currentTrace)[ind].state == (*newTrace)[ind].state)
                break;
            if (debug)
                LOG_EVT(INFO_TRANS_VERB, "exiting state '%s'",
                        short_statename((*currentTrace)[ind].statename));
            Action a = (this->*((*currentTrace)[ind].state))(exit_evt);
            if (a.type != Action::HANDLED)
                LOG_EVT(ERR_EXIT_HANDLED,
                        "state '%s' ExitEvent should return HANDLED",
                        short_statename((*currentTrace)[ind].statename));
        }
        ind++;
        for (; ind < newTrace->size(); ind++)
        {
            if (debug)
                LOG_EVT(INFO_TRANS_VERB, "entering state '%s'",
                        short_statename((*newTrace)[ind].statename));
            Action a = (this->*((*newTrace)[ind].state))(entry_evt);
            if (a.type != Action::HANDLED)
                LOG_EVT(ERR_ENTRY_HANDLED,
                        "state '%s' EnterEvent should return HANDLED",
                        short_statename((*newTrace)[ind].statename));
        }

        currentState = newState;
        oldTrace = currentTrace;
        currentTrace = newTrace;
    }
}

void _HSMBase :: log_event(log_event_t type,
                           const char * file, int line,
                           const char * function,
                           const char * format, ... )
{
    fprintf(stderr, ">>> %s-%s: ",
            (type & INFO_MASK)  ? "INFO" :
            (type & ERROR_MASK) ? "ERROR" : "UNKNOWN",
            get_log_event_name(type));
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}
