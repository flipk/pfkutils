
#ifndef __PARSER_H__
#define __PARSER_H__

struct word {
    struct word * next;
    char * word;
};

struct command {
    struct command * next;
    struct word * command;
    struct word ** command_next;
};

struct variable {
    struct variable * next;
    char * variable;
    struct word * value;
    struct word ** value_next;
    int sum; // is it a += ?
};

struct rule {
    struct rule * next;
    struct word * targets;
    struct word * sources;
    struct command * commands;
};

struct makefile {
    struct variable * variables;
    struct variable ** variables_next;
    struct rule * rules;
    struct rule ** rules_next;
};

struct word * makeword(char *word);
void printword(struct word *);
struct variable * makevariable(char *word, struct word *value);
void addvarword(struct variable *v, struct word *value);
void printvariable(struct variable *, int recurse);
struct command * makecommand(struct word *words);
void addcommandword(struct command *cmd, char * word);
void printcommand(struct command *command);
struct rule * makerule(struct word *targets,
                       struct word *sources,
                       struct command *commands);
void printrule(struct rule *rule);
void printmakefile(struct makefile *makefile);

struct makefile * parse_makefile(char *fname);

void print_tokenized_file(char *fname);

#endif /* __PARSER_H__ */
