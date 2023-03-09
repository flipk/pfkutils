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

std::ostream&
operator<<(std::ostream& ostr, const amwordList& val)
{
    amword * w, * nw;
    for (w = val.get_head(); w; w = nw)
    {
        nw = val.get_next(w);
        ostr << *w->word;
        if (nw)
            ostr << " ";
    }
    return ostr;
}

std::ostream&
operator<<(std::ostream& ostr, const amvariable& val)
{
    ostr << *val.var << " = " << val.value << endl;
    return ostr;
}

std::ostream&
operator<<(std::ostream& ostr, const amvariables& val)
{
    amvariable * t;
    for (t = val.get_head(); t; t = val.get_next(t))
        ostr << *t;
    ostr << endl;
    return ostr;
}

std::ostream&
operator<<(std::ostream& ostr, const amcommand& val)
{
    ostr << "\t" << val.cmd << endl;
    return ostr;
}

std::ostream&
operator<<(std::ostream& ostr, const amcommandList& val)
{
    amcommand * c;
    for (c = val.get_head(); c; c = val.get_next(c))
        ostr << *c;
    return ostr;
}

std::ostream&
operator<<(std::ostream& ostr, const amrule& val)
{
    ostr << val.targets << ": " << val.sources << endl;
    ostr << val.commands << endl;
    return ostr;
}

std::ostream&
operator<<(std::ostream& ostr, const amruleList& val)
{
    amrule * r;
    for (r = val.get_head(); r; r = val.get_next(r))
        ostr << *r;
    return ostr;
}

std::ostream&
operator<<(std::ostream& ostr, const amtarget& val)
{
    ostr << "target: " << *val.target->word << " ";
    switch (val.target_type)
    {
    case amtarget::TARGET_TYPE_LIB:  ostr << "(lib) ";  break;
    case amtarget::TARGET_TYPE_PROG: ostr << "(prog) "; break;
    }
    switch (val.install_type)
    {
    case amtarget::INSTALL_NONE: ostr << "(noinst) "; break;
    case amtarget::INSTALL_LIB:  ostr << "(lib) ";    break;
    case amtarget::INSTALL_BIN:  ostr << "(bin) ";    break;
    }
    ostr << endl;

#define OUTFIELD(field) \
    if (val.field) ostr << "\t" << #field << ": " << val.field->value << endl;

    OUTFIELD(sources);
    OUTFIELD(headers);
    OUTFIELD(includes);
    OUTFIELD(ldadd);
    OUTFIELD(ldflags);
    OUTFIELD(cflags);
    OUTFIELD(cxxflags);
    OUTFIELD(cppflags);
    OUTFIELD(lflags);
    OUTFIELD(yflags);

#undef  OUTFIELD

    if (val.objects.get_cnt() > 0)
    {
        ostr << "\tobjects: ";
        for (amword * w = val.objects.get_head();
             w;
             w = val.objects.get_next(w))
        {
            ostr << *w->word << " ";
        }
        ostr << endl;
    }

    return ostr;
}

std::ostream&
operator<<(std::ostream& ostr, const amtargetList& val)
{
    amtarget * t;
    for (t = val.get_head(); t; t = val.get_next(t))
        ostr << *t;
    return ostr;
}
