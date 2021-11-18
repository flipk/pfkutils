
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "simple_json.h"

static void callback(void *arg, const std::string &buf);

int
main(int argc, char ** argv)
{
    char buf[100];
    FILE * f = fopen(argv[1], "re");
    SimpleJson::SimpleJsonCollector   sjc(callback, NULL);
    if (f)
    {
        int cc;
        while ((cc = fread(buf, 1, sizeof(buf), f)) > 0)
        {
            int pos = 0, remain = cc;
            while (remain > 0)
            {
                int added = sjc.add_data(buf + pos, remain);
                remain -= added;
                pos += added;
            }
        }
        fclose(f);
    }
    return 0;
}

using namespace SimpleJson;

static std::string convertline(const std::string &l)
{
    std::string ret;
    bool prev_backslash = false;
    for (size_t ind = 0; ind < l.size(); ind++)
    {
        char c = l[ind];
        if (prev_backslash)
        {
            switch (c)
            {
            case '\\':   ret += '\\';   break;
            case 'n':    ret += '\n';   break;
            default:
                fprintf(stderr,"unhandled escape '%c'\n", c);
            }
            prev_backslash = false;
        }
        else
        {
            if (c == '\\')
                prev_backslash = true;
            else
                ret += c;
        }
    }
    return ret;
}

template <class T> struct __deleter {
    T * ptr;
    __deleter(T * _p) { ptr = _p; }
    ~__deleter(void) { if (ptr) delete ptr; }
};

static void callback(void *arg, const std::string &buf)
{
    Property * p = parseJson(buf);
    if (!p)
        return;
    __deleter<Property> del(p);
    ObjectProperty  * o = p->cast<ObjectProperty>();
    if (!o)
        return;
    Property * ap = o->getName("cells");
    if (!ap)
        return;
    ArrayProperty * a = ap->cast<ArrayProperty>();
    if (!a)
        return;
    for (size_t ind = 0; ind < a->size(); ind++)
    {
        ObjectProperty * ao =
            a->get(ind)->cast<ObjectProperty>();
        if (!ao)
            return;
        Property * sp = ao->getName("cell_type");
        if (!sp)
            return;
        StringProperty * s = sp->cast<StringProperty>();
        if (!s)
            return;
        if (s->value != "code")
            continue;
        Property * srcap = ao->getName("source");
        if (!srcap)
            return;
        ArrayProperty * srca = srcap->cast<ArrayProperty>();
        if (!srca)
            return;
        for (size_t srcind = 0; srcind < srca->size(); srcind++)
        {
            Property * srclinep = srca->get(srcind);
            if (!srclinep)
                return;
            StringProperty * srcline = srclinep->cast<StringProperty>();
            if (!srcline)
                return;
            std::string convertedline = convertline(srcline->value);
            std::cout << convertedline;
        }
    }
}
