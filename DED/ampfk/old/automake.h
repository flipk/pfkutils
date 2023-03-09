
#ifndef __AUTOMAKE_H__
#define __AUTOMAKE_H__

#include "parser.h"

struct amtarget {
    struct amtarget * next;
    char * target;
    enum { TARGET_TYPE_LIB, TARGET_TYPE_PROG } target_type;
    enum { INSTALL_NONE, INSTALL_LIB, INSTALL_BIN } install_type;
    struct variable * sources;
    struct variable * headers;
    struct variable * includes;
    struct variable * ldadd;
    struct variable * ldflags;
    struct variable * cflags;
    struct variable * cxxflags;
    struct variable * cppflags;
};

struct outputrule {
    struct outputrule * next;
    struct word * targets;
    struct word ** targets_next;
    struct word * sources;
    struct word ** sources_next;
    struct command * commands;
    struct command ** commands_next;
    struct outputrule * dependencies;
    struct outputrule ** dependencies_next;
};

void automake(struct makefile * mf);

#endif /* __AUTOMAKE_H__ */
