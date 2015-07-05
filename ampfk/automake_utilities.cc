
#include <stdio.h>
#include <iostream>

#include "automake_parser.h"
#include "tokenizer.h"

using namespace std;

string
replace_suffix(const string &file, const string &suffix)
{
    std::string  ret = file;
    size_t pos = ret.find_last_of(".");
    if (pos != string::npos)
        ret.replace(pos+1,string::npos,suffix);
    return ret;
}

amword *
make_d(const amtarget * t, const amword * source)
{
    amword * w = new amword;
    w->word = new string;
    w->word->assign(*t->target_underscored->word);
    w->word->append("-");
    w->word->append(replace_suffix(*source->word, "d"));
    return w;
}

amword *
make_o(const amtarget * t, const amword * source)
{
    amword * w = new amword;
    w->word = new string;
    w->word->assign(*t->target_underscored->word);
    w->word->append("-");
    w->word->append(replace_suffix(*source->word, "o"));
    return w;
}

amword *
add_srcdir(const amword * src)
{
    amword * w = new amword;
    w->word = new string;
    w->word->assign("$(srcdir)/");
    w->word->append(*src->word);
    return w;
}

bool
is_cc(const amword * w)
{
    size_t pos = w->word->find_last_of(".");
    if (pos == string::npos)
        return false;
    string suffix = w->word->substr(pos+1);
    if (suffix == "cc")
        return true;
    if (suffix == "c++")
        return true;
    if (suffix == "C")
        return true;
    if (suffix == "cpp")
        return true;
    if (suffix == "ll")
        return true;
    if (suffix == "yy")
        return true;
    if (suffix == "L")
        return true;
    if (suffix == "Y")
        return true;
    return false;
}

bool
is_lex(const amword * w)
{
    size_t pos = w->word->find_last_of(".");
    if (pos == string::npos)
        return false;
    if (w->word->at(pos+1) == 'l')
        return true;
    if (w->word->at(pos+1) == 'L')
        return true;
    return false;
}

bool
is_lex_or_yacc(const amword * w)
{
    size_t pos = w->word->find_last_of(".");
    if (pos == string::npos)
        return false;
    if (w->word->at(pos+1) == 'y')
        return true;
    if (w->word->at(pos+1) == 'l')
        return true;
    if (w->word->at(pos+1) == 'Y')
        return true;
    if (w->word->at(pos+1) == 'L')
        return true;
    return false;
}

amword *
make_h(const amword * source)
{
    amword * w;
    size_t pos = source->word->find_last_of(".");
    if (pos == string::npos)
        return NULL;
    w = new amword;
    w->word = new string;
    w->word->assign( *source->word );
    w->word->replace(pos+1, string::npos, "h");
    return w;
}

amword *
make_c_from_ly(const amtarget * t, const amword * source)
{
    amword * w;
    size_t pos = source->word->find_last_of(".");
    if (pos == string::npos)
        return NULL;
    w = new amword;
    w->word = new string;
    w->word->assign( *source->word );
    if (w->word->at(pos+1) == 'y')
    {
        if (w->word->length() == (pos+3))
            w->word->replace(pos+1, string::npos, "cc");
        else
            w->word->replace(pos+1, string::npos, "c");
    }
    if (w->word->at(pos+1) == 'l')
    {
        if (w->word->length() == (pos+3))
            w->word->replace(pos+1, string::npos, "cc");
        else
            w->word->replace(pos+1, string::npos, "c");
    }
    if (w->word->at(pos+1) == 'Y')
        w->word->replace(pos+1, string::npos, "cc");
    if (w->word->at(pos+1) == 'L')
        w->word->replace(pos+1, string::npos, "cc");
    w->word->insert(0,"-");
    w->word->insert(0,*t->target_underscored->word);
    return w;
}
