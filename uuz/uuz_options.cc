
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <mbedtls/sha256.h>
#include <iostream>
#include <iomanip>

#include "uuz_options.h"
#include "posix_fe.h"

namespace PFK_uuz {

uuzopts :: uuzopts(int argc, char ** argv)
{
    _ok = false;
    mode = NONE;
    debug = 0;
    text_headers = false;
    compression = compression_default;
    data_block_variant = data_block_variant_default;
    output_uuz = "-";
    uuz_f = stdout;
    file_splitting_enabled = false;
    outfile_split_size = 0;
    outfile_counter = 1;
    encryption = PFK::uuz::NO_ENCRYPTION;
    hmac = PFK::uuz::NO_HMAC;

    if (argc < 2)
        return;
    std::string  arg1 = argv[1];
    if (arg1 == "e")
        mode = ENCODE;
    else if (arg1 == "d")
        mode = DECODE;
    else if (arg1 == "t")
        mode = LIST;
    else
        return;
    argc -= 1;
    argv += 1;
    char ch;
    while (( ch = getopt(argc, argv, "c:td:e:o:m:v:")) != -1)
    {
        switch (ch)
        {
        // the following opts are allowed only in encoding.
        case 'c':
            if (mode != ENCODE)
            {
                fprintf(stderr,
                        "ERROR: compression setting valid only when encoding\n"
                        "(decoder gets it from VERSION block)\n");
                return;
            }
            if (strcmp(optarg, "none") == 0)
                compression = PFK::uuz::NO_COMPRESSION;
            else if (strcmp(optarg, "libz") == 0)
                compression = PFK::uuz::LIBZ_COMPRESSION;
            else
            {
                fprintf(stderr, "ERROR: unknown compression alg '%s'\n",
                        optarg);
                return;
            }
            break;
        case 'v':
        {
            if (mode != ENCODE)
            {
                fprintf(stderr,
                        "ERROR: variant setting valid only when encoding\n"
                        "(decoder gets it from VERSION block)\n");
                return;
            }
            std::string _optarg = optarg;
            uint32_t  optval;
            if (!pxfe_utils::parse_number(_optarg, &optval))
            {
                fprintf(stderr, "ERROR: specify variant as number\n");
                return;
            }
            if (Base64::variant_name((Base64Variant) optval) == NULL)
            {
                fprintf(stderr, "ERROR: invalid variant# specified\n");
                return;
            }
            data_block_variant = (Base64Variant) optval;
            break;
        }
        case 't':
            if (mode != ENCODE)
            {
                fprintf(stderr,
                        "ERROR: text header setting valid "
                        "only when encoding\n");
                return;
            }
            text_headers = true;
            break;
        case 'o':
        {
            if (mode != ENCODE)
            {
                fprintf(stderr,
                        "ERROR: output file setting valid "
                        "only when encoding\n");
                return;
            }
            output_uuz = optarg;
            break;
        }
        case 'm':
        {
            if (mode != ENCODE)
            {
                fprintf(stderr,
                        "ERROR: file splitting setting valid "
                        "only when encoding\n");
                return;
            }
            std::string _optarg = optarg;
            int32_t optval;
            if (!pxfe_utils::parse_number(_optarg, &optval) ||
                optval < 100)
            {
                fprintf(stderr, "ERROR: invalid number for max size\n");
                return;
            }
            file_splitting_enabled = true;
            outfile_split_size = (size_t) optval;
            break;
        }

        // below this line, these opts are allowed in both
        // encode and decode mode.
        case 'd':
        {
            std::string _optarg = optarg;
            uint32_t optval;
            if (!pxfe_utils::parse_number(_optarg, &optval))
            {
                fprintf(stderr, "ERROR: debug FLAG should be a number\n");
                return;
            }
            debug = optval;
            break;
        }
        case 'e':
        {
            encryption_key.resize(32);
            mbedtls_sha256_ret( (const unsigned char*) optarg,
                                strlen(optarg),
                                (unsigned char *) encryption_key.c_str(),
                                /*is224*/ 0 );
            encryption = PFK::uuz::AES256_ENCRYPTION;
            // hmac is turned on automatically when encryption is on.
            hmac = PFK::uuz::HMAC_SHA256HMAC;
            break;
        }
        case '?':
            fprintf(stderr, "ERROR: unknown option! quitting\n");
            return;
        }
    }
    // i messed with argc and argv above, so this is all
    // now off by 1. forgive me.
    if (mode == ENCODE)
    {
        // open the output file.
        if (output_uuz != "-")
        {
            if (!file_splitting_enabled)
            {
                uuz_f = fopen(output_uuz.c_str(), "w");
                if (!uuz_f)
                {
                    int e = errno;
                    fprintf(stderr,
                            "ERROR: can't open output file %s: %d (%s)\n",
                            output_uuz.c_str(), e, strerror(e));
                    return;
                }
            }
            else
            {
                std::ostringstream  fname;
                fname << output_uuz << "."
                      << std::setfill('0') << std::setw(3)
                      << outfile_counter;
                outfile_counter ++;
                uuz_f = fopen(fname.str().c_str(), "w");
                if (!uuz_f)
                {
                    int e = errno;
                    fprintf(stderr,
                            "ERROR: can't open output file %s: %d (%s)\n",
                            output_uuz.c_str(), e, strerror(e));
                    return;
                }
            }
        }

        // open all input files.
        bool die = false;
        for (int ind = optind; ind != argc; ind++)
        {
            char * fn = argv[ind];
            input_file * inf = new input_file;
            inf->path = fn;

            while (inf->path[0] == '/')
            {
                fprintf(stderr, "WARNING: stripping leading '/' "
                        "from pathnames\n");
                inf->path.erase(0,1);
            }

            {
                std::vector<std::string>  components;
                splitString(components, inf->path);

                for (auto &s : components)
                {
                    if (s == "." || s == "..")
                    {
                        fprintf(stderr, "ERROR: path components "
                                "'.' and '..' "
                                "are not allowed\n");
                        return;
                    }
                }
            }

            if (stat(fn, &inf->sb) < 0)
            {
                int e = errno;
                fprintf(stderr, "stat %s: %d (%s)\n",
                        fn, e, strerror(e));
                die = true;
            }
            if (!die && inf->sb.st_size == 0)
            {
                fprintf(stderr, "file '%s' is empty?\n", fn);
                die = true;
            }
            if (!die)
            {
                if (inf->sb.st_mode & S_IFDIR)
                {
                    fprintf(stderr, "ERROR: recursing into subdirs not "
                            "supported\n");
                    return;
                }
                inf->fd = open(fn, O_RDONLY);
                if (inf->fd < 0)
                {
                    int e = errno;
                    fprintf(stderr, "open %s: %d (%s)\n",
                            fn, e, strerror(e));
                    die = true;
                }
            }
            if (!die)
            {
                inf->f = fdopen(inf->fd, "r");
                input_files.push_back(inf);
            }
            else
                delete inf;
        }
        if (die)
            return;
    }
    else if (mode == DECODE || mode == LIST)
    {
        input_file * inf = new input_file;
        if (argc == optind+1)
        {
            inf->path = argv[optind];
        }
        else
        {
            fprintf(stderr, "ERROR: must provide one input file name\n");
            return;
        }
        if (inf->path == "-")
        {
            inf->fd = 0; // stdin
            memset(&inf->sb, 0, sizeof(struct stat));
        }
        else
        {
            if (stat(inf->path.c_str(), &inf->sb) < 0)
            {
                int e = errno;
                fprintf(stderr, "ERROR: input file %s: %d (%s)\n",
                        inf->path.c_str(), e, strerror(e));
                return;
            }
            if (inf->sb.st_size == 0)
            {
                fprintf(stderr, "ERROR: input file is empty?\n");
                return;
            }

            inf->fd = open(inf->path.c_str(), O_RDONLY);
            if (inf->fd < 0)
            {
                int e = errno;
                fprintf(stderr, "ERROR: input file %s: %d (%s)\n",
                        inf->path.c_str(), e, strerror(e));
                return;
            }
        }
        inf->f = fdopen(inf->fd, "r");
        input_files.push_back(inf);
    }
    if (input_files.size() == 0)
    {
        fprintf(stderr, "ERROR: you must specify at least 1 input file\n");
        return;
    }
    current_file = input_files[0];
    _ok = true;
}

uuzopts :: ~uuzopts(void)
{
    for (auto inf : input_files)
    {
        if (inf->f)
        {
            fclose(inf->f);
        }
        delete inf;
    }
    if (uuz_f != stdout)
        fclose(uuz_f);
}

void uuzopts :: check_rollover(void)
{
    if (file_splitting_enabled == false)
        return;

    if (ftello(uuz_f) < outfile_split_size)
    {
        return;
    }

    fclose(uuz_f);
    std::ostringstream  fname;
    fname << output_uuz << "."
          << std::setfill('0') << std::setw(3)
          << outfile_counter;
    outfile_counter ++;
    uuz_f = fopen(fname.str().c_str(), "w");
    if (!uuz_f)
    {
        int e = errno;
        fprintf(stderr,
                "ERROR: can't open output file %s: %d (%s)\n",
                output_uuz.c_str(), e, strerror(e));
        return;
    }
}

void uuzopts :: usage(void)
{
    fprintf(stderr,
            "usage:\n"
            "    uuz e [encode options] file [file...]\n"
            "    uuz t [list options] file.uuz\n"
            "    uuz d [decode options] file.uuz\n"
            "      (file.uuz can be '-' for stdin)\n"
            "encode options:\n"
            "    -o FILE.uuz : output filename for uuz\n"
            "      (defaults to stdout if not specified)\n"
            "    -m maxbytes\n"
            "      (outputs multiple file.uuz.%%03d of max size)\n"
            "    -c [none|libz] : specify compression, libz default\n"
            "    -t : include text headers, default is off\n"
            "    -d FLAGS : debug mode, sum of following:\n"
            "       1 : protobuf\n"
            "       2 : libz\n"
            "       4 : decoder\n"
            "       8 : MBED encryption\n"
            "       16 : HMAC\n"
            "    -e PASSWORD : encrypt with AES256\n"
            "        (key derived by SHA256 of password)\n"
            "    -v [variant#] : specify base64 variant\n"
            "       note VERSION block is always in hex\n"
            "       default data block variant is %d (%s)\n"
            "       available base64 variants:\n",
            (int) data_block_variant_default,
            Base64::variant_name(data_block_variant_default)
        );
    for (int v = (int) __BASE64_VARIANT_FIRST;
         v <= (int) __BASE64_VARIANT_LAST;
         v++)
    {
        fprintf(stderr, "          %d (%s)\n", v,
                Base64::variant_name((Base64Variant) v));
    }
    fprintf(stderr,
            "decode and list options:\n"
            "    -d FLAGS : debug mode\n"
            "    -e PASSWORD : decrypt with AES256\n"
        );
}

}; // namespace PFK_uuz
