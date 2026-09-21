#if 0
set -e -x
g++ -Wall -Wextra -std=c++11 posix_fe_test.cc -lpthread -lrt -o posix_fe_test
./posix_fe_test
rm -f posix_fe_test
exit 0
;
#endif

#include "posix_fe.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>

static int test_total = 0;
static int test_failed = 0;

#define TEST_ASSERT(cond) do { \
    test_total++; \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s:%d: assertion failed: %s\n", \
                __FILE__, __LINE__, #cond); \
        test_failed++; \
    } \
} while (0)

#define RUN_TEST(fn) do { \
    printf("[ RUN      ] %s\n", #fn); \
    int prev_failed = test_failed; \
    fn(); \
    if (test_failed == prev_failed) { \
        printf("[       OK ] %s\n", #fn); \
    } else { \
        printf("[  FAILED  ] %s (%d failure(s))\n", #fn, test_failed - prev_failed); \
    } \
} while (0)

// -----------------------------------------------------------------------------
// 1. pxfe_timeval tests
// -----------------------------------------------------------------------------
static void test_timeval(void)
{
    // Default constructor
    pxfe_timeval tv0;
    TEST_ASSERT(tv0.tv_sec == 0);
    TEST_ASSERT(tv0.tv_usec == 0);

    // Args constructor and copy constructor
    pxfe_timeval tv1(10, 500000);
    TEST_ASSERT(tv1.tv_sec == 10);
    TEST_ASSERT(tv1.tv_usec == 500000);
    pxfe_timeval tv1_copy(tv1);
    TEST_ASSERT(tv1_copy.tv_sec == 10);
    TEST_ASSERT(tv1_copy.tv_usec == 500000);

    // Copy assignment
    pxfe_timeval tv1_assign;
    tv1_assign = tv1;
    TEST_ASSERT(tv1_assign == tv1);

    // double constructor and set(double)
    pxfe_timeval tv_dbl(2.75);
    TEST_ASSERT(tv_dbl.tv_sec == 2);
    TEST_ASSERT(tv_dbl.tv_usec == 750000);
    TEST_ASSERT(tv_dbl.to_double() >= 2.749999 && tv_dbl.to_double() <= 2.750001);

    pxfe_timeval tv_neg;
    tv_neg.set(-3.5);
    TEST_ASSERT(tv_neg.tv_sec == -3);
    TEST_ASSERT(tv_neg.tv_usec == -500000);

    // operator+= with rollover at exact 1,000,000 usec
    pxfe_timeval tva(1, 400000);
    pxfe_timeval tvb(2, 600000);
    tva += tvb;
    TEST_ASSERT(tva.tv_sec == 4);
    TEST_ASSERT(tva.tv_usec == 0);

    // operator-= with borrow
    pxfe_timeval tvc(5, 100000);
    pxfe_timeval tvd(2, 400000);
    tvc -= tvd;
    TEST_ASSERT(tvc.tv_sec == 2);
    TEST_ASSERT(tvc.tv_usec == 700000);

    // operator+, operator-
    pxfe_timeval sum = tv1 + tv_dbl;
    TEST_ASSERT(sum.tv_sec == 13);
    TEST_ASSERT(sum.tv_usec == 250000);

    pxfe_timeval diff = sum - tv1;
    TEST_ASSERT(diff.tv_sec == 2);
    TEST_ASSERT(diff.tv_usec == 750000);

    // Comparisons
    pxfe_timeval ta(10, 100);
    pxfe_timeval tb(10, 200);
    pxfe_timeval tc(11, 50);
    TEST_ASSERT(ta == ta);
    TEST_ASSERT(ta != tb);
    TEST_ASSERT(ta < tb);
    TEST_ASSERT(tb > ta);
    TEST_ASSERT(ta <= tb);
    TEST_ASSERT(tb >= ta);
    TEST_ASSERT(ta < tc);
    TEST_ASSERT(tc > tb);

    // msecs(), usecs(), nsecs() with large 64-bit timestamps
    pxfe_timeval t_large(1800000000LL, 250000);
    TEST_ASSERT(t_large.msecs() == 1800000000250ULL);
    TEST_ASSERT(t_large.usecs() == 1800000000250000ULL);
    TEST_ASSERT(t_large.nsecs() == 1800000000250000000ULL);

    // breakdown
    pxfe_timeval dur(3665, 123456); // 1 hr, 1 min, 5 sec
    uint32_t hrs, mins, secs, us;
    dur.breakdown(hrs, mins, secs, us);
    TEST_ASSERT(hrs == 1);
    TEST_ASSERT(mins == 1);
    TEST_ASSERT(secs == 5);
    TEST_ASSERT(us == 123456);

    // Format
    pxfe_timeval t_fmt(1700000000LL, 123456);
    std::string s_fmt = t_fmt.Format("%Y");
    TEST_ASSERT(s_fmt.find("2023") != std::string::npos);
    TEST_ASSERT(s_fmt.find(".123456") != std::string::npos);

    // getNow()
    pxfe_timeval now;
    now.getNow();
    TEST_ASSERT(now.tv_sec > 1700000000LL);
}

// -----------------------------------------------------------------------------
// 2. pxfe_timespec tests
// -----------------------------------------------------------------------------
static void test_timespec(void)
{
    // Default constructor
    pxfe_timespec ts0;
    TEST_ASSERT(ts0.tv_sec == 0);
    TEST_ASSERT(ts0.tv_nsec == 0);

    // Args constructor and copy constructor
    pxfe_timespec ts1(10, 500000000);
    TEST_ASSERT(ts1.tv_sec == 10);
    TEST_ASSERT(ts1.tv_nsec == 500000000);
    pxfe_timespec ts1_copy(ts1);
    TEST_ASSERT(ts1_copy.tv_sec == 10);
    TEST_ASSERT(ts1_copy.tv_nsec == 500000000);

    // Copy assignment
    pxfe_timespec ts1_assign;
    ts1_assign = ts1;
    TEST_ASSERT(ts1_assign == ts1);

    // Conversion from timeval
    timeval tv_src;
    tv_src.tv_sec = 8;
    tv_src.tv_usec = 123456;
    pxfe_timespec ts_from_tv;
    ts_from_tv = tv_src;
    TEST_ASSERT(ts_from_tv.tv_sec == 8);
    TEST_ASSERT(ts_from_tv.tv_nsec == 123456000);

    // double constructor and set(double)
    pxfe_timespec ts_dbl(3.25);
    TEST_ASSERT(ts_dbl.tv_sec == 3);
    TEST_ASSERT(ts_dbl.tv_nsec == 250000000);

    pxfe_timespec ts_neg;
    ts_neg.set(-4.5);
    TEST_ASSERT(ts_neg.tv_sec == -4);
    TEST_ASSERT(ts_neg.tv_nsec == -500000000);

    // operator+= with rollover at exact 1,000,000,000 nsec
    pxfe_timespec tsa(1, 400000000);
    pxfe_timespec tsb(2, 600000000);
    tsa += tsb;
    TEST_ASSERT(tsa.tv_sec == 4);
    TEST_ASSERT(tsa.tv_nsec == 0);

    // operator-= with borrow
    pxfe_timespec tsc(5, 100000000);
    pxfe_timespec tsd(2, 400000000);
    tsc -= tsd;
    TEST_ASSERT(tsc.tv_sec == 2);
    TEST_ASSERT(tsc.tv_nsec == 700000000);

    // Comparisons
    pxfe_timespec ta(10, 100);
    pxfe_timespec tb(10, 200);
    pxfe_timespec tc(11, 50);
    TEST_ASSERT(ta == ta);
    TEST_ASSERT(ta != tb);
    TEST_ASSERT(ta < tb);
    TEST_ASSERT(tb > ta);
    TEST_ASSERT(ta <= tb);
    TEST_ASSERT(tb >= ta);
    TEST_ASSERT(ta < tc);

    // 64-bit msecs(), usecs(), nsecs()
    pxfe_timespec ts_large(1800000000LL, 500000000);
    TEST_ASSERT(ts_large.msecs() == 1800000000500ULL);
    TEST_ASSERT(ts_large.usecs() == 1800000000500000ULL);
    TEST_ASSERT(ts_large.nsecs() == 1800000000500000000ULL);

    // getNow() & getMonotonic()
    pxfe_timespec now_rt, now_mono;
    now_rt.getNow();
    now_mono.getMonotonic();
    TEST_ASSERT(now_rt.tv_sec > 1700000000LL);
    TEST_ASSERT(now_mono.tv_sec >= 0);
}

// -----------------------------------------------------------------------------
// 3. pxfe_shared_ptr & pxfe_shared_ptr_base tests
// -----------------------------------------------------------------------------
struct TestSharedBase : public pxfe_shared_ptr_base {
    static int base_alive;
    int base_val;
    TestSharedBase(int v = 0) : base_val(v) { base_alive++; }
    virtual ~TestSharedBase(void) { base_alive--; }
};
int TestSharedBase::base_alive = 0;

struct TestSharedDerived : public TestSharedBase {
    static int derived_alive;
    int derived_val;
    TestSharedDerived(int bv, int dv) : TestSharedBase(bv), derived_val(dv) {
        derived_alive++;
    }
    virtual ~TestSharedDerived(void) { derived_alive--; }
};
int TestSharedDerived::derived_alive = 0;

static void test_shared_ptr(void)
{
    TEST_ASSERT(TestSharedBase::base_alive == 0);
    TEST_ASSERT(TestSharedDerived::derived_alive == 0);

    {
        pxfe_shared_ptr<TestSharedBase> p1(new TestSharedBase(42));
        TEST_ASSERT(TestSharedBase::base_alive == 1);
        TEST_ASSERT(p1.unique());
        TEST_ASSERT(p1->use_count() == 1);
        TEST_ASSERT(p1->base_val == 42);
        TEST_ASSERT((bool)p1);

        // Copy constructor
        {
            pxfe_shared_ptr<TestSharedBase> p2(p1);
            TEST_ASSERT(p1->use_count() == 2);
            TEST_ASSERT(p2->use_count() == 2);
            TEST_ASSERT(!p1.unique());
        }
        TEST_ASSERT(p1->use_count() == 1);
        TEST_ASSERT(p1.unique());

        // Self-assignment safety
        p1 = p1;
        TEST_ASSERT(TestSharedBase::base_alive == 1);
        TEST_ASSERT(p1->use_count() == 1);

        // Self-reset safety
        p1.reset(p1.get());
        TEST_ASSERT(TestSharedBase::base_alive == 1);
        TEST_ASSERT(p1->use_count() == 1);

        // reset to a new object
        p1.reset(new TestSharedBase(99));
        TEST_ASSERT(TestSharedBase::base_alive == 1);
        TEST_ASSERT(p1->base_val == 99);

        // _take and _give
        TestSharedBase *raw = p1._take();
        TEST_ASSERT(!p1);
        TEST_ASSERT(raw != NULL);
        TEST_ASSERT(TestSharedBase::base_alive == 1);

        p1._give(raw);
        TEST_ASSERT((bool)p1);
        TEST_ASSERT(p1->base_val == 99);
    }
    TEST_ASSERT(TestSharedBase::base_alive == 0);

    // Polymorphic casting constructor and assignment
    {
        pxfe_shared_ptr<TestSharedDerived> p_der(new TestSharedDerived(10, 20));
        TEST_ASSERT(TestSharedBase::base_alive == 1);
        TEST_ASSERT(TestSharedDerived::derived_alive == 1);

        // Upcast to base
        pxfe_shared_ptr<TestSharedBase> p_base(p_der);
        TEST_ASSERT(p_base->use_count() == 2);
        TEST_ASSERT(p_base->base_val == 10);

        // Downcast from base
        pxfe_shared_ptr<TestSharedDerived> p_der2;
        p_der2 = p_base;
        TEST_ASSERT((bool)p_der2);
        TEST_ASSERT(p_der2->derived_val == 20);
        TEST_ASSERT(p_base->use_count() == 3);

        // Casting non-derived type yields null
        pxfe_shared_ptr<TestSharedBase> p_base_plain(new TestSharedBase(100));
        pxfe_shared_ptr<TestSharedDerived> p_failed_cast(p_base_plain);
        TEST_ASSERT(!p_failed_cast);
    }
    TEST_ASSERT(TestSharedBase::base_alive == 0);
    TEST_ASSERT(TestSharedDerived::derived_alive == 0);
}

// -----------------------------------------------------------------------------
// 4. pxfe_errno tests
// -----------------------------------------------------------------------------
static void test_errno(void)
{
    pxfe_errno e0;
    TEST_ASSERT(e0.e == 0);
    TEST_ASSERT(e0.err == NULL);

    pxfe_errno e_copy;
    {
        pxfe_errno e_orig(ENOENT, "test_op");
        TEST_ASSERT(e_orig.e == ENOENT);
        TEST_ASSERT(e_orig.err != NULL);
        e_copy = e_orig;
    }
    // e_orig is out of scope; e_copy must have valid err pointing to its own storage
    TEST_ASSERT(e_copy.e == ENOENT);
    TEST_ASSERT(e_copy.err != NULL);
    std::string str = e_copy.Format();
    TEST_ASSERT(str.find("test_op") != std::string::npos);
    TEST_ASSERT(str.find(std::to_string(ENOENT)) != std::string::npos);
}

// -----------------------------------------------------------------------------
// 5. pxfe_string tests
// -----------------------------------------------------------------------------
static void test_string(void)
{
    pxfe_string s("hello world");
    TEST_ASSERT(s.length() == 11);
    TEST_ASSERT(s.vptr() != NULL);
    TEST_ASSERT(s.ucptr() != NULL);
    TEST_ASSERT(s.u8ptr() != NULL);

    // startswith
    TEST_ASSERT(s.startswith("hello"));
    TEST_ASSERT(s.startswith("hello", 5));
    TEST_ASSERT(!s.startswith("world"));
    TEST_ASSERT(!s.startswith("hello world extra"));
    TEST_ASSERT(s.startswith(""));
    TEST_ASSERT(!s.startswith(NULL));

    // endswith
    TEST_ASSERT(s.endswith("world"));
    TEST_ASSERT(s.endswith("world", 5));
    TEST_ASSERT(!s.endswith("hello"));
    TEST_ASSERT(!s.endswith("long hello world"));
    TEST_ASSERT(s.endswith(""));
    TEST_ASSERT(!s.endswith(NULL));

    // format_hex
    pxfe_string s_bin;
    s_bin.push_back((char)0xde);
    s_bin.push_back((char)0xad);
    s_bin.push_back((char)0xbe);
    s_bin.push_back((char)0xef);
    TEST_ASSERT(s_bin.format_hex() == "deadbeef");

    // chained assignment
    pxfe_string s1, s2;
    s2 = s1 = s;
    TEST_ASSERT(s2 == "hello world");
    TEST_ASSERT(s1 == "hello world");
}

// -----------------------------------------------------------------------------
// 6. pxfe_pthread_mutex & pxfe_pthread_mutex_lock tests
// -----------------------------------------------------------------------------
static void test_mutex(void)
{
    pxfe_pthread_mutex mut;
    // Auto-init on lock/trylock
    TEST_ASSERT(mut.trylock() == 0);
    mut.unlock();

    mut.lock();
    mut.unlock();

    // RAII lock wrapper
    {
        pxfe_pthread_mutex_lock lock(mut);
        // mut is locked
        TEST_ASSERT(!lock.lock()); // already locked
        TEST_ASSERT(lock.unlock()); // unlocked
        TEST_ASSERT(lock.lock());   // locked again
    }
    // Automatically unlocked on destruction
    TEST_ASSERT(mut.trylock() == 0);
    mut.unlock();
}

// -----------------------------------------------------------------------------
// 7. pxfe_semaphore tests
// -----------------------------------------------------------------------------
struct SemThreadArg {
    pxfe_semaphore *sem;
    int delay_ms;
};

static void *sem_worker(void *arg)
{
    SemThreadArg *a = (SemThreadArg *)arg;
    usleep(a->delay_ms * 1000);
    a->sem->give();
    return NULL;
}

static void test_semaphore(void)
{
    pxfe_semaphore sem(0);

    // Test timeout (must return false after 100ms, not hang forever!)
    pxfe_timespec expire;
    expire.getNow();
    expire.tv_nsec += 100000000; // +100ms
    if (expire.tv_nsec >= 1000000000) {
        expire.tv_sec += 1;
        expire.tv_nsec -= 1000000000;
    }

    bool timed_out = !sem.take(expire());
    TEST_ASSERT(timed_out);

    // Test give and take with worker thread
    SemThreadArg arg;
    arg.sem = &sem;
    arg.delay_ms = 50;

    pthread_t th;
    pthread_create(&th, NULL, sem_worker, &arg);

    bool taken = sem.take(NULL); // wait forever
    TEST_ASSERT(taken);
    pthread_join(th, NULL);

    // Test P() and V() aliases
    sem.V(); // give
    TEST_ASSERT(sem.P()); // take
}

// -----------------------------------------------------------------------------
// 8. pxfe_pipe & pxfe_fd tests
// -----------------------------------------------------------------------------
static void test_pipe_and_fd(void)
{
    pxfe_pipe p;
    TEST_ASSERT(p.readEnd >= 0);
    TEST_ASSERT(p.writeEnd >= 0);

    std::string msg = "Testing pxfe_pipe communication!";
    bool w_ok = p.write(msg);
    TEST_ASSERT(w_ok);

    std::string read_buf;
    bool r_ok = p.read(read_buf, 100);
    TEST_ASSERT(r_ok);
    TEST_ASSERT(read_buf == msg);

    // Raw buffer read/write
    int val_out = 0x12345678;
    int val_in = 0;
    TEST_ASSERT(p.write(&val_out, sizeof(val_out)) == sizeof(val_out));
    TEST_ASSERT(p.read(&val_in, sizeof(val_in)) == sizeof(val_in));
    TEST_ASSERT(val_in == val_out);

    // Move semantics on pxfe_pipe
    pxfe_pipe p_moved(std::move(p));
    TEST_ASSERT(p.readEnd == -1);
    TEST_ASSERT(p.writeEnd == -1);
    TEST_ASSERT(p_moved.readEnd >= 0);

    // pxfe_fd open, read, write, close
    char tmp_file[] = "/tmp/pxfe_fd_test_XXXXXX";
    int fd_raw = mkstemp(tmp_file);
    TEST_ASSERT(fd_raw >= 0);
    ::close(fd_raw);

    pxfe_fd fd_obj;
    TEST_ASSERT(fd_obj.open(tmp_file, O_RDWR | O_TRUNC));
    TEST_ASSERT(fd_obj.getFd() >= 0);

    std::string file_data = "File content test";
    TEST_ASSERT(fd_obj.write(file_data));
    fd_obj.close();
    TEST_ASSERT(fd_obj.getFd() == -1);

    TEST_ASSERT(fd_obj.open(tmp_file, O_RDONLY));
    std::string file_in;
    TEST_ASSERT(fd_obj.read(file_in, 100));
    TEST_ASSERT(file_in == file_data);
    fd_obj.close();

    unlink(tmp_file);
}

// -----------------------------------------------------------------------------
// 9. pxfe_fd_set, pxfe_select, and pxfe_poll tests
// -----------------------------------------------------------------------------
static void test_select_and_poll(void)
{
    pxfe_pipe p;

    // pxfe_fd_set bounds safety
    pxfe_fd_set fds;
    fds.set(-1);
    fds.clr(-1);
    TEST_ASSERT(!fds.is_set(-1));
    fds.set(FD_SETSIZE + 50);
    fds.clr(FD_SETSIZE + 50);
    TEST_ASSERT(!fds.is_set(FD_SETSIZE + 50));

    fds.set(p.readEnd);
    TEST_ASSERT(fds.is_set(p.readEnd));
    TEST_ASSERT(fds.nfds() == p.readEnd + 1);
    fds.clr(p.readEnd);
    TEST_ASSERT(!fds.is_set(p.readEnd));

    // pxfe_select with timeout
    pxfe_select sel;
    sel.rfds.zero();
    sel.rfds.set(p.readEnd);
    sel.tv.set(0, 50000); // 50ms timeout
    int sel_ret = sel.select();
    TEST_ASSERT(sel_ret == 0); // timeout, no data yet

    // Write data, select should return ready
    std::string byte_msg = "X";
    p.write(byte_msg);
    sel.rfds.zero();
    sel.rfds.set(p.readEnd);
    sel.tv.set(1, 0);
    sel_ret = sel.select();
    TEST_ASSERT(sel_ret > 0);
    TEST_ASSERT(sel.rfds.is_set(p.readEnd));

    std::string read_out;
    p.read(read_out, 10);

    // pxfe_poll
    pxfe_poll pol;
    pol.set(-1, POLLIN); // negative fd safety
    TEST_ASSERT(pol.eget(-1) == 0);
    TEST_ASSERT(pol.rget(-1) == 0);

    pol.set(p.readEnd, POLLIN);
    TEST_ASSERT(pol.eget(p.readEnd) == POLLIN);

    // Poll timeout
    int pol_ret = pol.poll(50);
    TEST_ASSERT(pol_ret == 0);

    // Poll ready
    p.write(byte_msg);
    pol_ret = pol.poll(500);
    TEST_ASSERT(pol_ret > 0);
    TEST_ASSERT(pol.rget(p.readEnd) & POLLIN);

    p.read(read_out, 10);

    // Remove descriptor
    pol.set(p.readEnd, 0);
    TEST_ASSERT(pol.eget(p.readEnd) == 0);
}

// -----------------------------------------------------------------------------
// 10. pxfe_ticker tests
// -----------------------------------------------------------------------------
static void test_ticker(void)
{
    pxfe_ticker tick;
    // 25ms interval
    tick.start(0, 25000);
    TEST_ASSERT(tick.fd() >= 0);

    pxfe_select sel;
    int tick_count = 0;
    for (int i = 0; i < 3; i++) {
        sel.rfds.zero();
        sel.rfds.set(tick.fd());
        sel.tv.set(1, 0);
        if (sel.select() > 0 && sel.rfds.is_set(tick.fd())) {
            if (tick.doread())
                tick_count++;
        }
    }
    TEST_ASSERT(tick_count >= 2);

    tick.pause();
    usleep(50000);
    // Drain any pending ticks
    tick.doread();

    // After pause, select with short timeout should time out
    sel.rfds.zero();
    sel.rfds.set(tick.fd());
    sel.tv.set(0, 60000);
    int ret = sel.select();
    TEST_ASSERT(ret == 0);

    tick.resume();
    tick.stopjoin();
}

// -----------------------------------------------------------------------------
// 11. pxfe_utils::parse_number and format_thousands tests
// -----------------------------------------------------------------------------
static void test_utils_parse(void)
{
    // uint32_t
    uint32_t u32 = 0;
    TEST_ASSERT(pxfe_utils::parse_number("12345", &u32));
    TEST_ASSERT(u32 == 12345);
    TEST_ASSERT(pxfe_utils::parse_number("0", &u32));
    TEST_ASSERT(u32 == 0);
    TEST_ASSERT(!pxfe_utils::parse_number("", &u32));
    TEST_ASSERT(!pxfe_utils::parse_number("4294967296", &u32)); // > UINT32_MAX
    TEST_ASSERT(!pxfe_utils::parse_number("123abc", &u32));

    // int32_t
    int32_t s32 = 0;
    TEST_ASSERT(pxfe_utils::parse_number("-54321", &s32));
    TEST_ASSERT(s32 == -54321);
    TEST_ASSERT(!pxfe_utils::parse_number("", &s32));
    TEST_ASSERT(!pxfe_utils::parse_number("3000000000", &s32)); // > INT32_MAX
    TEST_ASSERT(!pxfe_utils::parse_number("-3000000000", &s32));

    // uint64_t and int64_t
    uint64_t u64 = 0;
    TEST_ASSERT(pxfe_utils::parse_number("1234567890123456789", &u64));
    TEST_ASSERT(u64 == 1234567890123456789ULL);
    TEST_ASSERT(!pxfe_utils::parse_number("", &u64));

    int64_t s64 = 0;
    TEST_ASSERT(pxfe_utils::parse_number("-1234567890123456789", &s64));
    TEST_ASSERT(s64 == -1234567890123456789LL);
    TEST_ASSERT(!pxfe_utils::parse_number("", &s64));

    // double and float
    double dval = 0;
    TEST_ASSERT(pxfe_utils::parse_number("3.14159", &dval));
    TEST_ASSERT(dval > 3.14 && dval < 3.15);
    TEST_ASSERT(!pxfe_utils::parse_number("", &dval));

    float fval = 0;
    TEST_ASSERT(pxfe_utils::parse_number("-2.5", &fval));
    TEST_ASSERT(fval == -2.5f);
    TEST_ASSERT(!pxfe_utils::parse_number("", &fval));

    // format_thousands
    std::string out;
    pxfe_utils::format_thousands(out, 0);
    TEST_ASSERT(out == "0");
    pxfe_utils::format_thousands(out, 12);
    TEST_ASSERT(out == "12");
    pxfe_utils::format_thousands(out, 1234);
    TEST_ASSERT(out == "1,234");
    pxfe_utils::format_thousands(out, 1234567890ULL);
    TEST_ASSERT(out == "1,234,567,890");
}

// -----------------------------------------------------------------------------
// 12. pxfe_iputils tests
// -----------------------------------------------------------------------------
static void test_iputils(void)
{
    uint16_t port = 0;
    TEST_ASSERT(pxfe_iputils::parse_port_number("80", &port));
    TEST_ASSERT(port == 80);
    TEST_ASSERT(pxfe_iputils::parse_port_number("65535", &port));
    TEST_ASSERT(port == 65535);
    TEST_ASSERT(!pxfe_iputils::parse_port_number("", &port));
    TEST_ASSERT(!pxfe_iputils::parse_port_number("65536", &port));
    TEST_ASSERT(!pxfe_iputils::parse_port_number("99999", &port));
    TEST_ASSERT(!pxfe_iputils::parse_port_number("-1", &port));
    TEST_ASSERT(!pxfe_iputils::parse_port_number("http", &port));

    // format_ip
    std::string ip_str = pxfe_iputils::format_ip(0x7f000001); // 127.0.0.1
    TEST_ASSERT(ip_str == "127.0.0.1");

    // format_ip6
    in6_addr ip6;
    memset(&ip6, 0, sizeof(ip6));
    ip6.s6_addr[15] = 1; // ::1
    std::string ip6_str = pxfe_iputils::format_ip6(ip6);
    TEST_ASSERT(ip6_str == "::1");
}

// -----------------------------------------------------------------------------
// 13. pxfe_sockaddr family tests
// -----------------------------------------------------------------------------
static void test_sockaddrs(void)
{
    // pxfe_sockaddr_in
    pxfe_sockaddr_in sin;
    sin.init_any(8080);
    TEST_ASSERT(sin.sin_family == AF_INET);
    TEST_ASSERT(sin.get_port() == 8080);
    TEST_ASSERT(sin.get_addr() == INADDR_ANY);

    sin.set_addr(0x7f000001);
    sin.set_port(9090);
    TEST_ASSERT(sin.get_addr() == 0x7f000001);
    TEST_ASSERT(sin.get_port() == 9090);

    // pxfe_sockaddr_un
    pxfe_sockaddr_un sun;
    sun.init("/tmp/test_sock.un");
    TEST_ASSERT(sun.sun_family == AF_UNIX);
    std::string un_path;
    sun.get_path(un_path);
    TEST_ASSERT(un_path == "/tmp/test_sock.un");

    // pxfe_sockaddr
    pxfe_sockaddr sa;
    TEST_ASSERT(sa.set4("192.168.1.1"));
    TEST_ASSERT(sa.family() == AF_INET);
    TEST_ASSERT(sa.Format() == "192.168.1.1");

    TEST_ASSERT(sa.set6("fe80::1"));
    TEST_ASSERT(sa.family() == AF_INET6);
    TEST_ASSERT(sa.Format() == "fe80::1");

    sa.set_un("/tmp/generic_un.sock");
    TEST_ASSERT(sa.family() == AF_UNIX);
    std::string sa_path;
    sa.get_un(sa_path);
    TEST_ASSERT(sa_path == "/tmp/generic_un.sock");
    TEST_ASSERT(sa.Format() == "unix:/tmp/generic_un.sock");
}

// -----------------------------------------------------------------------------
// 14. Network sockets: UDP and TCP stream
// -----------------------------------------------------------------------------
static void test_sockets(void)
{
    // UDP socket loopback test
    pxfe_udp_socket udp_srv, udp_cli;
    TEST_ASSERT(udp_srv.init(0)); // bind to ephemeral port
    TEST_ASSERT(udp_cli.init());

    sockaddr_in srv_addr;
    socklen_t srv_len = sizeof(srv_addr);
    getsockname(udp_srv.getFd(), (sockaddr*)&srv_addr, &srv_len);
    uint16_t srv_port = ntohs(srv_addr.sin_port);

    sockaddr_in to_addr;
    memset(&to_addr, 0, sizeof(to_addr));
    to_addr.sin_family = AF_INET;
    to_addr.sin_port = htons(srv_port);
    to_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    std::string udp_msg = "Hello UDP!";
    TEST_ASSERT(udp_cli.send(udp_msg, to_addr));

    std::string udp_rx;
    sockaddr_in from_addr;
    TEST_ASSERT(udp_srv.recv(udp_rx, from_addr));
    TEST_ASSERT(udp_rx == udp_msg);

    // TCP stream socket loopback test
    pxfe_tcp_stream_socket tcp_srv, tcp_cli;
    TEST_ASSERT(tcp_srv.init(0, true)); // ephemeral port, reuse
    tcp_srv.listen(5);

    socklen_t tcp_len = sizeof(srv_addr);
    getsockname(tcp_srv.getFd(), (sockaddr*)&srv_addr, &tcp_len);
    uint16_t tcp_port = ntohs(srv_addr.sin_port);

    TEST_ASSERT(tcp_cli.init());
    TEST_ASSERT(tcp_cli.connect(INADDR_LOOPBACK, tcp_port));

    pxfe_tcp_stream_socket *accepted = tcp_srv.accept();
    TEST_ASSERT(accepted != NULL);

    std::string tcp_msg = "Hello TCP stream!";
    TEST_ASSERT(tcp_cli.send(tcp_msg));

    std::string tcp_rx;
    TEST_ASSERT(accepted->recv(tcp_rx));
    TEST_ASSERT(tcp_rx == tcp_msg);

    delete accepted;

    // UNIX datagram socket test
    pxfe_unix_dgram_socket uds1, uds2;
    TEST_ASSERT(uds1.init());
    TEST_ASSERT(uds2.init());
    TEST_ASSERT(uds1.getPath() != "");
    TEST_ASSERT(uds2.getPath() != "");

    std::string uds_msg = "Hello UNIX dgram!";
    TEST_ASSERT(uds1.send(uds_msg, uds2.getPath()));

    std::string uds_rx;
    std::string uds_from;
    TEST_ASSERT(uds2.recv(uds_rx, uds_from));
    TEST_ASSERT(uds_rx == uds_msg);
    TEST_ASSERT(uds_from == uds1.getPath());
}

// -----------------------------------------------------------------------------
// Main test runner
// -----------------------------------------------------------------------------
int main(void)
{
    printf("==================================================\n");
    printf(" Running posix_fe test suite\n");
    printf("==================================================\n");

    RUN_TEST(test_timeval);
    RUN_TEST(test_timespec);
    RUN_TEST(test_shared_ptr);
    RUN_TEST(test_errno);
    RUN_TEST(test_string);
    RUN_TEST(test_mutex);
    RUN_TEST(test_semaphore);
    RUN_TEST(test_pipe_and_fd);
    RUN_TEST(test_select_and_poll);
    RUN_TEST(test_ticker);
    RUN_TEST(test_utils_parse);
    RUN_TEST(test_iputils);
    RUN_TEST(test_sockaddrs);
    RUN_TEST(test_sockets);

    printf("==================================================\n");
    printf(" Results: %d total assertions, %d failed\n",
           test_total, test_failed);
    printf("==================================================\n");

    return (test_failed == 0) ? 0 : 1;
}
