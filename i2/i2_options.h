/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __I2_OPTIONS_H__
#define __I2_OPTIONS_H__

#include <string>

class i2_options {
    void print_help(void);
public:
    bool ok;
    bool stats_at_end;
    bool verbose;
    bool debug_flag;

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

    bool outbound;
    std::string hostname;
    int port_number;

    i2_options(int argc, char ** argv);
    ~i2_options(void);
    void print(void);
};

#endif /* __I2_OPTIONS_H__ */
