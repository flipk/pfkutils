
#define MAX_LINE_LEN 10000

struct rule_file {
    struct rule_file * next;
    FILE * file;
    int enable_stdout;
};

extern struct rule_file * rule_files;
extern struct rule_file * rule_files_tail;

typedef enum {
    ACTION_COMPARE_IGNORE,
    ACTION_COMPARE_STORE,
    ACTION_DEFAULT_STORE
} rule_action;

struct rule {
    struct rule *next;
    int rule_id;
    rule_action action;
    regex_t expr;
    struct rule_file * file;
};

extern int next_rule_id;
extern struct rule * rules;
extern struct rule * rules_tail;
