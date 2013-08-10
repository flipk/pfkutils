
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "automake.h"
#include "utils.h"

// the enum for install_type should match this in order
static const char * noinstbinlibprefix[3] = { "noinst", "lib", "bin" };

// the enum for target_type should match this in order
static const char * libprogsuffix[2] = { "LIBRARIES", "PROGRAMS" };

static struct amtarget * makeamtarget(struct makefile * mf,
                                  const char *word,
                                  int target_type,
                                  int install_type);
static struct outputrule * makeoutputrule(char * target);
static void addtarget(struct outputrule *rule, char * word);
static void addsource(struct outputrule *rule, char * word);
static void addcommand(struct outputrule *rule, struct command *cmd);
static void adddependency(struct outputrule *rule, struct outputrule *deprule);
static void printoutputrule(struct outputrule *or, int recurse);

// note the struct makefile mf is unusuable after this,
// the pointers are all muffed.
void
automake(struct makefile * mf)
{
    struct variable * v;
    struct word * w;
    struct amtarget * t, * targets = NULL, ** nexttarget = &targets;
    const char * prefix, * suffix;
    int noinstbinlib, libprog;

    for (noinstbinlib = 0; noinstbinlib < 3; noinstbinlib++)
    {
        prefix = noinstbinlibprefix[noinstbinlib];
        for (libprog = 0; libprog < 2; libprog++)
        {
            suffix = libprogsuffix[libprog];
            char search[64];
            sprintf(search, "%s_%s", prefix, suffix);
            v = find_var(mf, search);
            if (v)
            {
                for (w = v->value; w; w = w->next)
                {
                    t = makeamtarget(mf, w->word, libprog, noinstbinlib);
                    if (t)
                    {
                        *nexttarget = t;
                        nexttarget = &t->next;
                    }
                }
            }
        }
    }

    struct variable * globalvars = NULL;
    struct variable ** globalvars_next = &globalvars;

    // find all the global vars in the makefile

#define LOCATE(name) \
    v = find_var(mf, #name); \
    if (v) \
    { \
        struct variable *newv = makevariable(v->variable, v->value); \
        *globalvars_next = newv; \
        globalvars_next = &newv->next; \
    }

    LOCATE(CC);
    LOCATE(CFLAGS);
    LOCATE(CXX);
    LOCATE(CXXFLAGS);
    LOCATE(CPPFLAGS);
    LOCATE(AR);

#undef LOCATE

    // find all the per-target vars, and add their defs
    // to the global list using their per-target var names.

    for (t = targets; t; t = t->next)
    {
        // ldflags, cflags, cxxflags, cppflags

#define LOCATE(field,name) \
        if (t->field) \
        { \
            v = makevariable(t->field->variable, t->field->value); \
            *globalvars_next = v; \
            globalvars_next = &v->next; \
        }

        LOCATE(ldflags,LDFLAGS);
        LOCATE(cflags,CFLAGS);
        LOCATE(cxxflags,CXXFLAGS);
        LOCATE(cppflags,CPPFLAGS);

#undef LOCATE
    }


    // make the 'all' rule which bounces to the xmakefile

    struct outputrule * allrule;
    allrule = makeoutputrule("all");
    addsource(allrule, "xmakefile");
    struct command * cmd;
    cmd = makecommand(makeword("make -f xmakefile"));
    for (t = targets; t; t = t->next)
        addcommandword(cmd, t->target);
    addcommand(allrule, cmd);

    // make a rule for xmakefile and add it to allrule dependencies

    struct outputrule * or;
    or = makeoutputrule("xmakefile");
    addsource(or, "Makefile");



    adddependency(allrule,or);

    // for each target, make a rule and add to allrule dependencies
    // emit target-file.d rule for every source file.[cc/cyl]
    // emit target: target-objects rule and commands
    // emit target-object.o: object.c rules and commands

    // make clean rules
    struct outputrule * cleanrule;
    cleanrule = makeoutputrule("clean");
    cmd = makecommand(makeword("rm -f xmakefile *.d *.o *.a *.so"));
    for (t = targets; t; t = t->next)
        addcommandword(cmd, t->target);
    addcommand(cleanrule, cmd);

    printf("\n");
    printvariable(globalvars,1);
    printf("\n");
    printoutputrule(allrule,1);
    printoutputrule(cleanrule,1);
}

static struct amtarget *
makeamtarget(struct makefile * mf, const char *word,
           int target_type, int install_type)
{
    int namelen = strlen(word) + 1;
    char * targetname = strdup(word);
    char * dot;
    char varname[ 200 ];
    struct variable * v;

    targetname = (char *) malloc(namelen);
    memcpy(targetname, word, namelen);

    struct amtarget * ret;
    ret = (struct amtarget *) malloc(sizeof(struct amtarget));

    ret->next = NULL;

    for (dot = targetname; dot; dot = strchr(dot, '.'))
    {
        if (*dot == '.')
            *dot = '_';
    }

    ret->target = targetname;

#define LOCATE(field,name) \
    snprintf(varname,sizeof(varname),"%s_" #name, targetname); \
    v = find_var(mf, varname); \
    if (v) \
    { \
        if (0) { printf("found "); printvariable(v,0); } \
        ret->field = v; \
    } \
    else \
        ret->field = NULL;

    LOCATE(sources,SOURCES);
    LOCATE(headers,HEADERS);
    LOCATE(includes,INCLUDES);
    LOCATE(ldadd,LDADD);
    LOCATE(ldflags,LDFLAGS);
    LOCATE(cflags,CFLAGS);
    LOCATE(cxxflags,CXXFLAGS);
    LOCATE(cppflags,CPPFLAGS);

#undef LOCATE

    // change '_' back to '.', done with that form.
    memcpy(targetname, word, namelen);

    ret->target_type = target_type;
    ret->install_type = install_type;

    return ret;
}

static struct outputrule *
makeoutputrule(char * target)
{
    struct outputrule * ret;
    ret = (struct outputrule *) malloc(sizeof(struct outputrule));

    ret->next = NULL;
    ret->targets = NULL;
    ret->targets_next = &ret->targets;
    ret->sources = NULL;
    ret->sources_next = &ret->sources;
    ret->commands = NULL;
    ret->commands_next = &ret->commands;
    ret->dependencies = NULL;
    ret->dependencies_next = &ret->dependencies;

    addtarget(ret, target);

    return ret;
}

static void
addtarget(struct outputrule *rule, char * word)
{
    struct word * nw = makeword(word);
    *rule->targets_next = nw;
    rule->targets_next = &nw->next;
}

static void
addsource(struct outputrule *rule, char * word)
{
    struct word * w = makeword(word);
    *rule->sources_next = w;
    rule->sources_next = &w->next;
}

static void
addcommand(struct outputrule *rule, struct command *cmd)
{
    *rule->commands_next = cmd;
    while (cmd)
    {
        rule->commands_next = &cmd->next;
        cmd = cmd->next;
    }
}

static void
adddependency(struct outputrule *rule, struct outputrule *deprule)
{
    *rule->dependencies_next = deprule;
    while (deprule)
    {
        rule->dependencies_next = &deprule->next;
        deprule = deprule->next;
    }
}

static void
printoutputrule(struct outputrule *or, int recurse)
{
    for ( ; or; or = or->next)
    {
        printword(or->targets);
        printf(": ");
        printword(or->sources);
        printf("\n");
        printcommand(or->commands);
        printf("\n");
        if (recurse)
        {
            struct outputrule *ord;
            for (ord = or->dependencies; ord; ord = ord->next)
                printoutputrule(ord, 1);
        }
    }
}
