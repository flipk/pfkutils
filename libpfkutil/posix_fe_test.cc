#if 0
set -e -x
g++ posix_fe_test.cc -lpthread -o posix_fe_test
./posix_fe_test
rm -f posix_fe_test
exit 0
;
#endif

// TODO someday convert this to a gtest executable
// TODO add a lot more tests of things other than semamphore.........

#include "posix_fe.h"

struct thread_data {
    pxfe_semaphore *s;
};

void *func(void*arg)
{
    thread_data *td = (thread_data *) arg;

    sleep(1);
    printf("thread giving\n");
    td->s->give();

    return NULL;
}

static void print_value64(uint64_t v)
{
    std::string  out;
    pxfe_utils::format_thousands(out, v);
    printf("%016" PRIx64 " = %20" PRIu64 " = '%s'\n",
           v, v, out.c_str());
}

static void test_format_thousands(void)
{
    uint64_t  v = 0;
    uint32_t dig = 1;
    uint32_t ctr;

    print_value64(v);

    for (ctr = 0; ctr < 20; ctr++)
    {
        v *= 10;
        v += dig;
        dig++;
        dig %= 10;
        print_value64(v);
    }
}

int main()
{
    struct thread_data td;

    td.s = new pxfe_semaphore(1);

    pthread_t id;
    pthread_attr_t         attr;
    pthread_attr_init(    &attr );
    pthread_create( &id,  &attr, &func, &td);
    pthread_attr_destroy( &attr );

    printf("taking first time\n");
    td.s->take();
    printf("taking first time success\n");
    printf("taking second time, should block\n");
    td.s->take();
    printf("taking second time success\n");

    td.s->give();

    void *ret = NULL;
    pthread_join(id, &ret);

    test_format_thousands();

    return 0;
}
