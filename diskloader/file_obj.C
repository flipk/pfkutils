
#include "file_obj.H"
#include "pseudo_random.H"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

file_obj :: file_obj(void)
{
    exists = false;
    seed = 0;
    busy = false;
}

void
file_obj :: init(int _ind)
{
    int ind1;
    int ind2;
    int fileind;
    char dir1[64];
    char dir2[64];

    ind1 = _ind / 10000;
    _ind = _ind % 10000;
    ind2 = _ind / 100;
    _ind = _ind % 100;
    fileind = _ind;

    sprintf(dir1, "%03d", ind1);
    mkdir(dir1, 0755);
    sprintf(dir2, "%03d/%02d", ind1, ind2);
    mkdir(dir2, 0755);
    sprintf(path, "%03d/%02d/%02d", ind1, ind2, fileind);
}

file_obj :: ~file_obj(void)
{
    if (exists)
        unlink(path);
}

void
file_obj :: create(void)
{
    seed = random();
    size = (random() % MAX_FILE_SIZE) & 0x7ffffffc;
#if 0
    printf("creating '%s' of size %d with seed %d\n",
           path, size, seed);
#else
    printf("c"); fflush(stdout);
#endif

    FILE * f = fopen(path, "w");
    if (!f)
    {
        fprintf(stderr, "error opening '%s': %d : %s\n",
                path, errno, strerror(errno));
        return;
    }

    pseudo_random_generator  g(seed);
    uint32_t pos = 0;
    while (pos < size)
    {
        uint32_t v = g.next_value();
        fwrite(&v, 4, 1, f);
        pos += 4;
    }
    fclose(f);
    exists = true;
}

void
file_obj :: verify(void)
{
    printf("v"); fflush(stdout);
    FILE * f = fopen(path, "r");
    if (!f)
    {
        fprintf(stderr, "error opening '%s': %d : %s\n",
                path, errno, strerror(errno));
        return;
    }

    pseudo_random_generator  g(seed);
    uint32_t pos = 0;
    while (pos < size)
    {
        uint32_t v = g.next_value();
        uint32_t v2;
        if (fread(&v2, 1, 4, f) != 4)
        {
            fprintf(stderr, "file '%s' short read!\n", path);
            break;
        }
        if (v != v2)
        {
            fprintf(stderr, "file '%s' validation failed at offset %d!\n",
                    path, pos);
            break;
        }
        pos += 4;
    }

    fclose(f);
}

void
file_obj :: destroy(void)
{
    printf("d"); fflush(stdout);
    if (unlink(path) < 0)
        fprintf(stderr, "file '%s' unlink error: %d: %s\n",
                path, errno, strerror(errno));
    exists = false;
}
