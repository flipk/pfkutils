
#define __STDC_FORMAT_MACROS 1

#include "signal_backtrace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <stdarg.h>
#include <pthread.h>
#include <inttypes.h>

SignalBacktrace* SignalBacktrace::instance = NULL;

SignalBacktrace :: SignalBacktrace(void)
{
    struct sigaction sa;

    sa.sa_handler = SIG_IGN;
    sigfillset( &sa.sa_mask );
    sa.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sa, NULL);

    sa.sa_sigaction = &signal_handler;
    sa.sa_flags = SA_SIGINFO;
    if (getenv("ALLOW_CORE_DUMPS") == NULL)
    {
        sigaction(SIGSEGV, &sa, NULL);
        sigaction(SIGILL,  &sa, NULL);
        sigaction(SIGBUS,  &sa, NULL);
        sigaction(SIGFPE,  &sa, NULL);
        sigaction(SIGTRAP, &sa, NULL);
        sigaction(SIGABRT, &sa, NULL);
    }

    // restartable and not fatal.
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    pid = getpid();

    handler = &default_user_handler;
    instance = this;
}

SignalBacktrace :: ~SignalBacktrace(void)
{
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGILL,  &sa, NULL);
    sigaction(SIGBUS,  &sa, NULL);
    sigaction(SIGFPE,  &sa, NULL);
    sigaction(SIGTRAP, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);

    instance = NULL;
}

SignalBacktrace *
SignalBacktrace :: get_instance(void)
{
    if (instance == NULL)
        instance = new SignalBacktrace;
    return instance;
}

void
SignalBacktrace :: cleanup(void)
{
    if (instance != NULL)
        delete instance;
    instance = NULL;
}

SignalBacktraceInfo :: SignalBacktraceInfo(void)
{
    // clear will try to free these if they
    // are not null.
    messages = NULL;
#ifdef __arm__
    messages2 = NULL;
#endif
    clear();
}

SignalBacktraceInfo :: ~SignalBacktraceInfo(void)
{
    if (messages)
        free(messages);
#ifdef __arm__
    if (messages2)
        free(messages2);
#endif
}

void
SignalBacktraceInfo :: clear(void)
{
    fatal = true;
    description[0] = 0;
    desc_len = 0;
    sig = 0;
    memset(&siginfo_data, 0, sizeof(siginfo_data));
    uc = NULL;
    trace_size = 0;
    memset(trace,0,sizeof(trace));
    if (messages)
        free(messages);
    messages = NULL;
#ifdef __arm__
    trace_size2 = 0;
    memset(trace2,0,sizeof(trace2));
    if (messages2)
        free(messages2);
    messages2 = NULL;
#endif
}

void
SignalBacktraceInfo :: desc_print(const char *format...)
{
    va_list ap;
    va_start(ap, format);
    if (desc_len < (SignalBacktrace_MAX_DESCRIPTION - 1)) {
        int cc = vsnprintf(description + desc_len,
                           SignalBacktrace_MAX_DESCRIPTION - desc_len - 1,
                           format, ap);
        desc_len += cc;
        if (desc_len > (SignalBacktrace_MAX_DESCRIPTION - 1))
            desc_len = (SignalBacktrace_MAX_DESCRIPTION - 1);
        va_end(ap);
        description[desc_len] = 0; // always null terminated
    }
}

void
SignalBacktrace :: register_handler( const char *new_process_name,
                                     SignalBacktraceHandler new_handler )
{
    if (new_process_name != NULL)
    {
        strncpy(process_name, new_process_name, sizeof(process_name)-1);
        process_name[sizeof(process_name)-1] = 0;
    }
    else
        process_name[0] = 0;
    if (new_handler != NULL)
        handler = new_handler;
    else
        handler = &default_user_handler;
}

//static
void
SignalBacktrace :: default_user_handler(
    const struct SignalBacktraceInfo *info )
{
    ssize_t s = write(2, info->description, info->desc_len);
    if (s) { } // remove unused warning
    if (info->fatal)
        exit(1);
}

extern char _start;
extern char _etext;
extern char _fini;

//static
void
SignalBacktrace :: signal_handler(int sig, siginfo_t *info, void *uc)
{
    if (instance == NULL)
    {
        fprintf(stderr,
                "[bt] %u unhandled signal %d with no SignalBacktrace!\n",
                getpid(), sig);
        exit(1);
    }
    instance->info.clear();
    switch (sig)
    {
    case SIGHUP:
    case SIGTERM:
    case SIGINT:
    case SIGQUIT:
        instance->info.fatal = false;
        break;
    default:
        instance->info.fatal = true;
    }
    instance->info.sig = sig;
    instance->info.siginfo_data = *info;
    instance->info.uc = (ucontext_t *) uc;
    instance->do_backtrace();
}

void
SignalBacktrace :: backtrace_now( const char *reason )
{
    info.clear();
    info.fatal = false;
    info.desc_print("[bt] %s %u ---backtrace requested, reason: %s\n",
                    process_name, pid, reason);
    do_backtrace();
}

void
SignalBacktrace :: do_backtrace( void )
{
    int i;

    // first we'll try a good old honest backtrace try
    // and see what we get.
    info.trace_size = backtrace( info.trace,
                                 SignalBacktrace_MAX_TRACE_ENTRIES );
    info.messages = backtrace_symbols( info.trace, info.trace_size );

    if (info.sig != 0)
    {
        bool si_addr_valid = false;
        info.desc_print("[bt] %s %u ---got signal %d ",
                        process_name, pid, info.sig);

        switch (info.sig)
        {
        case SIGHUP:
            info.desc_print("SIGHUP");
            break;
        case SIGINT:
            info.desc_print("SIGINT -- did you hit ^C?");
            break;
        case SIGQUIT:
            info.desc_print("SIGQUIT -- did you hit ^\\?");
            break;
        case SIGTERM:
            info.desc_print("SIGTERM");
            break;

        case SIGABRT:
            info.desc_print("SIGABRT");
            break;

        case SIGILL:
            info.desc_print("SIGILL");
            si_addr_valid = true;
            break;
        case SIGFPE:
            info.desc_print("SIGFPE");
            si_addr_valid = true;
            break;
        case SIGTRAP:
            info.desc_print("SIGTRAP");
            si_addr_valid = true;
            break;
        case SIGSEGV:
            info.desc_print("SIGSEGV");
            si_addr_valid = true;
            break;
        case SIGBUS:
            info.desc_print("SIGBUS");
            si_addr_valid = true;
            break;
        }

        if (si_addr_valid)
            info.desc_print(" at si_addr = 0x%" PRIxPTR,
                   (uintptr_t) info.siginfo_data.si_addr);

        info.desc_print("\n");
    }

    info.desc_print("[bt] %s %u ---Execution path according to backtrace:\n",
                    process_name, pid);
    for ( i = 0; i < info.trace_size; i++ )
        info.desc_print( "[bt] %s %u %s\n",
                         process_name, pid, info.messages[ i ] );
    info.desc_print( "[bt] %s %u ---end\n", process_name, pid);

#ifdef __arm__
    do { // extra sauce to deal with arm's terrible backtraces
        info.trace_size2 = 0; // start over
        pthread_attr_t   attr;
        if (pthread_getattr_np(pthread_self(), &attr) != 0)
            // give up
            break;
        void *stackaddr;
        size_t stacksize;
        if (pthread_attr_getstack(&attr, &stackaddr, &stacksize) != 0)
            // give up
            break;
        uintptr_t  text_lo = (uintptr_t) &_start;
        uintptr_t  text_hi = (uintptr_t) &_fini;
        uintptr_t  stack_lo = (uintptr_t) stackaddr;
        uintptr_t  stack_hi = stack_lo + stacksize;
        uint32_t * ptr = (uint32_t *) &i;

        for (int ind = 0;
             info.trace_size2 < SignalBacktrace_MAX_TRACE_ENTRIES;
             ind++)
        {
            uintptr_t ptr_to_v = (uintptr_t) &ptr[ind];
            if (ptr_to_v >= stack_hi)
                break;
            uintptr_t v = (uintptr_t) ptr[ind];
            if (v > text_lo && v < text_hi)
                info.trace2[info.trace_size2++] = (void*) v;
        }
        // skip cuz we're about to die and i don't care
        // and i ain't taking any risks: pthread_attr_destroy(&attr);
        info.messages2 = backtrace_symbols( info.trace2, info.trace_size2 );
        info.desc_print( "[bt] %s %u ---all text values on stack "
                         "(NOTE SOME MAY BE STRAYS):\n", process_name, pid);
        for ( i = 0; i < info.trace_size2; i++ )
            info.desc_print( "[bt] %s %u %s\n",
                             process_name, pid, info.messages2[ i ] );
        info.desc_print( "[bt] %s %u ---end\n", process_name, pid);
    } while(0);
#endif

    if (info.uc != NULL)
    {
#if __powerpc__
        // register dump
        mcontext_t *regs = info.uc->uc_mcontext.uc_regs;
        info.desc_print("[bt] %s %u REGISTER DUMP:\n", process_name, pid);
        for (i=0; i < 32; i++)
        {
            info.desc_print("  r%-2d %08x  ", i, regs->gregs[i]);
            if ((i & 3) == 3)
                info.desc_print("\n");
        }
        info.desc_print("  nip %08x  ", regs->gregs[32]);
        info.desc_print("  msr %08x  ", regs->gregs[33]);
        info.desc_print("   r3 %08x  ", regs->gregs[34]);
        info.desc_print("  ctr %08x\n", regs->gregs[35]);
        info.desc_print("  lnk %08x  ", regs->gregs[36]);
        info.desc_print("  xer %08x  ", regs->gregs[37]);
        info.desc_print("  ccr %08x  ", regs->gregs[38]);
        info.desc_print("   mq %08x\n", regs->gregs[39]);
        info.desc_print("  trp %08x  ", regs->gregs[40]);
        info.desc_print("  dar %08x  ", regs->gregs[41]);
        info.desc_print("  dir %08x  ", regs->gregs[42]);
        info.desc_print("  res %08x\n", regs->gregs[43]);
#endif
#if __arm__
        mcontext_t *regs = &info.uc->uc_mcontext;
        info.desc_print("[bt] %s %u REGISTER DUMP:\n", process_name, pid);
        info.desc_print("  r0  %08x  ", regs->arm_r0);
        info.desc_print("  r1  %08x  ", regs->arm_r1);
        info.desc_print("  r2  %08x  ", regs->arm_r2);
        info.desc_print("  r3  %08x\n", regs->arm_r3);
        info.desc_print("  r4  %08x  ", regs->arm_r4);
        info.desc_print("  r5  %08x  ", regs->arm_r5);
        info.desc_print("  r6  %08x  ", regs->arm_r6);
        info.desc_print("  r7  %08x\n", regs->arm_r7);
        info.desc_print("  r8  %08x  ", regs->arm_r8);
        info.desc_print("  r9  %08x  ", regs->arm_r9);
        info.desc_print("  r10 %08x  ", regs->arm_r10);
        info.desc_print("  fp  %08x\n", regs->arm_fp);
        info.desc_print("  ip  %08x  ", regs->arm_ip);
        info.desc_print("  sp  %08x  ", regs->arm_sp);
        info.desc_print("  lr  %08x  ", regs->arm_lr);
        info.desc_print(" cpsr %08x\n", regs->arm_cpsr);
// in si_addr already // info.desc_print("  FA  %08x\n", regs->fault_address);
#endif
#ifdef __x86_64__
        mcontext_t *regs = &info.uc->uc_mcontext;
        info.desc_print("[bt] %s %u REGISTER DUMP:\n", process_name, pid);
        info.desc_print("  r8 %016" PRIx64 "  ", regs->gregs[REG_R8     ]);
        info.desc_print("  r9 %016" PRIx64 "  ", regs->gregs[REG_R9     ]);
        info.desc_print(" r10 %016" PRIx64 "  ", regs->gregs[REG_R10    ]);
        info.desc_print(" r11 %016" PRIx64 "\n", regs->gregs[REG_R11    ]);
        info.desc_print(" r12 %016" PRIx64 "  ", regs->gregs[REG_R12    ]);
        info.desc_print(" r13 %016" PRIx64 "  ", regs->gregs[REG_R13    ]);
        info.desc_print(" r14 %016" PRIx64 "  ", regs->gregs[REG_R14    ]);
        info.desc_print(" r15 %016" PRIx64 "\n", regs->gregs[REG_R15    ]);
        info.desc_print(" rdi %016" PRIx64 "  ", regs->gregs[REG_RDI    ]);
        info.desc_print(" rsi %016" PRIx64 "  ", regs->gregs[REG_RSI    ]);
        info.desc_print(" rbp %016" PRIx64 "  ", regs->gregs[REG_RBP    ]);
        info.desc_print(" rbx %016" PRIx64 "\n", regs->gregs[REG_RBX    ]);
        info.desc_print(" rdx %016" PRIx64 "  ", regs->gregs[REG_RDX    ]);
        info.desc_print(" rax %016" PRIx64 "  ", regs->gregs[REG_RAX    ]);
        info.desc_print(" rcx %016" PRIx64 "  ", regs->gregs[REG_RCX    ]);
        info.desc_print(" rsp %016" PRIx64 "\n", regs->gregs[REG_RSP    ]);
        info.desc_print(" rip %016" PRIx64 "  ", regs->gregs[REG_RIP    ]);
        info.desc_print(" efl %016" PRIx64 "  ", regs->gregs[REG_EFL    ]);
        info.desc_print("cgfs %016" PRIx64 "  ", regs->gregs[REG_CSGSFS ]);
        info.desc_print(" err %016" PRIx64 "\n", regs->gregs[REG_ERR    ]);
        info.desc_print("trap %016" PRIx64 "  ", regs->gregs[REG_TRAPNO ]);
        info.desc_print("oldm %016" PRIx64 "  ", regs->gregs[REG_OLDMASK]);
        info.desc_print(" cr2 %016" PRIx64 "\n", regs->gregs[REG_CR2    ]);
#endif
#ifdef __i386__
        info.desc_print(" NOTE __i386__ linux mcontext_t register dump "
                        "not implemented\n");
#endif
    }

    handler(&info);
}


//// -------------- C interface --------------

void
signal_backtrace_init(const char *process_name,
                      SignalBacktraceHandler new_handler)
{
    SignalBacktrace::get_instance()->register_handler(process_name,
                                                      new_handler);
}

void
signal_backtrace_cleanup(void)
{
    SignalBacktrace::cleanup();
}

void
signal_backtrace_now(const char *reason)
{
    SignalBacktrace::get_instance()->backtrace_now(reason);
}
