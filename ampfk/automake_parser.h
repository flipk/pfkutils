/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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

#ifndef __AUTOMAKE_PARSER_H__
#define __AUTOMAKE_PARSER_H__

#include <string>
#include <dll2.h>
#include <iostream>

struct amword;
typedef LList<amword,0> amwordList;

struct amword {
    LListLinks<amword> links[1];
    std::string * word;
    amword(void) { word = NULL; }
    amword(const amword *w) {
        word = new std::string;
        word->assign(*w->word);
    }
    amword(const std::string &str) {
        word = new std::string;
        word->assign(str);
    }
    amword(const amwordList &list) {
        word = new std::string;
        for (amword * w = list.get_head(); w; w = list.get_next(w))
        {
            word->append(" ");
            word->append(*w->word);
        }
    }
    ~amword(void) { if (word) delete word; }
};


std::ostream& operator<<(std::ostream& ostr, const amwordList& val);

enum {
    AMVAR_LIST,
    AMVAR_HASH,
    AMVAR_NUMLISTS
};

struct amvariable {
    LListLinks<amvariable> links[AMVAR_NUMLISTS];
    std::string * var;
    amwordList  value;
    bool sum;
    amvariable(void) { sum = false; }
    amvariable(amvariable * v) {
        var = v->var;
        for (amword * w = v->value.get_head();
             w;
             w = v->value.get_next(w))
        {
            value.add(new amword(w));
        }
    }
    ~amvariable(void) { 
        amword * w;
        while ((w = value.dequeue_head()) != NULL)
            delete w;
    }
    void appendfrom(amvariable* rhs) {
        amword * w;
        while ((w = rhs->value.dequeue_head()) != NULL)
            value.add(w);
    }
    void appendfrom(amwordList* rhs) {
        amword * w;
        while ((w = rhs->dequeue_head()) != NULL)
            value.add(w);
    }
};

std::ostream& operator<<(std::ostream& ostr, const amvariable& val);

class amvariableHashComparator {
public:
    static int hash_key( amvariable * item ) {
        return hash_key( *item->var );
    }
    static int hash_key( const std::string &key ) {
        unsigned int ret = 5381; // djb2 string hash algorithm
        for (unsigned int ind = 0; ind < key.size(); ind++)
            ret = ((ret << 5) + ret) + key[ind];  /* hash * 33 + c */
        return (int) ret;
    }
    static bool hash_key_compare( amvariable * item, const std::string &key ) {
        return (*item->var == key);
    }
};

typedef LList<amvariable,AMVAR_LIST> amvariableList;
typedef LListHash<amvariable,std::string,
                  amvariableHashComparator,AMVAR_HASH> amvariableHash;

class amvariables {
    amvariableList list;
    amvariableHash hash;
public:
    void add(amvariable *v) {
        list.add(v);
        hash.add(v);
    }
    amvariable *find(const std::string &key) const {
        return hash.find(key);
    }
    amvariable *get_head(void) const {
        return list.get_head();
    }
    amvariable *dequeue_head(void) {
        amvariable * item = list.dequeue_head();
        if (item) hash.remove(item);
        return item;
    }
    void remove(amvariable *v) {
        list.remove(v);
        hash.remove(v);
    }
    amvariable *get_next(amvariable *item) const {
        return list.get_next(item);
    }
};

std::ostream& operator<<(std::ostream& ostr, const amvariables& val);

struct amcommand {
    LListLinks<amcommand> links[1];
    amwordList  cmd;
    amcommand(void) { }
    amcommand(const std::string &str) { cmd.add(new amword(str)); }
    ~amcommand(void) {
        amword * w;
        while ((w = cmd.dequeue_head()) != NULL)
            delete w;
    }
    void appendfrom(amwordList *rhs) {
        amword * w;
        while ((w = rhs->dequeue_head()) != NULL)
            cmd.add(w);
    }
};

std::ostream& operator<<(std::ostream& ostr, const amcommand& val);

typedef LList<amcommand,0> amcommandList;

std::ostream& operator<<(std::ostream& ostr, const amcommandList& val);

struct amrule {
    LListLinks<amrule> links[1];
    amwordList targets;
    amwordList sources;
    amcommandList commands;
    ~amrule(void) {
        amword * w;
        while ((w = targets.dequeue_head()) != NULL)
            delete w;
        while ((w = sources.dequeue_head()) != NULL)
            delete w;
        amcommand * c;
        while ((c = commands.dequeue_head()) != NULL)
            delete c;
    }
    void appendtargetsfrom(amwordList* rhs) {
        amword * w;
        while ((w = rhs->dequeue_head()) != NULL)
            targets.add(w);
    }
    void appendsourcesfrom(amwordList* rhs) {
        amword * w;
        while ((w = rhs->dequeue_head()) != NULL)
            sources.add(w);
    }
    void appendcommandsfrom(amcommandList* rhs) {
        amcommand * c;
        while ((c = rhs->dequeue_head()) != NULL)
            commands.add(c);
    }
};

std::ostream& operator<<(std::ostream& ostr, const amrule& val);

typedef LList<amrule,0> amruleList;

std::ostream& operator<<(std::ostream& ostr, const amruleList& val);

struct amtarget {
    LListLinks<amtarget> links[1];
    amtarget(void) {
        target = NULL;
        sources = headers = includes = ldadd =
            ldflags = cflags = cxxflags = cppflags = 
            lflags = yflags = cc = cxx = NULL;
    }
    ~amtarget(void) {
        amword * w;
        while ((w = objects.dequeue_head()) != NULL)
            delete w;
    }
    amword * target;
    amword * target_underscored;
    enum tgttype { TARGET_TYPE_LIB, TARGET_TYPE_PROG } target_type;
    enum insttype { INSTALL_NONE, INSTALL_LIB, INSTALL_BIN } install_type;
    amvariable * cc;
    amvariable * cxx;
    amvariable * sources;
    amvariable * headers;
    amwordList   objects;
    amvariable * includes;
    amvariable * ldadd;
    amvariable * ldflags;
    amvariable * cflags;
    amvariable * cxxflags;
    amvariable * cppflags;
    amvariable * lflags;
    amvariable * yflags;
};

std::ostream& operator<<(std::ostream& ostr, const amtarget& val);

typedef LList<amtarget,0> amtargetList;

std::ostream& operator<<(std::ostream& ostr, const amtargetList& val);

class automake_file {
    std::string * underscoreize(std::string * str);
    amtarget * make_amtarget(amword * word,
                             amtarget::tgttype target_type,
                             amtarget::insttype install_type);

    void find_targets(void);
    void make_variables(void);
    void make_allrule(const std::string &input_filename);
    void make_depfilerules(void);
    void make_targetlinkrules(void);
    void make_targetobjrules(void);
    void make_lexyaccrules(void);
    void make_cleanrule(void);

    std::string     srcdir;
    std::string     builddir;

    amtargetList    targets;

    amvariables     output_variables;
    amruleList      output_rules;

public:
    automake_file(void);
    ~automake_file(void);
    void tokenize(const std::string &filename); // debug only
    bool parse(const std::string &filename);
    void output_makefile(const std::string &input_filename,
                         const std::string &output_filename);

    amvariables     input_variables;
    amruleList      input_rules;

};

// utilities
std::string replace_suffix(const std::string &file, const std::string &suffix);
amword * make_d(const amtarget * t, const amword * source);
amword * make_o(const amtarget * t, const amword * source);
amword * make_h(const amword * source);
amword * make_c_from_ly(const amtarget * t, const amword * source);
amword * add_srcdir(const amword * src);
bool is_cc(const amword * w); // also returns true for L and Y
bool is_lex(const amword * w);
bool is_lex_or_yacc(const amword * w);

extern automake_file * parser_amf;
void print_tokenized_file(void);
extern int yyparse(void);

#endif /* __AUTOMAKE_PARSER_H__ */
