
#include <stdio.h>
#include <string>

struct csvfield {
    csvfield * next;
    std::string word;
    csvfield(void) {
        next = NULL;
    }
    ~csvfield(void) {
        if (next)
            delete next;
    }
    void append(std::string _w) {
        word.append(_w);
    }
};

struct csvline {
    csvline *next;
    csvfield *fields;
    csvline(csvfield *_f) {
        next = NULL;
        fields = _f;
    }
    ~csvline(void) {
        if (next)
            delete next;
        if (fields)
            delete fields;
    }
};

struct csvfile {
    csvline *lines;
    csvline **next;
    csvfile(void) {
        lines = NULL;
        next = &lines;
    }
    void add_line(csvline *l) {
        *next = l;
        next = &l->next;
    }
};

csvline * parse_file(FILE *in);
