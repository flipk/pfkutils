
#include <stdio.h>
#include <fftw3.h>
#include <stdlib.h>

#define __STDC_FORMAT_MACROS // enable PRIu64 in intttypes.h
#include <inttypes.h>

#include <string>
#include <fstream>

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
"   t[wc][rq][c]   -- text integer [whitespace | comma]\n"
"                  [real | quadrature] [conjugate]\n"
"first_sample is 0-based, 0 if not specified\n"
"repeat_count is #fft of sample_count each, 1 if not specified\n",
            errstr.c_str()
        );
}

struct InputFormat {
    bool ok;
    double ioffset, qoffset;
    enum { BINARY, TEXT } binary;
    enum { BITS8, BITS16 } binarySize;
    enum { BIG, LITTLE } endian;
    enum { SIGNED, OFFSET } representation;
    enum { WHITESPACE, COMMA } textsep;
    enum { REAL, QUADRATURE } complexity;
    bool conjugate;
    InputFormat(const std::string &arg) {
        binary = BINARY;
        binarySize = BITS8;
        endian = BIG;
        representation = SIGNED;
        textsep = WHITESPACE;
        complexity = REAL;
        conjugate = false;
        ioffset = qoffset = 0.0;
        ok = false;
        if (arg.length() < 3)
            return;
        switch (arg[0]) {
        case 'b':
        {
            binary = BINARY;
            switch (arg[1]) {
            case 's': representation = SIGNED; break;
            case 'o': representation = OFFSET; break;
            default:
                fprintf(stderr,
                        "input_format binary second char must be s or o\n");
                return;
            }
            switch (arg[2]) {
            case 'r': complexity = REAL;       break;
            case 'q': complexity = QUADRATURE; break;
            default:
                fprintf(stderr,
                        "input_format binary third char must be r or q\n");
                return;
            }
            if (arg[3] == 'c')
                conjugate = true;
            const std::string &wordsize = arg.substr(conjugate ? 4 : 3);
            if (wordsize == "8") {
                binarySize = BITS8;
                if (representation == OFFSET)
                    ioffset = 128;
            } else if (wordsize == "16b") {
                binarySize = BITS16;
                endian = BIG;
                if (representation == OFFSET)
                    ioffset = 32768;
            } else if (wordsize == "16l") {
                binarySize = BITS16;
                endian = LITTLE;
                if (representation == OFFSET)
                    ioffset = 32768;
            } else {
                fprintf(stderr,
                        "input_format word size part must be 8, 16b, "
                        "or 16l, not '%s'\n", wordsize.c_str());
                return;
            }
            if (complexity == QUADRATURE)
                qoffset = ioffset;
            break;
        }
        case 't':
        {
            binary = TEXT;
            switch (arg[1]) {
            case 'w': textsep = WHITESPACE; break;
            case 'c': textsep = COMMA;      break;
            default:
                fprintf(stderr,
                        "input_format text second char must be w or c\n");
                return;
            }
            switch (arg[2]) {
            case 'r': complexity = REAL;       break;
            case 'q': complexity = QUADRATURE; break;
            default:
                fprintf(stderr,
                        "input_format text third char must be r or q\n");
                return;
            }
            if (arg.length() == 3 && arg[3] == 'c')
                conjugate = true;
            break;
        }
        default:
            fprintf(stderr,"input_format first char must be b or t\n");
            return;
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
            if (complexity == REAL) {
                // xxx TEXT real ws,comma
            } else {
                if (textsep == WHITESPACE)
                {
                    // TEXT QUAD SPACE
                    istr >> val[0] >> val[1];
                }
                else
                {
                    // xxx TEXT quadrature comma
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

// direction is not currently used...
//  enum { DIR_FORW, DIR_REV } direction = DIR_FORW;
    switch (argv[1][0])
    {
    case 'f':
//      direction = DIR_FORW;
        break;
    case 'r':
//      direction = DIR_REV;
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
    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

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

        repeat_counter++;
    }

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);

    return 0;
}
