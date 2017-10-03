struct wordlist {
    struct wordlist * next;
    char * word;
};

struct command {
    struct command * next;
    struct wordlist * command;
};

struct variable {
    struct variable * next;
    char * variable;
    struct wordlist * value;
    int sum; // is it a += ?
};

struct rule {
    struct rule * next;
    struct wordlist * targets;
    struct wordlist * sources;
    struct command * commands;
};

struct makefile {
    struct variable * variables;
    struct variable * variables_tail;
    struct rule * rules;
    struct rule * rules_tail;
};

struct wordlist * makewordlist(char *word);
void printwordlist(struct wordlist *);
struct variable * makevariable(char *word, struct wordlist *value);
void printvariable(struct variable *);
struct command * makecommand(struct wordlist *words);
void printcommand(struct command *command);
struct rule * makerule(struct wordlist *targets,
                       struct wordlist *sources,
                       struct command *commands);
void printrule(struct rule *rule);
void printmakefile(struct makefile *makefile);

struct makefile * parse_makefile(char *fname);

void print_tokenized_file(char *fname);
