/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <string>

class SimpleUrl {
    bool _ok;
public:
    std::string url;
    std::string protocol; // http, https, ws, wss
    std::string hostname; // or ip addr
    std::string username; // empty string if not specified
    std::string password; // empty string if not specified
    uint32_t addr; // if hostname could be resolved, or -1 if not
    uint16_t port;
    std::string path;
    std::string anchor; // if #something is present
    std::string query;  // if ?var=value&var=value is present
//SOMEDAY separate query args?
    SimpleUrl(void);
    SimpleUrl(const std::string &url);
    ~SimpleUrl(void);
    bool ok(void) const { return _ok; }
    bool parse(const std::string &url);
};

extern std::ostream &operator<<(std::ostream &str, const SimpleUrl &u);
