
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "i2_options.h"

using namespace std;

void
i2_options :: print_help(void)
{
    cerr <<
"i2 [-svnd] [-I[r|z]] [-i file] [-O] [-o file] [-z[r|t]] port\n"
"i2 [-svnd] [-I[r|z]] [-i file] [-O] [-o file] [-z[r|t]] host port\n"
"    -i filename: redirect fd 0 to input file\n"
"    -o filename: redirect fd 1 to output file\n"
/*
"   -Ir: input is random data (implies -n)\n"
"   -Iz: input is zero data (implies -n)\n"
*/
"    -O: discard output data\n"
"    -n: do not read from stdin\n"
"    -s: display stats of transfer at end\n"
"    -v: verbose stats during transfer (0.5 second updates), implies -s\n"
/*
"   -zr: uncompress any data received from network\n"
"   -zt: compress any data transmitted to network\n"
"i2 [-d] -f port host port [port host port...]\n"
"    -d: debug mode\n"
"    -D: log data transferred\n"
"    -f: forward local port to remote host/port\n"
*/
        ;
}

i2_options :: i2_options(int argc, char ** argv)
    : ok(false), stats_at_end(false), verbose(false),
      input_set(false), input_fd(-1), input_nul(false),
      input_rand(false), input_zero(false),
      output_set(false), output_fd(-1), output_discard(false),
      outbound(false), port_number(-1)
{
    int ch;

    while (( ch = getopt( argc, argv, "svndI:i:Oo:z:" )) != -1)
    {
        switch (ch)
        {
        case 'i':
            if (input_nul || input_rand || input_zero || input_set)
            {
            minus_i_exclusive:
                cerr << "-i and -n and -Ir and -Iz are exclusive\n";
                return;
            }
            input_set = true;
            input_file = optarg;

            input_fd = open(input_file.c_str(), O_RDONLY);
            if (input_fd < 0)
            {
                int e = errno;
                char * err = strerror(e);
                cerr << "open input file '" << input_file
                     << "' failed with error " << e
                     << "(" << err << ")\n";
                return;
            }

            break;

        case 'I':
            if (input_nul || input_rand || input_zero || input_set)
                goto minus_i_exclusive;
            if (optarg[0] == 'z')
                input_zero = true;
            else if (optarg[0] == 'r')
                input_rand = true;
            else
            {
                cerr << "-I should be followed by r or z\n";
                return;
            }
            break;

        case 'n':
            if (input_nul || input_rand || input_zero || input_set)
                goto minus_i_exclusive;
            input_nul = true;
            break;

        case 'o':
            if (output_set || output_discard)
            {
            output_exclusive:
                cerr << "-o and -O are exclusive\n";
                return;
            }
            output_set = true;
            output_file = optarg;
            output_fd = open(output_file.c_str(),
                             O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (output_fd < 0)
            {
                int e = errno;
                char * err = strerror(e);
                cerr << "open output file '" << output_file
                     << "' failed with error " << e
                     << "(" << err << ")\n";
                return;
            }
            break;

        case 'O':
            if (output_set || output_discard)
                goto output_exclusive;
            output_discard = true;
            break;

        case 'v':
            verbose = true;
            break;

        case 's':
            stats_at_end = true;
            break;

        case 'z':
            // xxx
            printf("Z not yet supported\n");
            break;
        }
    }

    if (argc == (optind + 1))
    {
        outbound = false;
        port_number = atoi(argv[optind]);
    }
    else if (argc == (optind + 2))
    {
        outbound = true;
        hostname = argv[optind];
        port_number = atoi(argv[optind+1]);
    }
    else
    {
        cerr << "unknown arguments" << endl;
        print_help();
        return;
    }

    if (input_nul == false && input_rand == false &&
        input_zero == false && input_set == false)
    {
        input_set = true;
        input_fd = 0;
    }

    if (output_set == false && output_discard == false)
    {
        output_set = true;
        output_fd = 1;
    }

    ok = true;
}

i2_options :: ~i2_options(void)
{
    if (input_fd > 0)
        close(input_fd);
    if (output_fd > 0)
        close(output_fd);
}

void
i2_options :: print(void)
{
    printf("ok = %d\n"
           "stats_at_end = %d\n"
           "verbose = %d\n",
           ok, stats_at_end, verbose);
    if (input_set)
        printf("input_set, fd = %d, file = '%s'\n",
               input_fd, input_file.c_str());
    if (input_nul)
        printf("input_nul\n");
    if (input_rand)
        printf("input_rand\n");
    if (input_zero)
        printf("input_zero\n");
    if (output_set)
        printf("output_set, fd = %d, file = '%s'\n",
               output_fd, output_file.c_str());
    if (output_discard)
        printf("output_discard\n");
    if (outbound)
        printf("outbound : host = '%s' port %d\n",
               hostname.c_str(), port_number);
    else
        printf("inbound : port %d\n", port_number);
}
