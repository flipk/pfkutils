
#ifndef __AUTOMAKE_PARSER_H__
#define __AUTOMAKE_PARSER_H__

#include <string>
#include <dll2.H>
#include <iostream>

struct amword {
    LListLinks<amword> links[1];
    std::string * word;
};

typedef LList<amword,0> amwordList;

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
    amvariable *get_next(amvariable *item) const {
        return list.get_next(item);
    }
};

std::ostream& operator<<(std::ostream& ostr, const amvariables& val);

struct amcommand {
    LListLinks<amcommand> links[1];
    amwordList  cmd;
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
            ldflags = cflags = cxxflags = cppflags = NULL;
    }
    amword * target;
    enum tgttype { TARGET_TYPE_LIB, TARGET_TYPE_PROG } target_type;
    enum insttype { INSTALL_NONE, INSTALL_LIB, INSTALL_BIN } install_type;
    amvariable * sources;
    amvariable * headers;
    amvariable * includes;
    amvariable * ldadd;
    amvariable * ldflags;
    amvariable * cflags;
    amvariable * cxxflags;
    amvariable * cppflags;
};

std::ostream& operator<<(std::ostream& ostr, const amtarget& val);

typedef LList<amtarget,0> amtargetList;

std::ostream& operator<<(std::ostream& ostr, const amtargetList& val);

class automake_file {
    std::string * underscoreize(std::string * str);
    amtarget * make_amtarget(amword * word,
                             amtarget::tgttype target_type,
                             amtarget::insttype install_type);
public:
    automake_file(void);
    ~automake_file(void);
    void tokenize(const char * filename); // debug only
    bool parse(const char * filename);
    void find_targets(void);

    amvariables     input_variables;
    amruleList      input_rules;

    amtargetList    targets;

    amvariables     output_variables;
    amruleList      output_rules;
};

std::ostream& operator<<(std::ostream& ostr, const automake_file& val);

extern automake_file * parser_amf;
void print_tokenized_file(void);
extern int yyparse(void);

#endif /* __AUTOMAKE_PARSER_H__ */
