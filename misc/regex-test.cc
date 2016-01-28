
// a short program to verify functionality of the regex
// used to parse a command line option in libpfkfb/PageIO

#include <stdio.h>
#include <regex.h>
#include <string>

static const char * path_pattern = "\
^fbserver:([0-9]+.[0-9]+):([0-9]+)$|\
^fbserver:([0-9]+.[0-9]+):([0-9]+):([0-9a-z]+)$|\
^fbserver:([0-9]+.[0-9]+.[0-9]+.[0-9]+):([0-9]+.)$|\
^fbserver:([0-9]+.[0-9]+.[0-9]+.[0-9]+):([0-9]+.):([0-9a-z]+)$|\
^fbserver:(.+):([0-9]+.)$|\
^fbserver:(.+):([0-9]+.):([0-9a-z]+)$|\
^(.+):([0-9a-z]+)$|\
^(.+)$";

#define MATCH_LIST \
    MATCH_ENTRY(ALL) \
    MATCH_ENTRY(FBSRV_IP_2_CT) \
    MATCH_ENTRY(FBSRV_PORT_2_CT) \
    MATCH_ENTRY(FBSRV_IP_2_ENC) \
    MATCH_ENTRY(FBSRV_PORT_2_ENC) \
    MATCH_ENTRY(FBSRV_PORT_2_KEY) \
    MATCH_ENTRY(FBSRV_IP_4_CT) \
    MATCH_ENTRY(FBSRV_PORT_4_CT) \
    MATCH_ENTRY(FBSRV_IP_4_ENC) \
    MATCH_ENTRY(FBSRV_PORT_4_ENC) \
    MATCH_ENTRY(FBSRV_PORT_4_KEY) \
    MATCH_ENTRY(FBSRV_HOSTNAME_CT) \
    MATCH_ENTRY(FBSRV_PORT_HN_CT) \
    MATCH_ENTRY(FBSRV_HOSTNAME_ENC) \
    MATCH_ENTRY(FBSRV_PORT_HN_ENC) \
    MATCH_ENTRY(FBSRV_PORT_HN_KEY) \
    MATCH_ENTRY(FULLPATH_ENC) \
    MATCH_ENTRY(FULLPATH_KEY) \
    MATCH_ENTRY(FULLPATH_CT) \
    MATCH_ENTRY(MAX_MATCHES)

enum {
#define MATCH_ENTRY(x)  MATCH_##x,
    MATCH_LIST
#undef MATCH_ENTRY
};

const char * match_names[] = {
#define MATCH_ENTRY(x)  #x,
    MATCH_LIST
#undef MATCH_ENTRY
};

int
main(int argc, char ** argv)
{
    regex_t expr;
    regmatch_t matches[ MATCH_MAX_MATCHES ];
    int regerr;
    char errbuf[80];
    std::string arg(argv[1]);

    regerr = regcomp( &expr, path_pattern, REG_EXTENDED );
    if (regerr != 0)
    {
        regerror( regerr, &expr, errbuf, sizeof(errbuf) );
        fprintf(stderr,"regcomp error: %s\n", errbuf);
        return -1;
    }

    regerr = regexec( &expr, arg.c_str(), MATCH_MAX_MATCHES, matches, 0 );
    if (regerr != 0)
    {
        regerror( regerr, &expr, errbuf, sizeof(errbuf) );
        fprintf(stderr,"regexec error: %s\n", errbuf);
        return -1;
    }

    regfree( &expr );

    for (int i = 0; i < MATCH_MAX_MATCHES; i++)
    {
        std::string  substr;
        if (matches[i].rm_so != -1)
            substr = arg.substr(matches[i].rm_so,
                                matches[i].rm_eo - matches[i].rm_so);
        printf("entry %2d: %20s so %2d eo %2d   \"%s\"\n",
               i, match_names[i], matches[i].rm_so, matches[i].rm_eo,
               substr.c_str());
    }

    return 0;
}
