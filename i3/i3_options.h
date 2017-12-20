/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __I3_OPTIONS_H__
#define __I3_OPTIONS_H__

#include <string>

#define I3_OPTIONS_DEFAULT_CERT_FILE ".i3-certs/my-certificate.crt"
#define I3_OPTIONS_DEFAULT_KEY_FILE  ".i3-certs/my-certificate.key"
#define I3_OPTIONS_DEFAULT_CA_FILE   ".i3-certs/Root-CA.crt"
#define I3_OPTIONS_OTHERCOMMONNAME   "i3commonname"
#define I3_OPTIONS_DEFAULT_PORT      2005

class i3_options {
    void print_help(void);
public:
    bool ok;
    bool pingack;
    int pingack_preload;
    bool verbose;
    int debug_flag;

    bool input_set;
    int  input_fd;
    bool input_nul;
    std::string input_file;
    bool input_rand;
    bool input_zero;

    bool output_set;
    int  output_fd;
    std::string output_file;
    bool output_discard;

    std::string my_cert_path;
    std::string my_key_path;
    std::string my_key_password;
    std::string ca_cert_path;

    bool outbound;
    std::string hostname;

    int port_number;

    i3_options(int argc, char ** argv);
};

#endif /* __I3_OPTIONS_H__ */
