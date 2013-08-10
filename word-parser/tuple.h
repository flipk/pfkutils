
enum tuple_type { 
    TUPLE_TYPE_WORD,
    TUPLE_TYPE_TUPLE
};

struct tuple {
    struct tuple * next;
    enum tuple_type type;
    char * word;
    union {
        char * word;
        struct tuple * tuples;
    } u;
};

extern struct tuple * tuple_parse(char * input_buf, int len);
extern void print_tuples(struct tuple * t);
