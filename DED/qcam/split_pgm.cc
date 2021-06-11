#if 0

for f in dance00* ; do echo file "'"$f"'" ; done > list.txt

ffmpeg -r 5 -f concat -safe 0 \
   -i list.txt -c:v libx264 -vf "fps=20,format=yuv420p" -y out.mp4

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

void usage(void)
{
    fprintf(stderr,
            "usage: split_pgm in.mov out_basename\n"
            "       (outfiles will be out_basename%%05d.pgm\n");
}

struct pgm {
    int x_size;
    int y_size;
    uint8_t *data;

    pgm(void) { data = NULL; }
    ~pgm(void) { if (data) delete[] data; }
};

bool read_pgm_hdr(pgm &p, FILE *in)
{
    char inl[40], *tp;

    // discard the 'P5'
    if (fgets(inl, 40, in) == NULL)
        return false;

    // read the x and y size part.
    if (fgets(inl, 40, in) == NULL)
        return false;

    tp = inl;
    strsep(&tp, " ");
    p.x_size = atoi(inl);
    p.y_size = atoi(tp);

    // discard the number of gray levels.
    // it will be 63 btw.
    fgets(inl, 40, in);

    p.data = new uint8_t[p.x_size * p.y_size];

    return true;
}

void write_pgm_hdr(FILE *out, const pgm &p)
{
    fprintf(out, "P5\n");
    fprintf(out, "%d %d\n", p.x_size, p.y_size);
    fprintf(out, "63\n");
}

bool read_pgm_frame(pgm &p, FILE *in)
{
    size_t expected = p.x_size * p.y_size;
    size_t cc = fread(p.data, 1, expected, in);
    return (cc == expected);
};

void write_pgm_frame(FILE *out, const pgm &p)
{
    fwrite(p.data, 1, p.x_size * p.y_size, out);
}

int
main(int argc, char ** argv)
{
    if (argc != 3)
    {
        usage();
        return 1;
    }

    FILE *in = fopen(argv[1], "r");
    if (!in)
    {
        fprintf(stderr, "unable to open: %s\n", argv[1]);
        return 1;
    }

    string outbase = argv[2];
    ostringstream outname;

    pgm p;

    if (read_pgm_hdr(p, in) == false)
    {
        fprintf(stderr, "failure parsing input header\n");
        return 1;
    }

    int counter = 1;
    while (true)
    {
        if (read_pgm_frame(p, in) == false)
            break;

        outname.str("");
        outname << outbase
                << dec << setfill('0') << setw(5) << counter
                << ".pgm";

        FILE *out = fopen(outname.str().c_str(), "w");
        if (out == NULL)
        {
            fprintf(stderr, "failure opening output '%s'\n",
                    outname.str().c_str());
            return 1;
        }
        write_pgm_hdr(out, p);
        write_pgm_frame(out, p);
        fclose(out);

        counter++;
    }

    return 0;
}
