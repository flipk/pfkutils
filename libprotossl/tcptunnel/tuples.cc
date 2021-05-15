
#include "tuples.h"
#include "posix_fe.h"

tuple2_regex :: tuple2_regex(void)
    : regex(pattern)
{
}

tuple2_regex :: ~tuple2_regex(void)
{
}

bool
tuple2_regex :: parse_tuple2(const std::string &str)
{
    if (exec(str) == false)
        return false;
//        printf("parse debug:\n%s\n", print(str).c_str());
    if (match(LOCAL_PORT))
    {
        is_local = true;
        remote_is_hostname = false;
        pxfe_utils::parse_number(
            match(str,LOCAL_PORT),
            &local_port);
//            printf("local port %u\n", local_port);
        return true;
    }
    else
    {
        is_local = false;
        if (match(REMOTE_IP))
        {
            remote_is_hostname = false;
            pxfe_iputils::hostname_to_ipaddr(
                match(str,REMOTE_IP),
                &remote_ip);
            pxfe_utils::parse_number(
                match(str,REMOTE_IP_PORT),
                &remote_port);
//                printf("remote ip %08x remote port %u\n",
//                       remote_ip, remote_port);
            return true;
        }
        else
        {
            remote_is_hostname = true;
            if (match(REMOTE_HOST))
            {
                remote_host = match(str,REMOTE_HOST);
                if (pxfe_iputils::hostname_to_ipaddr(
                        remote_host, &remote_ip) == false)
                    return false;
                pxfe_utils::parse_number(
                    match(str,REMOTE_HOST_PORT),
                    &remote_port);
//                    printf("remote host '%s' remote ip %08x "
//                           "remote port %u\n",
//                           remote_host.c_str(), remote_ip,
//                           remote_port);
                return true;
            }
        }
    }
    return false;
}

const char *tuple2_regex :: pattern =
"^"
   "("
      "([1-9][0-9]+)"                               // 2 = local_port
   ")"
"|"
   "("
      "([1-9][0-9]*.[0-9]+.[0-9]+.[0-9][1-9]*)"     // 4 = remote_ip
      ":"
      "([1-9][0-9]+)"                               // 5 = remote_port
   ")"
"|"
   "("
      "("                                           // 7 = remote_host
// regex for RFC <somethingorother> internet hostname valid pattern.
         "([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]"
            "{0,61}"
            "[a-zA-Z0-9])"
            "(\\.([a-zA-Z0-9]|"
            "[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}"
            "[a-zA-Z0-9]))*"                        // 8,9,10
      ")"
      ":"
      "([1-9][0-9]+)"                               // 11 = remote_port
   ")"
"$"
;




tuple3_regex :: tuple3_regex(void)
    : regex(pattern)
{
}

tuple3_regex :: ~tuple3_regex(void)
{
}

bool
tuple3_regex :: parse_tuple3(const std::string &str)
{
    if (exec(str) == false)
        return false;
//        printf("parse debug:\n%s\n", print(str).c_str());
    if (match(IP_SYNTAX))
    {
        pxfe_utils::parse_number(
            match(str,IP_SYNTAX_LOCAL_PORT),
            &local_port);
        pxfe_iputils::hostname_to_ipaddr(
            match(str,IP_SYNTAX_REMOTE_IP),
            &remote_ip);
        pxfe_utils::parse_number(
            match(str,IP_SYNTAX_REMOTE_PORT),
            &remote_port);
        remote_is_hostname = false;
//            printf("local port %u remote ip %08x remote port %u\n",
//                   local_port, remote_ip, remote_port);
        return true;
    }
    else if (match(HOSTNAME_SYNTAX))
    {
        pxfe_utils::parse_number(
            match(str,HOSTNAME_SYNTAX_LOCAL_PORT),
            &local_port);
        remote_host = match(str,HOSTNAME_SYNTAX_REMOTE_HOST);
        pxfe_utils::parse_number(
            match(str,HOSTNAME_SYNTAX_REMOTE_PORT),
            &remote_port);
        if (pxfe_iputils::hostname_to_ipaddr(
                remote_host, &remote_ip) == false)
            return false;
        remote_is_hostname = true;
//            printf("local port %u remote host '%s' remote ip %08x "
//                   "remote port %u\n",
//                   local_port, remote_host.c_str(),
//                   remote_ip, remote_port);
        return true;
    }
    return false;
}

const char * tuple3_regex :: pattern =
"^"
   "("                                              // 1 = using ip syntax
      "([1-9][0-9]+)"                               // 2 = local_port
      ":"
      "([1-9][0-9]*.[0-9]+.[0-9]+.[0-9][1-9]*)"     // 3 = remote_ip
      ":"
      "([1-9][0-9]+)"                               // 4 = remote_port
   ")"
"|"
   "("                                              // 5 = using hostname
      "([1-9][0-9]+)"                               // 6 = local_port
      ":"
      "("                                           // 7 = remote_host
// regex for RFC <somethingorother> internet hostname valid pattern.
         "([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]"
            "{0,61}"
            "[a-zA-Z0-9])"
            "(\\.([a-zA-Z0-9]|"
            "[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}"
            "[a-zA-Z0-9]))*"                        // 8,9,10
      ")"
      ":"
      "([1-9][0-9]+)"                               // 11 = remote_port
   ")"
"$"
;
