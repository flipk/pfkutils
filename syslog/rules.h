
#define MAX_LINE_LEN 10000

struct rule_file {
    struct rule_file * next;
    FILE * file;
    int enable_stdout;
};

struct rule_file * rule_files;
struct rule_file * rule_files_tail;

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

int next_rule_id = 1;
struct rule * rules;
struct rule * rules_tail;
