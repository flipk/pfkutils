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
automake_file :: make_allrule(const string &input_filename)
{
    amrule * r = new amrule;
    r->targets.add(new amword("all"));
    r->sources.add(new amword("xmakefile"));
    amcommand * c = new amcommand("+make $(MAKEJOBS) -f xmakefile");
    amtarget * t;
    for (t = targets.get_head(); t; t = targets.get_next(t))
        c->cmd.add(new amword(t->target));
    r->commands.add(c);
    output_rules.add(r);

    r = new amrule;
    r->targets.add(new amword("xmakefile"));
    r->sources.add(new amword("Makefile"));

    for (t = targets.get_head(); t; t = targets.get_next(t))
    {
        string depfile;

        amword * w;
        for (w = t->sources->value.get_head();
             w;
             w = t->sources->value.get_next(w))
        {
            r->sources.add(make_d(t, w));
        }
    }

    r->commands.add(new amcommand("rm -f xmakefile xmakefile-tmp"));
    r->commands.add(new amcommand("cat Makefile > xmakefile-tmp"));
    r->commands.add(new amcommand("echo '' >> xmakefile-tmp"));

    c = new amcommand("cat");

    for (t = targets.get_head(); t; t = targets.get_next(t))
    {
        amword * w;
        for (w = t->sources->value.get_head();
             w;
             w = t->sources->value.get_next(w))
        {
            c->cmd.add(make_d(t, w));
        }
    }

    c->cmd.add(new amword(">> xmakefile-tmp"));
    r->commands.add(c);
    r->commands.add(new amcommand("mv xmakefile-tmp xmakefile"));

    // commands to make xmakefile

    output_rules.add(r);

    string input_filename_fixed = input_filename;

    size_t last_slash_pos =
        input_filename_fixed.find_last_of('/');

    if (last_slash_pos != string::npos)
        input_filename_fixed.erase(0,last_slash_pos+1);

    input_filename_fixed.insert(0, srcdir + "/");

    r = new amrule;

    r->targets.add(new amword("Makefile"));
    r->sources.add(new amword(input_filename_fixed));

    r->commands.add(new amcommand("ampfk " + input_filename_fixed));
    r->commands.add(new amcommand("make clean"));

    output_rules.add(r);
}

void
automake_file :: make_depfilerules(void)
{
    amtarget * t;
    amrule * r;
    for (t = targets.get_head(); t; t = targets.get_next(t))
    {
        amword * w;
        for (w = t->sources->value.get_head();
             w;
             w = t->sources->value.get_next(w))
        {
            r = new amrule;
            amword * depfile = make_d(t, w);
            r->targets.add(depfile);
            if (!is_lex_or_yacc(w))
                r->sources.add(add_srcdir(w));
            else
                r->sources.add(make_c_from_ly(t, w));
            bool is_source_cc = is_cc(w);

            amcommand * c = new amcommand;

            if (is_source_cc)
                c->cmd.add(new amword("$(" +
                                      *t->target_underscored->word +
                                      "_CXX) $(CXXFLAGS) $(" +
                                      *t->target_underscored->word +
                                      "_CXXFLAGS)"));
            else
                c->cmd.add(new amword("$(" +
                                      *t->target_underscored->word +
                                      "_CC) $(CFLAGS) $(" +
                                      *t->target_underscored->word +
                                      "_CFLAGS)"));
            c->cmd.add(new amword("$(CPPFLAGS)"));
            c->cmd.add(new amword("$(" + *t->target_underscored->word +
                                  "_CPPFLAGS)"));
            c->cmd.add(new amword("-MT"));
            c->cmd.add(new amword("'"));
            c->cmd.add(make_o(t, w));
            c->cmd.add(new amword(depfile));
            c->cmd.add(new amword("'"));
            c->cmd.add(new amword("-M -MF"));
            c->cmd.add(new amword(depfile));
            if (!is_lex_or_yacc(w))
                c->cmd.add(add_srcdir(w));
            else
                c->cmd.add(make_c_from_ly(t, w));
            r->commands.add(c);
            output_rules.add(r);
        }
    }
}

void
automake_file :: make_targetlinkrules(void)
{
    amtarget * t;
    amrule * r;
    amword * w;
    amcommand * c;

    for (t = targets.get_head(); t; t = targets.get_next(t))
    {
        bool is_source_cc = false;

        r = new amrule;
        r->targets.add(new amword(t->target));

        for (w = t->objects.get_head(); w; w = t->objects.get_next(w))
            r->sources.add(new amword(w));

        for (w = t->sources->value.get_head(); w;
             w = t->sources->value.get_next(w))
        {
            if (is_cc(w))
                is_source_cc = true;
        }
        r->sources.add(new amword("$(" + *t->target_underscored->word +
                                  "_LDADD)"));

        c = new amcommand;
        c->cmd.add(new amword("rm -f"));
        c->cmd.add(new amword(t->target));
        r->commands.add(c);

        c = new amcommand;
        if (t->target_type == amtarget::TARGET_TYPE_LIB)
        {
            c->cmd.add(new amword("$(AR) cq"));
            c->cmd.add(new amword(t->target));
            c->cmd.add(new amword(t->objects));
            c->cmd.add(new amword("$(" + *t->target_underscored->word +
                                  "_LDADD)"));
            c->cmd.add(new amword("$(" + *t->target_underscored->word +
                                  "_LDFLAGS)"));
        }
        else
        {
            if (is_source_cc)
            {
                c->cmd.add(new amword("$(" +
                                      *t->target_underscored->word +
                                      "_CXX) $(CXXFLAGS)"));
                c->cmd.add(new amword("$(" + *t->target_underscored->word +
                                      "_CXXFLAGS)"));
                c->cmd.add(new amword("-o"));
                c->cmd.add(new amword(t->target));
            }
            else
            {
                c->cmd.add(new amword("$(" +
                                      *t->target_underscored->word +
                                      "_CC) $(CFLAGS)"));
                c->cmd.add(new amword("$(" + *t->target_underscored->word +
                                      "_CFLAGS)"));
                c->cmd.add(new amword("-o"));
                c->cmd.add(new amword(t->target));
            }
            c->cmd.add(new amword(t->objects));
            c->cmd.add(new amword("$(" + *t->target_underscored->word +
                                  "_LDADD)"));
            c->cmd.add(new amword("$(" + *t->target_underscored->word +
                                  "_LDFLAGS)"));
        }
        r->commands.add(c);
                
        output_rules.add(r);
    }

}

void
automake_file :: make_targetobjrules(void)
{
    amtarget * t;
    amword * w;
    amrule * r;
    amcommand * c;
    for (t = targets.get_head(); t; t = targets.get_next(t))
    {
        for (w = t->sources->value.get_head(); w;
             w = t->sources->value.get_next(w))
        {
            r = new amrule;
            amword * obj = make_o(t, w);
            amword * ly = NULL;

            r->targets.add(obj);
            if (is_lex_or_yacc(w))
            {
                ly = make_c_from_ly(t, w);
                r->sources.add(ly);
            }
            else
                r->sources.add(new amword("$(srcdir)/" + *w->word));

            c = new amcommand;
            if (is_cc(w))
            {
                c->cmd.add(new amword("$(" +
                                      *t->target_underscored->word +
                                      "_CXX) $(CXXFLAGS)"));
                c->cmd.add(new amword("$(" + *t->target_underscored->word +
                                      "_CXXFLAGS)"));
            }
            else
            {
                c->cmd.add(new amword("$(" +
                                      *t->target_underscored->word +
                                      "_CC) $(CFLAGS)"));
                c->cmd.add(new amword("$(" + *t->target_underscored->word +
                                      "_CFLAGS)"));
            }
            c->cmd.add(new amword("$(CPPFLAGS)"));
            c->cmd.add(new amword("$(" + *t->target_underscored->word +
                                  "_CPPFLAGS)"));
            c->cmd.add(new amword("-c"));
            if (is_lex_or_yacc(w))
                c->cmd.add(new amword(ly));
            else
                c->cmd.add(new amword("$(srcdir)/" + *w->word));
            c->cmd.add(new amword("-o"));
            c->cmd.add(new amword(obj));

            r->commands.add(c);

            output_rules.add(r);
        }
    }
}

void
automake_file :: make_lexyaccrules(void)
{
    amtarget * t;
    amword * w;
    amrule * r;
    amcommand * c;
    for (t = targets.get_head(); t; t = targets.get_next(t))
    {
        for (w = t->sources->value.get_head(); w;
             w = t->sources->value.get_next(w))
        {
            if (is_lex_or_yacc(w))
            {
                bool is_src_lex = is_lex(w);
                r = new amrule;
                amword * obj = make_c_from_ly(t, w);
                amword * hdr = NULL;
                amword * src = new amword("$(srcdir)/" + *w->word);

                r->targets.add(obj);
                if (!is_src_lex)
                {
                    hdr = make_h(w);
                    r->targets.add(hdr);
                }
                r->sources.add(src);

                if (is_src_lex)
                {
                    c = new amcommand;
                    c->cmd.add(new amword("rm -f " + *obj->word));
                    r->commands.add(c);
                    c = new amcommand;
                    c->cmd.add(new amword("$(LEX)"));
                    c->cmd.add(new amword("$(" +
                                          *t->target_underscored->word +
                                          "_LFLAGS)"));
                    c->cmd.add(new amword(*src->word));
                    r->commands.add(c);
                    c = new amcommand;
                    c->cmd.add(new amword("mv lex.yy.c " + *obj->word));
                    r->commands.add(c);
                }
                else
                {
                    c = new amcommand;
                    c->cmd.add(new amword("rm -f " + *obj->word +
                                          " " + *hdr->word));
                    r->commands.add(c);
                    c = new amcommand;
                    c->cmd.add(new amword("$(YACC) $(" +
                                          *t->target_underscored->word +
                                          "_YFLAGS) -d " + *src->word));
                    r->commands.add(c);

                    // don't have to check return on find_last_of
                    // because i know that src will always have 
                    // both a / and a .
                    size_t slashpos = src->word->find_last_of('/');
                    size_t dotpos = src->word->find_last_of('.');
                    string source_basename = src->word->substr(
                        slashpos+1,
                        dotpos - slashpos - 1);
                    
                    c = new amcommand;

                    if (is_cc(w))
                        c->cmd.add(new amword("mv " + source_basename +
                                              ".tab.cc " + *obj->word));
                    else
                        c->cmd.add(new amword("mv " + source_basename +
                                              ".tab.c " + *obj->word));
                    r->commands.add(c);
                    c = new amcommand;
                    if (is_cc(w))
                        c->cmd.add(new amword("mv " + source_basename +
                                              ".tab.hh " + *hdr->word));
                    else
                        c->cmd.add(new amword("mv " + source_basename +
                                              ".tab.h " + *hdr->word));
                    r->commands.add(c);
                }

                output_rules.add(r);
            }
        }
    }
}

/*

clean:
	rm -f xmakefile *.d *.o targets lexyacc_sources

 */

void
automake_file :: make_cleanrule(void)
{
    amrule * r = new amrule;
    amcommand * c = new amcommand;

    r->targets.add(new amword("clean"));

    c->cmd.add(new amword("rm -f xmakefile *.d *.o"));

    amtarget *t;
    for (t = targets.get_head(); t; t = targets.get_next(t))
    {
        c->cmd.add(new amword(*t->target->word));
        amword * w;
        for (w = t->sources->value.get_head(); w;
             w = t->sources->value.get_next(w))
        {
            if (is_lex_or_yacc(w))
            {
                if (is_lex(w))
                {
                    c->cmd.add(make_c_from_ly(t, w));
                }
                else
                {
                    c->cmd.add(make_c_from_ly(t, w));
                    c->cmd.add(make_h(w));
                }
            }
        }
    }


    r->commands.add(c);

    output_rules.add(r);
}
