
#include "file_obj.H"
#include "pseudo_random.H"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define VERBOSE 0

//static
void
file_obj :: create_directories(int max_num_files)
{
    int ind1, ind2, topdirs;
    char path[64];
    topdirs = ((max_num_files-1) / 10000) + 1;
    for (ind1 = 0; ind1 < topdirs; ind1++)
    {
        printf("\rcreating %d of %d ", ind1, topdirs);
        fflush(stdout);
        sprintf(path, "%03d", ind1);
        if (mkdir(path, 0755) < 0)
            printf("error mkdir: %d:%s\n", errno, strerror(errno));
        else
            for (ind2 = 0; ind2 < 100; ind2++)
            {
                sprintf(path, "%03d/%02d", ind1, ind2);
                if (mkdir(path, 0755) < 0)
                    printf("error mkdir: %d:%s\n", errno, strerror(errno));
            }
    }
    printf("\n");
}

//static
void
file_obj :: destroy_directories(int max_num_files)
{
    int ind1, topdirs;
    char cmd[80];
    topdirs = ((max_num_files-1) / 10000) + 1;
    for (ind1 = 0; ind1 < topdirs; ind1++)
    {
        printf("\rdestroying %d of %d ", ind1, topdirs);
        fflush(stdout);
        sprintf(cmd, "rm -rf %03d", ind1);
        system(cmd);
    }
    printf("\n");
}

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

    ind1 = _ind / 10000;
    _ind = _ind % 10000;
    ind2 = _ind / 100;
    _ind = _ind % 100;
    fileind = _ind;

    sprintf(path, "%03d/%02d/%02d", ind1, ind2, fileind);
}

file_obj :: ~file_obj(void)
{
}

void
file_obj :: create(int max_file_size)
{
    seed = random();
    size = (random() % max_file_size) & 0x7ffffffc;
#if 0
    printf("creating '%s' of size %d with seed %d\n",
           path, size, seed);
#else
#if VERBOSE
    printf("c"); fflush(stdout);
#endif
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
#if VERBOSE
    printf("v"); fflush(stdout);
#endif
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
#if VERBOSE
    printf("d"); fflush(stdout);
#endif
    if (unlink(path) < 0)
        fprintf(stderr, "file '%s' unlink error: %d: %s\n",
                path, errno, strerror(errno));
    exists = false;
}
