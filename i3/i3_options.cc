
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "i3_options.h"

using namespace std;

void
i3_options :: print_help(void)
{
    cerr <<
   "i3 -h\n"
   "   print help\n"
   "i3 [-i file | -Ir | -Iz | -n] [-o file | -O] [-p numpkts] [-v] [-d]\n"
   "   [-P port] [-c mycert] [-k mykey] [-K keypass] [-a cacert] [host]\n"
   "   -i  : read input from file to send\n"
   "   -Ir : generate random data as input\n"
   "   -Iz : generate zero data as input\n"
   "   -n  : no input data (send no data)\n"
   " (if -i, -I, or -n is not present, read from stdin)\n"
   "   -o  : output to file\n"
   "   -O  : discard all output\n"
   " (if -o or -O is not present, write to stdout)\n"
   "   -P  : port# (default is 2005)\n"
   "   -p  : ping/ack to reduce network queuing, specify #pkts to preload\n"
   "   -v  : verbose reporting of stats\n"
   "   -vv : very verbose reporting of stats\n"
   "   -d  : enable ssl debug\n"
   "   -c  : path to my certificate file\n"
   "   -k  : path to my private key file\n"
   "   -K  : password to private key file if needed\n"
   "   -a  : path to root CA certificate file\n"
   " (if hostname not present, listen on port 2005 for inbound connection)\n"
   " (if hostname is present, attempt outputbound connection to host)\n"
        ;
}

i3_options :: i3_options(int argc, char ** argv)
    : ok(false), pingack(false), pingack_preload(0), verbose(false),
      very_verbose(false), debug_flag(0),
      input_set(false), input_fd(-1), input_nul(false),
      input_rand(false), input_zero(false),
      output_set(false), output_fd(-1), output_discard(false),
      outbound(false), port_number(I3_OPTIONS_DEFAULT_PORT)
{
    int ch;
    string home_dir = getenv("HOME");

    ostringstream path;
    path << "file:" << home_dir << "/" << I3_OPTIONS_DEFAULT_CERT_FILE;
    my_cert_path = path.str();

    path.str(""); // reset
    path << "file:" << home_dir << "/" << I3_OPTIONS_DEFAULT_KEY_FILE;
    my_key_path = path.str();

    path.str(""); // reset
    path << "file:" << home_dir << "/" << I3_OPTIONS_DEFAULT_CA_FILE;
    ca_cert_path = path.str();

    while (( ch = getopt( argc, argv, "hi:I:o:Onp:vdc:k:K:a:P:" )) != -1)
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

        case 'p':
            pingack = true;
            pingack_preload = atoi(optarg);
            break;

        case 'v':
            if (verbose)
                very_verbose = true;
            verbose = true;
            break;

        case 'c':
            path.str("");
            path << "file:" << optarg;
            my_cert_path = path.str();
            break;

        case 'k':
            path.str("");
            path << "file:" << optarg;
            my_key_path = path.str();
            break;

        case 'K':
            my_key_password = optarg;
            break;

        case 'a':
            path.str("");
            path << "file:" << optarg;
            ca_cert_path = path.str();
            break;

        case 'd':
            debug_flag ++;
            break;

        case 'P':
            port_number = atoi(optarg);
            break;

        case 'h':
            print_help();
            return;
        }
    }

    if (argc == (optind + 1))
    {
        outbound = true;
        hostname = argv[optind];
    }
    else if (argc != optind)
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
