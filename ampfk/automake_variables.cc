
#include <stdio.h>
#include <iostream>

#include "automake_parser.H"
#include "tokenizer.H"

using namespace std;

void
automake_file :: make_variables(void)
{
    amvariable * v;

#define FINDIT(name) \
    v = input_variables.find(#name); \
    if (v) \
    { \
        input_variables.remove(v); \
        output_variables.add(v); \
    }

    FINDIT(CC);
    FINDIT(CFLAGS);
    FINDIT(CXX);
    FINDIT(CXXFLAGS);
    FINDIT(CPPFLAGS);
    FINDIT(AR);
    FINDIT(LEX);
    FINDIT(YACC);
    FINDIT(LFLAGS);
    FINDIT(YFLAGS);

#undef FINDIT

    amtarget * t;
    for (t = targets.get_head(); t; t = targets.get_next(t))
    {
#define FINDITORSUB(field,varname) \
        if (t->field) \
            output_variables.add(t->field); \
        else { \
            v = new amvariable; \
            v->var = new string(*t->target_underscored->word + "_" #varname); \
            v->value.add(new amword("$(" #varname ")")); \
            output_variables.add(v); \
        }

        FINDITORSUB(cc,CC);
        FINDITORSUB(cxx,CXX);

#undef FINDITORSUB


#define FINDIT(field) \
        if (t->field) \
            output_variables.add(t->field);

        FINDIT(ldadd);
        FINDIT(ldflags);
        FINDIT(cflags);
        FINDIT(cxxflags);
        FINDIT(cppflags);
        FINDIT(lflags);
        FINDIT(yflags);

#undef FINDIT
    }

    v = new amvariable;
    v->var = new string("srcdir");
    v->value.add(new amword(srcdir));
    output_variables.add(v);

    v = new amvariable;
    v->var = new string("builddir");
    v->value.add(new amword(builddir));
    output_variables.add(v);

    v = output_variables.find("CPPFLAGS");
    if (!v)
    {
        v = new amvariable;
        v->var = new string("CPPFLAGS");
        output_variables.add(v);
    }
    v->value.add(new amword("-I$(srcdir)"));
}
