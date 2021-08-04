
#include <memory>
#include <iostream>
#include <stdio.h>

struct thing {

    typedef std::shared_ptr<thing> sp;
    template <typename... _Args>
    static sp make_s(_Args&&... __args)
    { return std::make_shared<thing>(std::forward<_Args>(__args)...); }

    typedef std::unique_ptr<thing> up;
    template <typename... _Args>
    static up make_u(_Args&&... __args)
    { return up(new thing(std::forward<_Args>(__args)...)); }

    thing(int _a) : a(_a) { printf("thingy constructor a=%d\n", a); }
    ~thing(void) { printf("thingy destructor a=%d\n",a); }
    void method(void) { printf("thingy method a=%d\n", a); }

    static void print(sp &x, const std::string &varname)
    {
        std::cout << varname << ": ";
        if (x)
        {
            std::cout << " use_count: " << x.use_count()
                      << " unique: " << x.unique() << " ";
            x->method();
        }
        else
            std::cout << "null\n";
    }

    static void print(up &x, const std::string &varname)
    {
        std::cout << varname << ": ";
        if (x)
            x->method();
        else
            std::cout << "null\n";
    }

private:
    int a;
};

int
main()
{
    thing::sp x;

    thing::print(x, "x 0");
    x = thing::make_s(4);
    thing::print(x, "x 1");

    {
        thing::sp y = x; // = std::move(x);

        thing::print(x, "x 2");
        thing::print(y, "y 1");

        x = thing::make_s(5);
        thing::print(x, "x 3");
        thing::print(y, "y 2");

    } // y deleted here

    thing::print(x, "x 4");

    printf("calling reset\n");
    x.reset();
    printf("called reset\n");

    thing::print(x, "x 5");

    thing::up z;

    z = std::unique_ptr<thing>(new thing(7));
    thing::print(z, "z 1");

    z = thing::make_u(8);
    thing::print(z, "z 2");

    z.reset(new thing(9));
    thing::print(z, "z 3");

    return 0;
}
