
#include <stdio.h>
#include <fftw3.h>
#include <stdlib.h>

#define __STDC_FORMAT_MACROS // enable PRIu64 in intttypes.h
#include <inttypes.h>

#include <string>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "simpleRegex.h"

void
usage(const std::string &errstr)
{
    fprintf(stderr,
"%s\n"
"do_fft [fr] input_format input_file sample_count first_sample repeat_count\n"
"\n"
"  f - forward fft\n"
"  r - reverse fft\n"
"input_format:\n"
"   b[so][rq][c]8  -- binary 8 bit \n"
"                  [signed 2's complement | offset by 0x80]\n"
"                  [real | quadrature] [conjugate]\n"
"   b[so][rq][c]16[bl] -- binary 16 bit\n"
"                  [signed 2's complement | offset by 0x8000]\n"
"                  [real | quadrature] [conjugate] [big | little]\n"
"   t[rq][c][Cn] -- text integer \n"
"                  [real | quadrature] [conjugate] [column #]\n"
"first_sample is 0-based, 0 if not specified\n"
"repeat_count is #fft of sample_count each, 1 if not specified\n"
"column # is 1-based, like gnuplot\n"
"NOTE for reverse mode, the input order is a bit odd:\n"
" 0=[dc] 1=[small +freq] ... (n/2)-1=[large +freq]\n"
" (n/2)=[nyquist freq]  (n/2)+1=[large -freq] ... (n-1)=[small -freq]\n",
            errstr.c_str()
        );
}

class input_format_regex : public pxfe_regex<> {
public:
    static const int BINARY = 2;
    static const int BIN_S_OR_O = 3;
    static const int BIN_R_OR_Q = 4;
    static const int BIN_C = 5;
    static const int BIN_816bl = 6;
    static const int TEXT = 7;
    static const int TEXT_R_OR_Q = 8;
    static const int TEXT_C = 9;
    static const int TEXT_COLNUM = 10;

    input_format_regex(void) :
        pxfe_regex(
            "^((b(s|o)(r|q)(c|)(8|16b|16l))|(t(r|q)(c|)([0-9]+)))$"
            ) { }
};

struct InputFormat {
    input_format_regex  reg;
    bool ok;
    double ioffset, qoffset;
    enum { BINARY, TEXT } binary;
    enum { BITS8, BITS16 } binarySize;
    enum { BIG, LITTLE } endian;
    enum { SIGNED, OFFSET } representation;
    enum { REAL, QUADRATURE } complexity;
    bool conjugate;
    int text_colnum;
    int line_number;
    std::vector<std::string> splitString( const std::string &line )
    {
        std::vector<std::string> ret;
        size_t pos;
        for (pos = 0; ;)
        {
            // valid token separators are tab, space, or comma
            size_t found = line.find_first_of("\t ,",pos);
            if (found == std::string::npos)
            {
                if (pos != line.size())
                    ret.push_back(line.substr(pos,line.size()-pos));
                break;
            }
            if (found > pos)
                ret.push_back(line.substr(pos,found-pos));
            pos = found+1;
        }
        return ret;
    }
    InputFormat(const std::string &arg) {
        binary = BINARY;
        binarySize = BITS8;
        endian = BIG;
        representation = SIGNED;
        complexity = REAL;
        conjugate = false;
        ioffset = qoffset = 0.0;
        text_colnum = 1;
        line_number = 0;
        ok = false;

        if (!reg.ok())
        {
            printf("ERROR REGEX COMPILE FAILED\n");
            return;
        }
        if (!reg.exec(arg))
        {
            printf("input format string '%s' did not parse\n", arg.c_str());
            return;
        }
        if (reg.match(input_format_regex::BINARY))
        {
            if (reg.match(arg, input_format_regex::BIN_R_OR_Q) == "q")
                complexity = QUADRATURE;
            if (reg.match(arg, input_format_regex::BIN_C) == "c")
                conjugate = true;
            std::string sz = reg.match(arg, input_format_regex::BIN_816bl);
            if (sz == "16l")
            {
                binarySize = BITS16;
                endian = LITTLE;
            }
            else if (sz == "16b")
            {
                binarySize = BITS16;
                endian = BIG;
            }
            if (reg.match(arg, input_format_regex::BIN_S_OR_O) == "o")
            {
                representation = OFFSET;
                if (binarySize == BITS8)
                    ioffset = qoffset = 0x80;
                else
                    ioffset = qoffset = 0x8000;
            }
        }
        else if (reg.match(input_format_regex::TEXT))
        {
            binary = TEXT;
            if (reg.match(arg, input_format_regex::TEXT_R_OR_Q) == "q")
                complexity = QUADRATURE;
            if (reg.match(arg, input_format_regex::TEXT_C) == "c")
                conjugate = true;
            std::string colnum = reg.match(arg,
                                           input_format_regex::TEXT_COLNUM);
            text_colnum = atoi(colnum.c_str());
        }
        ok = true;
    }
private:
    signed short get_short(unsigned char * buf) {
        if (endian == BIG)
            return (signed short) ((buf[0] << 8) | buf[1]);
        return (signed short) ((buf[1] << 8) | buf[0]);
    }
public:
    bool get_sample(std::istream &istr, fftw_complex &val) {
        if (!istr)
            return false;
        if (binary == BINARY) {
            if (binarySize == BITS8) {
                unsigned char iq[2] = { 0, 0 };
                istr.read((char*) iq, (complexity == REAL) ? 1 : 2);
                if (!istr)
                    return false;
                val[0] = (double) iq[0];
                val[1] = (double) iq[1];
            } else { // BITS16
                unsigned char buf[4];
                istr.read((char*) buf, (complexity == REAL) ? 2 : 4);
                if (!istr)
                    return false;
                val[0] = (double) get_short(buf + 0);
                if (complexity == QUADRATURE)
                    val[1] = (double) get_short(buf + 2);
                else
                    val[1] = 0.0;
            }
        } else { // TEXT
            std::string line;
            std::getline(istr, line);
            line_number ++;
            std::vector<std::string> fields = splitString(line);
            if (complexity == REAL) {
                if (fields.size() < text_colnum)
                {
                    printf("ERROR on line %d of input: not enough columns\n",
                           line_number);
                    return false;
                }
                // remember, colnum is 1-based
                char *endptr = NULL;
                val[0] = strtof(fields[text_colnum-1].c_str(), &endptr);
                if (*endptr != 0)
                {
                    printf("ERROR on line %d of input: not a number?\n",
                           line_number);
                    return false;
                }
                val[1] = 0;
            } else {
                if (fields.size() < (text_colnum+1))
                {
                    printf("ERROR on line %d of input: not enough columns\n",
                           line_number);
                    return false;
                }
                // remember, colnum is 1-based
                char *endptr = NULL;
                val[0] = strtof(fields[text_colnum-1].c_str(), &endptr);
                if (*endptr != 0)
                {
                    printf("ERROR on line %d of input: not a number?\n",
                           line_number);
                    return false;
                }
                val[1] = strtof(fields[text_colnum].c_str(), &endptr);
                if (*endptr != 0)
                {
                    printf("ERROR on line %d of input: not a number?\n",
                           line_number);
                    return false;
                }
            }
        }
        val[0] -= ioffset;
        val[1] -= qoffset;
        if (conjugate)
            val[1] = -val[1];
        return true;
    }
};    

int
main(int argc, char ** argv)
{
    if (argc < 5 || argc > 7)
    {
        usage("incorrect # of arguments");
        exit(1);
    }

    int direction;
    switch (argv[1][0])
    {
    case 'f':
        direction = FFTW_FORWARD;
        break;
    case 'r':
        direction = FFTW_BACKWARD;
        break;
    default:
        usage("direction must be f or r");
        exit(1);
    }

    InputFormat format(argv[2]);
    if (!format.ok)
    {
        usage("format descriptor arg not recognized");
        exit(1);
    }

    std::string input_file(argv[3]);

    char *endptr = NULL;
    uint32_t sample_count = strtoul(argv[4], &endptr, 0);
    if (endptr[0] != 0)
    {
        usage("sample_count arg not an integer");
        exit(1);
    }
    uint64_t first_sample = 0;
    if (argc > 5)
    {
        first_sample = strtoull(argv[5], &endptr, 0);
        if (endptr[0] != 0)
        {
            usage("first_sample arg not an integer");
            exit(1);
        }
    }
    uint32_t repeat_count = 1;
    if (argc > 6)
    {
        repeat_count = strtoul(argv[6], &endptr, 0);
        if (endptr[0] != 0)
        {
            usage("repeat_count arg not an integer");
            exit(1);
        }
    }

    std::ifstream instream(input_file.c_str());

    if (!instream)
    {
        fprintf(stderr,"unable to open input file %s\n", input_file.c_str());
        exit(1);
    }

    fftw_complex *in, *out;
    fftw_plan p;

    int N = sample_count;
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    p = fftw_plan_dft_1d(N, in, out, direction, FFTW_ESTIMATE);

    uint64_t sample_num = 0;
    fftw_complex  dummy;

    // skip first_sample samples;
    while (sample_num < first_sample)
    {
        if (format.get_sample(instream, dummy) == false)
        {
            fprintf(stderr,
                    "error reading file at sample %" PRIu64 "\n",
                    sample_num);
            exit(1);
        }
        sample_num++;
    }

    uint32_t repeat_counter = 0;
    while (repeat_counter < repeat_count)
    {
        // read sample_count samples
        int ind;
        for (ind = 0; ind < (int) sample_count; ind++)
        {
            if (format.get_sample(instream, in[ind]) == false)
            {
                fprintf(stderr,
                        "error reading file at sample %" PRIu64 "\n",
                        sample_num);
                exit(1);
            }
            if (0)
                printf("%d %f %f\n", ind, in[ind][0], in[ind][1]);
            sample_num++;
        }

        fftw_execute(p);

        if (direction == FFTW_FORWARD)
        {
            // 4.8.1 The 1d Discrete Fourier Transform (DFT)
            //
            // "For those who like to think in terms of positive and
            // negative frequencies, this means that the positive
            // frequencies are stored in the first half of the output and
            // the negative frequencies are stored in backwards order in
            // the second half of the output."

            for (ind = 0; ind < (int) (sample_count/2); ind++)
            {
                if (repeat_count > 1)
                    printf("%d ", repeat_counter);
                printf("%d %f %f\n",
                       ind - (sample_count/2),
                       out[(sample_count/2)+ind][0],
                       out[(sample_count/2)+ind][1]);
            }

            for (ind = 0; ind < (int) (sample_count/2); ind++)
            {
                if (repeat_count > 1)
                    printf("%d ", repeat_counter);
                printf("%d %f %f\n", ind,
                       out[ind][0],
                       out[ind][1]);
            }
        }
        else
        {
            for (ind = 0; ind < sample_count; ind++)
            {
                if (repeat_count > 1)
                    printf("%d ", repeat_counter);
                printf("%d %f %f\n", ind,
                       out[ind][0],
                       out[ind][1]);
            }
        }

        repeat_counter++;
    }

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);

    return 0;
}
