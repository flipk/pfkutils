/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __fileparser_h__
#define __fileparser_h__ 1

#include <string>
#include <vector>

namespace pfktop {

    class fileParser {
        std::vector<std::string> fields;
        std::string line;
    public:
        fileParser(void);
        ~fileParser(void);
        int parse(const std::string &fname) { return parse(fname.c_str()); }
        int parse(const char * fname);
        int numFields(void) const { return fields.size(); }
        const std::string &get_line(void) { return line; }
        const std::string &operator[](int ind) const { return fields[ind]; }
    };

};

#endif /* __fileparser_h__ */
