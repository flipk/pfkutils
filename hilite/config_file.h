/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <sys/types.h>
#include <regex.h>
#include "color.h"
#include <string>
#include <list>

class Hilite_config {
    regex_t config_file_expr;
    static const int MAX_MATCHES = 16;
    bool parse_line(const std::string &line, int lineno);
    struct pattern {
        pattern(void);
        ~pattern(void);
        int config_lineno;
        std::string regex;
        regex_t expr;
        bool set_expr(int lineno, const std::string &regex);
        Hilite_color color;
        void print(void);
        bool colorize_line(std::string &line, int file_lineno);
    };
    std::list<pattern*> patterns;
public:
    Hilite_config(void);
    ~Hilite_config(void);
    bool parse_file(const std::string &fname);
    void colorize_line(std::string &line, int lineno);
};
