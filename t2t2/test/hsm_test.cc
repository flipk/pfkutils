
#include "hsm.h"
#include <stdarg.h>

struct UserHSMEventType : HSMEventType {
    enum { MYEVENT1 = USER_START, MYEVENT2 };
};

class MyEvent1 : public HSMEventBase
{
public:
    // convenience
    typedef pxfe_shared_ptr<MyEvent1> sp_t;
    static const int TYPE = UserHSMEventType::MYEVENT1;
    // data here
    int v;
    MyEvent1(int _v) : HSMEventBase(TYPE, "MyEvent1"), v(_v)
    {
        std::cout << "--> " << this << " construction\n";
    }
    virtual ~MyEvent1(void)
    {
        std::cout << "--> " << this << " destruction\n";
    }
    virtual void format(std::ostream &str) const
    {
        str << "MyEvent1 (v=" << v << ")";
    }
};

class MyEvent2 : public HSMEventBase
{
public:
    // convenience
    typedef pxfe_shared_ptr<MyEvent2> sp_t;
    static const int TYPE = UserHSMEventType::MYEVENT2;
    // data here
    int q;
    int t;
    MyEvent2(int _q, int _t) : HSMEventBase(TYPE, "MyEvent2"), q(_q), t(_t)
    {
        std::cout << "--> " << this << " construction\n";
    }
    virtual ~MyEvent2(void)
    {
        std::cout << "--> " << this << " destruction\n";
    }
    virtual void format(std::ostream &str) const
    {
        str << "MyEvent2 (q=" << q << " t=" << t << ")";
    }
};

typedef t2t2::t2t2_pool<HSMEventBase,
                        MyEvent1,
                        MyEvent2> mypool_t;

class MyHsm : public HSM<MyHsm>
{
    Action initial(HSMEventBase::sp_t evt)
    {
        switch (evt->event_type.val)
        {
        case EntryEvent::TYPE:
            printf("--> entry event to initial: trans to top\n");
            // the return from initial() state should always be a TRANS
            // for the new TOP state. warnings will be thrown if not.
            return HSM_TRANS(&MyHsm::top);
        }
        return HSM_TOP();
    }
    Action top(HSMEventBase::sp_t evt)
    {
        switch (evt->event_type.val)
        {
        case EntryEvent::TYPE:
            printf("--> entry event to top state\n");
            return HSM_HANDLED();

        case ExitEvent::TYPE:
            printf("--> exit event to top state\n");
            return HSM_HANDLED();

        case MyEvent1::TYPE:
            printf("--> e1 to top: going to state1\n");
            return HSM_TRANS(&MyHsm::state1);

        case MyEvent2::TYPE:
            printf("--> e2 to top: going to top or staying in top\n");
            return HSM_TRANS(&MyHsm::top);
        }
        return HSM_TOP();
    }
    Action state1(HSMEventBase::sp_t evt)
    {
        switch (evt->event_type.val)
        {
        case EntryEvent::TYPE:
            printf("--> entry event to state1\n");
            return HSM_HANDLED();

        case ExitEvent::TYPE:
            printf("--> exit event to state1\n");
            return HSM_HANDLED();

        // override top's handling of e1
        case MyEvent1::TYPE:
            printf("--> e1 to state1: going to state2\n");
            return HSM_TRANS(&MyHsm::state2);

        // e2 falls thru to top
        }

        return HSM_SUPER(&MyHsm::top);
    }
    Action state2(HSMEventBase::sp_t evt)
    {
        switch (evt->event_type.val)
        {
        case EntryEvent::TYPE:
            printf("--> entry event to state2\n");
            return HSM_HANDLED();

        case ExitEvent::TYPE:
            printf("--> exit event to state2\n");
            return HSM_HANDLED();

        // override top's and state1's handling of e1
        case MyEvent1::TYPE:
            printf("--> e1 to state2: going to state1\n");
            return HSM_TRANS(&MyHsm::state1);

        // e2 falls thru to top
        }

        return HSM_SUPER(&MyHsm::state1);
    }
public:
    MyHsm(bool _debug = false)
        : HSM(&MyHsm::initial, _debug)
    {
        set_name("MyHsm");
    }
    virtual ~MyHsm(void)
    {
    }

    // override base class method
    /*virtual*/ void log_event(log_event_t type,
                               const char * file, int line,
                               const char * function,
                               const char * format, ... )
    {
        fprintf(stderr, "%s> %s-%s: ",
                hsm_name.c_str(),
                (type & INFO_MASK)  ? "INFO" :
                (type & ERROR_MASK) ? "ERROR" : "UNKNOWN",
                get_log_event_name(type));
        va_list ap;
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
        fprintf(stderr, "\n");
    }
};

int main()
{
    mypool_t   pool( 2, 0);
    MyHsm      hsm(true);

    if (!hsm.init())
    {
        printf("HSM init failed\n");
        return 1;
    }

    MyEvent1::sp_t   e1;
    if (pool.alloc(&e1, t2t2::NO_WAIT, 7))
    {
        printf("--> dispatching e1\n");
        hsm.dispatch(e1);

        printf("--> dispatching e1\n");
        hsm.dispatch(e1);
    }
    else
        printf("failed to get e1!\n");

    MyEvent2::sp_t   e2;
    if (pool.alloc(&e2, t2t2::NO_WAIT, 8, 9))
    {
        printf("--> dispatching e2\n");
        hsm.dispatch(e2);

        printf("--> dispatching e1\n");
        hsm.dispatch(e1);
    }
    else
        printf("failed to get e2!\n");

    hsm.cleanup();

    return 0;
}
