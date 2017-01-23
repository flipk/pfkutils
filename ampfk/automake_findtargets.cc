/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
 */

#include <stdio.h>
#include <iostream>
#include <stdlib.h>

#include "automake_parser.h"
#include "tokenizer.h"

using namespace std;

struct targetvarnames_t {
    string varname;
    amtarget::tgttype target_type;
    amtarget::insttype install_type;
};

static const targetvarnames_t targetvarnames[] = {
    { "noinst_LIBRARIES", amtarget::TARGET_TYPE_LIB,  amtarget::INSTALL_NONE },
    { "noinst_PROGRAMS" , amtarget::TARGET_TYPE_PROG, amtarget::INSTALL_NONE },
    { "lib_LIBRARIES"   , amtarget::TARGET_TYPE_LIB,  amtarget::INSTALL_LIB  },
    { "bin_PROGRAMS"    , amtarget::TARGET_TYPE_PROG, amtarget::INSTALL_BIN  },
    { "" }
};

void
automake_file :: find_targets(void)
{
    const targetvarnames_t * tgtvarname;

    for (tgtvarname = targetvarnames;
         tgtvarname->varname.size() > 0;
         tgtvarname++)
    {
        amvariable * v = input_variables.find(tgtvarname->varname);
        if (!v)
            continue;

        amword * w;
        for (w = v->value.get_head(); w; w = v->value.get_next(w))
        {
            amtarget * t = make_amtarget(w,
                                         tgtvarname->target_type,
                                         tgtvarname->install_type);
            if (t)
                targets.add(t);
        }
    }
}

string *
automake_file :: underscoreize(string * str)
{
    string * ret = new string;
    ret->assign(*str);
    for (unsigned int ind = 0; ind < str->size(); ind++)
        if (ret->at(ind) == '.')
            ret->replace(ind,1,1,'_');
    return ret;
}

amtarget *
automake_file :: make_amtarget(amword * word,
                               amtarget::tgttype target_type,
                               amtarget::insttype install_type)
{
    amtarget * t = new amtarget;

    t->target = word;
    t->target_underscored = new amword;
    t->target_underscored->word = underscoreize(word->word);
    t->target_type = target_type;
    t->install_type = install_type;

    amvariable * v;
    string search;

#define SEARCH(field,name) \
    v = input_variables.find(*t->target_underscored->word + "_" + #name); \
    if (v) \
    { \
        if (0) cout << "  found : " << *v; \
        input_variables.remove(v); \
        t->field = v; \
    }

    SEARCH(cc,CC);
    SEARCH(cxx,CXX);
    SEARCH(sources,SOURCES);
    SEARCH(headers,HEADERS);
    SEARCH(includes,INCLUDES);
    SEARCH(ldadd,LDADD);
    SEARCH(ldflags,LDFLAGS);
    SEARCH(cflags,CFLAGS);
    SEARCH(cxxflags,CXXFLAGS);
    SEARCH(cppflags,CPPFLAGS);
    SEARCH(lflags,LFLAGS);
    SEARCH(yflags,YFLAGS);

#undef SEARCH

    if (t->sources)
        for (amword * w = t->sources->value.get_head();
             w;
             w = t->sources->value.get_next(w))
        {
            t->objects.add(new amword(make_o(t, w)));
        }
    else
    {
        printf("target %s : no SOURCES\n", word->word->c_str());
        exit(1);
    }

    return t;
}
