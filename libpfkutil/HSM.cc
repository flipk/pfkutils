
/** \file HSM.cc */

#include "HSM.h"

using namespace HSM;

const std::string HSMError::errStrings [__NUMERRS] = {
    "user states should not handle HSM_PROBE",
    "bogus action type value",
    "initial function must return TRANS",
    "state entry handler must return HANDLED",
    "state exit handler must return HANDLED"
};

//virtual
const std::string
HSMError::_Format(void) const
{
    std::string ret = "HSM ERROR: ";
    ret += errStrings[type];
    ret += " at:\n";
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
    WaitUtil::Lock lock(&active_hsms);
    active_hsms.add_tail(sm);
}

void
HSMScheduler::deregisterHSM(ActiveHSMBase *sm)
{
    WaitUtil::Lock lock(&active_hsms);
    active_hsms.remove(sm);
}

void
HSMScheduler::subscribe(ActiveHSMBase *sm, HSMEvent::Type type)
{
    WaitUtil::Lock lock(&subHash);
    HSMSubEntry * se;
    se = subHash.find(type);
    if (se == NULL)
    {
        se = new HSMSubEntry(type);
        subHash.add(se);
    }
    se->activeObjs.push_back(sm);
}

void
HSMScheduler::publish(HSMEvent * evt)
{
    WaitUtil::Lock lock(&subHash);
    HSMSubEntry * se = subHash.find((HSMEvent::Type)evt->type);
    if (se == NULL)
    {
        lock.unlock();
        std::cout << "no subscribers to event "
                  << evt->evtName()
                  << ", discarding\n";
        evt->ref();
        evt->deref();
    }
    else
    {
        for (size_t ind = 0; ind < se->activeObjs.size(); ind++)
        {
            se->activeObjs[ind]->enqueue(evt);
        }
    }
}

void
HSMScheduler::start(void)
{
    {
        WaitUtil::Lock  lock(&active_hsms);
        for (ActiveHSMBase * h = active_hsms.get_head();
             h != NULL;
             h = active_hsms.get_next(h))
        {
            h->init();
        }
    }
    HSMThread::Threads::start();
}

void
HSMScheduler::stop(void)
{
    HSMThread::Threads::cleanup();
}

HSMEventEnvelope *
HSMScheduler::allocEnv(void)
{
    return eventEnvelopePool.alloc(0,true);
}

void
HSMScheduler::releaseEnv(HSMEventEnvelope *env)
{
    eventEnvelopePool.release(env);
}

ActiveHSMBase::ActiveHSMBase(HSMScheduler * _sched,
                             const std::string &name)
    : Thread(name),
      sched(_sched),
      termEvent(HSMEvent::HSM_TERMINATE)
{
    sched->registerHSM(this);
}

//virtual
ActiveHSMBase::~ActiveHSMBase(void)
{
    sched->deregisterHSM(this);
}

void
ActiveHSMBase::entry(void)
{
    bool done = false;
    while (!done)
    {
        HSMEventEnvelope * env = eventQueue.dequeue(-1);
        AHSMdispatch(env->evt);
        if (env->evt->type == HSMEvent::HSM_TERMINATE)
            done = true;
        env->evt->deref();
        sched->releaseEnv(env);
    }
}

void
ActiveHSMBase::stopReq(void)
{
    // the termEvent doesn't come from a pool,
    // so we don't want deref trying to free it to
    // a nonexistent pool. give it an extra ref
    // so that it survives.
    termEvent.ref();
    enqueue(&termEvent);
}

void
ActiveHSMBase::enqueue(HSMEvent * evt)
{
    HSMEventEnvelope * env = sched->allocEnv();
    env->evt = evt;
    evt->ref();
    eventQueue.enqueue(env);
}
