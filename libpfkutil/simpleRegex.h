/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <regex.h>
#include <string>
#include <sstream>

template <int _max_matches=25> class pxfe_regex {
public:
    static const int max_matches = _max_matches;
private:
    bool _ok;
    regex_t reg;
    regmatch_t matches[max_matches];
    char errbuf[200];
public:
    pxfe_regex(const char *patt) {
        _ok = false;
        errbuf[0] = 0;
        int cc = regcomp(&reg, patt, REG_EXTENDED);
        if (cc == 0)
            _ok = true;
        else
        {
            regerror(cc, &reg, errbuf, sizeof(errbuf));
            errbuf[sizeof(errbuf)-1] = 0;
        }
    }
    ~pxfe_regex(void) {
        regfree(&reg);
    }
    bool ok(void) const { return _ok; }
    const char * err(void) const { return errbuf; }
    bool exec(const std::string &str) {
        int cc = regexec(&reg, str.c_str(), max_matches, matches, 0);
        if (cc == 0)
            return true;
        regerror(cc, &reg, errbuf, sizeof(errbuf));
        errbuf[sizeof(errbuf)-1] = 0;
        return false;
    }
    bool match(int ind, int *pstart_pos = NULL, int *plen = NULL) const {
        if (ind >= max_matches) return false;
        int s = matches[ind].rm_so;
        if (s == -1) return false;
        if (pstart_pos) *pstart_pos = s;
        if (plen) *plen = matches[ind].rm_eo - s;
        return true;
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
