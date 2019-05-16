/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <regex.h>
#include <string>
#include <sstream>

template <int max_matches=25> class regex {
    regex_t reg;
    regmatch_t matches[max_matches];
public:
    regex(const char *patt) {
        regcomp(&reg, patt, REG_EXTENDED);
    }
    ~regex(void) {
        regfree(&reg);
    }
    bool exec(const std::string &str) {
        int cc = regexec(&reg, str.c_str(), max_matches, matches, 0);
        return (cc == 0);
    }
    bool match(int ind) const {
        if (ind >= max_matches) return false;
        return matches[ind].rm_so != -1;
    }
    std::string match(const std::string &str, int ind) const {
        if (ind >= max_matches) return "";
        return str.substr(matches[ind].rm_so,
                          matches[ind].rm_eo - matches[ind].rm_so);
    }
    std::string print(const std::string &str) {
        std::ostringstream  ostr;
        for (int ind = 0; ind < max_matches; ind++)
            if (match(ind))
                ostr << " ind " << ind << ": \""
                     << match(str,ind) << "\"" << std::endl;
        return ostr.str();
    }
};
