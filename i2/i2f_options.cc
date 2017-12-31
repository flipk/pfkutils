
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

#include "i2f_options.h"
#include "posix_fe.h"

using namespace std;

void
i2f_options :: print_help(void)
{
    cerr <<
"i2f [-svd] port host port [port host port...]\n"
"    -s: display stats of transfer at end\n"
"    -v: verbose stats during transfer (0.5 second updates), implies -s\n"
"    -d: debug mode\n"
        ;
}

i2f_options :: i2f_options(int argc, char ** argv)
    : ok(false), stats_at_end(false), verbose(false), debug_flag(0)
{
    int ch;

    while (( ch = getopt( argc, argv, "svd" )) != -1)
    {
        switch (ch)
        {
        case 's':
            stats_at_end = true;
            break;

        case 'v':
            verbose = true;
            break;

        case 'd':
            debug_flag ++;
            break;

        default:
            print_help();
            return;
        }
    }

    int diff = argc - optind;
    if (((diff % 3) != 0) || (diff == 0))
    {
        fprintf(stderr,"wrong number of args, need [port host port] tuples\n");
        print_help();
        return;
    }
    for (int ind = optind; ind < argc; ind += 3)
    {
        forw_port * p = new forw_port;
        bool good = true;
        if (pxfe_iputils::parse_port_number(argv[ind],
                                            &p->port) == false)
        {
            good = false;
        }
        p->remote_host = argv[ind+1];
        if (pxfe_iputils::hostname_to_ipaddr(argv[ind+1],
                                             &p->remote_addr) == false)
        {
            good = false;
        }
        if (pxfe_iputils::parse_port_number(argv[ind+2],
                                            &p->remote_port) == false)
        {
            good = false;
        }
        if (good)
            ports.push_back(p);
        else
        {
            delete p;
            return;
        }
    }

    ok = true;
}

i2f_options :: ~i2f_options(void)
{
    ports_iter_t it;
    for (it = ports.begin(); it != ports.end();)
    {
        forw_port * p = *it;
        it = ports.erase(it);
        delete p;
    }
}

void
i2f_options :: print(void)
{
    fprintf(stderr,"ok = %d\n"
           "stats_at_end = %d\n"
           "verbose = %d\n"
           "debug_flag = %d\n",
           ok, stats_at_end, verbose, debug_flag);

    ports_iter_t it;
    for (it = ports.begin(); it != ports.end(); it++)
    {
        forw_port * p = *it;
        fprintf(stderr,"%d:%s:%d\n",
               p->port, p->remote_host.c_str(), p->remote_port);
    }
}
