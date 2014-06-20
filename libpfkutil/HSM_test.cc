#if 0
set -e -x
g++ -Wall -Werror -g3 -c HSM_test.cc
g++ -g3 HSM_test.o -o HSM_test
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

class myStateMachine : public HSM<myStateMachine>
{
public:
    myStateMachine(bool __debug = false)
        : HSM<myStateMachine>(__debug)
    {
    }
    virtual ~myStateMachine(void)
    {
    }
    /*virtual*/ Action initial(void)
    {
        return TRANS(&myStateMachine::unconfigured);
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
            return TRANS(&myStateMachine::init);
        }
        return SUPER(&myStateMachine::top, "unconfigured");
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
            return TRANS(&myStateMachine::connected);
        case DISCON:
            printf("got DISCON in top.configured.auth\n");
            return TRANS(&myStateMachine::unconfigured);
        }
        return SUPER(&myStateMachine::top, "configured");
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
        return SUPER(&myStateMachine::configured, "init");
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
            return TRANS(&myStateMachine::auth);
        }
        return SUPER(&myStateMachine::configured, "connected");
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
        return SUPER(&myStateMachine::configured, "auth");
    }
};

}; // namespace MyTestApp

int
main()
{
    MyTestApp::myStateMachine  myHsm(true);

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

    return 0;
}
