#if 0
set -e -x
g++ -Wall -Werror -g3 -c HSM.cc
g++ -Wall -Werror -g3 -c dll3.cc
g++ -Wall -Werror -g3 -c HSM_test.cc
g++ -g3 HSM_test.o HSM.o dll3.o -rdynamic -o HSM_test -lpthread
exit 0
;
#endif

#include "HSM.h"

using namespace PFKHSM;

namespace MyTestApp {

enum MyHSMEVENTTYPE {
    CONFIG = HSM_USER_START,
    CONNECT,
    DISCON,
    AUTH,
    DUMMY
};

struct myConfigEvt : HSMEvent
{
    myConfigEvt(void) : HSMEvent(CONFIG) { }
    ~myConfigEvt(void) { }
    const std::string evtName(void) const { return "myConfigEvt"; }
};
struct myConnectEvt : HSMEvent
{
    myConnectEvt(void) : HSMEvent(CONNECT) { }
    ~myConnectEvt(void) { }
    const std::string evtName(void) const { return "myConnectEvt"; }
};
struct myDisconEvt : HSMEvent
{
    myDisconEvt(void) : HSMEvent(DISCON) { }
    ~myDisconEvt(void) { }
    const std::string evtName(void) const { return "myDisconEvt"; }
};
struct myAuthEvt : HSMEvent
{
    myAuthEvt(void) : HSMEvent(AUTH) { }
    ~myAuthEvt(void) { }
    const std::string evtName(void) const { return "myAuthEvt"; }
};
struct myDummyEvt : HSMEvent
{
    myDummyEvt(void) : HSMEvent(DUMMY) { }
    ~myDummyEvt(void) { }
    const std::string evtName(void) const { return "myDummyEvt"; }
};

class myStateMachine1 : public ActiveHSM<myStateMachine1>
{
public:
    myStateMachine1(HSMScheduler * _sched, bool __debug = false)
        : ActiveHSM<myStateMachine1>(_sched, __debug)
    {
    }
    virtual ~myStateMachine1(void)
    {
    }
    /*virtual*/ Action initial(void)
    {
        return TRANS(&myStateMachine1::unconfigured);
    }
    Action top(HSMEvent const * ev)
    {
        switch (ev->type)
        {
        case HSM_ENTRY:
            printf("ENTER: top\n");
            return HANDLED();
        case HSM_EXIT:
            printf("EXIT: top\n");
            return HANDLED();
        }
        return TOP();
    }
    Action unconfigured(HSMEvent const * ev)
    {
        switch (ev->type)
        {
        case HSM_ENTRY:
            printf("ENTER: top.unconfigured\n");
            return HANDLED();
        case HSM_EXIT:
            printf("EXIT: top.unconfigured\n");
            return HANDLED();
        case CONFIG:
            printf("got CONFIG in top.unconfigured\n");
            return TRANS(&myStateMachine1::init);
        }
        return SUPER(&myStateMachine1::top, "unconfigured");
    }
    Action configured(HSMEvent const * ev)
    {
        switch (ev->type)
        {
        case HSM_ENTRY:
            printf("ENTER: top.configured\n");
            return HANDLED();
        case HSM_EXIT:
            printf("EXIT: top.configured\n");
            return HANDLED();
        case CONNECT:
            printf("got CONNECT in top.configured.init\n");
            return TRANS(&myStateMachine1::connected);
        case DISCON:
            printf("got DISCON in top.configured.auth\n");
            return TRANS(&myStateMachine1::unconfigured);
        }
        return SUPER(&myStateMachine1::top, "configured");
    }
    Action init(HSMEvent const * ev)
    {
        switch (ev->type)
        {
        case HSM_ENTRY:
            printf("ENTER: top.configured.init\n");
            return HANDLED();
        case HSM_EXIT:
            printf("EXIT: top.configured.init\n");
            return HANDLED();
        }
        return SUPER(&myStateMachine1::configured, "init");
    }
    Action connected(HSMEvent const * ev)
    {
        switch (ev->type)
        {
        case HSM_ENTRY:
            printf("ENTER: top.configured.connected\n");
            return HANDLED();
        case HSM_EXIT:
            printf("EXIT: top.configured.connected\n");
            return HANDLED();
        case AUTH:
            printf("got AUTH in top.configured.connected\n");
            return TRANS(&myStateMachine1::auth);
        }
        return SUPER(&myStateMachine1::configured, "connected");
    }
    Action auth(HSMEvent const * ev)
    {
        switch (ev->type)
        {
        case HSM_ENTRY:
            printf("ENTER: top.configured.auth\n");
            return HANDLED();
        case HSM_EXIT:
            printf("EXIT: top.configured.auth\n");
            return HANDLED();
        }
        return SUPER(&myStateMachine1::configured, "auth");
    }
};

}; // namespace MyTestApp

int
main()
{
    try {
        PFKHSM::HSMScheduler  sched;

        MyTestApp::myStateMachine1  myHsm(&sched, true);

        myHsm.HSMInit();
        MyTestApp::myConfigEvt config;
        myHsm.dispatch(&config);
        MyTestApp::myConnectEvt connect;
        myHsm.dispatch(&connect);
        MyTestApp::myAuthEvt auth;
        myHsm.dispatch(&auth);
        MyTestApp::myDisconEvt discon;
        myHsm.dispatch(&discon);
        MyTestApp::myDummyEvt dummy;
        myHsm.dispatch(&dummy);
    }
    catch (PFKHSM::HSMError  err)
    {
        std::cout << "caught HSM error:\n"
                  << err.Format();
    }
    catch (DLL3::ListError le)
    {
        std::cout << "caught DLL3 error:\n"
                  << le.Format();
    }

    return 0;
}
