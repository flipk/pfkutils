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

#include "automake_parser.h"
#include "tokenizer.h"

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
