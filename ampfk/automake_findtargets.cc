
#include <stdio.h>
#include <iostream>

#include "automake_parser.H"
#include "tokenizer.H"

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
    t->target_type = target_type;
    t->install_type = install_type;

    amvariable * v;
    string * search;

#define SEARCH(field,name) \
    search = underscoreize(word->word); \
    search->append("_"); \
    search->append(#name); \
    v = input_variables.find(*search); \
    if (v) \
    { \
        if (0) cout << "  found : " << *v;      \
        t->field = v; \
    } \
    delete search;

    SEARCH(sources,SOURCES);
    SEARCH(headers,HEADERS);
    SEARCH(includes,INCLUDES);
    SEARCH(ldadd,LDADD);
    SEARCH(ldflags,LDFLAGS);
    SEARCH(cflags,CFLAGS);
    SEARCH(cxxflags,CXXFLAGS);
    SEARCH(cppflags,CPPFLAGS);

#undef SEARCH

    return t;
}
