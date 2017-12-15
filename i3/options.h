/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <string>

#define I3_OPTIONS_DEFAULT_CERT_FILE ".i3-certs/my-certificate.crt"
#define I3_OPTIONS_DEFAULT_KEY_FILE  ".i3-certs/my-certificate.key"
#define I3_OPTIONS_DEFAULT_CA_FILE   ".i3-certs/Root-CA.crt"
#define I3_OPTIONS_OTHERCOMMONNAME   "i3commonname"

class i3_options {
    void print_help(void);
public:
    bool ok;
    bool pingack;
    bool verbose;
    bool debug_flag;

    bool input_set;
    bool input_nul;
    std::string input_file;
    bool input_rand;
    bool input_zero;

    bool output_set;
    std::string output_file;
    bool output_discard;

    std::string my_cert_path;
    std::string my_key_path;
    std::string my_key_password;
    std::string ca_cert_path;

    i3_options(int argc, char ** argv);
};
