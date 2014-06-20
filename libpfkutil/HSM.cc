
#include "HSM.h"

using namespace PFKHSM;

const std::string HSMError::errStrings [__NUMERRS] = {
    "user states should not handle HSM_PROBE",
    "bogus action type value",
    "initial function must return TRANS",
    "state entry handler must return HANDLED",
    "state exit handler must return HANDLED"
};

const std::string
HSMError::Format(void) const
{
    std::string ret = "HSM ERROR: ";
    ret += errStrings[type];
    ret += " at:\n";
    ret += BackTraceFormat();
    return ret;
}

HSMScheduler::HSMScheduler(void)
{
}

HSMScheduler::~HSMScheduler(void)
{
}

void
HSMScheduler::registerHSM(ActiveHSMBase *sm)
{
    PFK::Lock lock(&active_hsms);
    active_hsms.add_tail(sm);
}

void
HSMScheduler::deregisterHSM(ActiveHSMBase *sm)
{
    PFK::Lock lock(&active_hsms);
    active_hsms.remove(sm);
}

void
HSMScheduler::subscribe(ActiveHSMBase *sm, int type)
{
}

void
HSMScheduler::publish(HSMEvent const * evt)
{
}

ActiveHSMBase::ActiveHSMBase(HSMScheduler * _sched)
    : sched(_sched)
{
    sched->registerHSM(this);
}

//virtual
ActiveHSMBase::~ActiveHSMBase(void)
{
    sched->deregisterHSM(this);
}
