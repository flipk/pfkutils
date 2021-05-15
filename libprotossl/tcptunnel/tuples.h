
#include <sys/types.h>
#include "simpleRegex.h"

class tuple2_regex : public regex<>
{
    static const char *pattern;
    enum {
        LOCAL_PORT = 2,
        REMOTE_IP = 4,
        REMOTE_IP_PORT = 5,
        REMOTE_HOST = 7,
        REMOTE_HOST_PORT = 11
    };
public:
    tuple2_regex(void);
    ~tuple2_regex(void);
    bool parse_tuple2(const std::string &str);

    bool is_local;
    uint32_t local_port;

    bool remote_is_hostname;
    std::string remote_host;
    uint32_t remote_ip;
    uint32_t remote_port;
};

class tuple3_regex : public regex<>
{
    static const char *pattern;
    enum {
        IP_SYNTAX = 1,
        IP_SYNTAX_LOCAL_PORT = 2,
        IP_SYNTAX_REMOTE_IP = 3,
        IP_SYNTAX_REMOTE_PORT = 4,
        HOSTNAME_SYNTAX = 5,
        HOSTNAME_SYNTAX_LOCAL_PORT = 6,
        HOSTNAME_SYNTAX_REMOTE_HOST = 7,
        HOSTNAME_SYNTAX_REMOTE_PORT = 11
    };
public:
    tuple3_regex(void);
    ~tuple3_regex(void);
    bool parse_tuple3(const std::string &str);

    uint32_t local_port;
    bool remote_is_hostname;
    std::string remote_host;
    uint32_t remote_ip;
    uint32_t remote_port;
};
