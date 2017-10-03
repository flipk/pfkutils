
#include <stdio.h>
#include <iostream>

#include "automake_parser.H"
#include "tokenizer.H"

using namespace std;

void
automake_file :: make_allrule(void)
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
                c->cmd.add(new amword("$(CXX) $(CXXFLAGS) $(" +
                                      *t->target_underscored->word +
                                      "_CXXFLAGS)"));
            else
                c->cmd.add(new amword("$(CC) $(CFLAGS) $(" +
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
                c->cmd.add(new amword("$(CXX) $(CXXFLAGS)"));
                c->cmd.add(new amword("$(" + *t->target_underscored->word +
                                      "_CXXFLAGS)"));
                c->cmd.add(new amword("-o"));
                c->cmd.add(new amword(t->target));
            }
            else
            {
                c->cmd.add(new amword("$(CC) $(CFLAGS)"));
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
                c->cmd.add(new amword("$(CXX) $(CXXFLAGS)"));
                c->cmd.add(new amword("$(" + *t->target_underscored->word +
                                      "_CXXFLAGS)"));
            }
            else
            {
                c->cmd.add(new amword("$(CC) $(CFLAGS)"));
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
                    c->cmd.add(new amword("$(LEX) " + *src->word));
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
                    c->cmd.add(new amword("$(YACC) -d " + *src->word));
                    r->commands.add(c);
                    c = new amcommand;
                    c->cmd.add(new amword("mv y.tab.c " + *obj->word));
                    r->commands.add(c);
                    c = new amcommand;
                    c->cmd.add(new amword("mv y.tab.h " + *hdr->word));
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
