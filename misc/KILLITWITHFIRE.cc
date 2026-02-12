#if 0
set -e -x
g++ -Wall -Werror KILLITWITHFIRE.cc -o KILLITWITHFIRE
exit 0 ;
#endif

#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <string>

void process_directory(const std::string &dirname, int pass);
void wipe_file(const std::string &fname, int pass);
void init_buffers(void);
void write_buffer(const std::string &fname, int numbufs, char *buf);

int
main(int argc, char ** argv)
{
    if (argc < 2)
    {
        printf("usage :   KILLITWITHFIRE <directory> [<directory>]\n");
        return 1;
    }
    init_buffers();
    for (int arg = 1; arg < argc; arg++)
    {
        std::string dir(argv[arg]);
        for (int pass = 0; pass <= 4; pass++)
        {
            // pass 0: overwrite with 00's
            // pass 1: overwrite with ff's
            // pass 2: overwrite with 55's
            // pass 3: overwrite with aa's
            // pass 4: unlink/rmdir
            process_directory(dir, pass);
            sync();
        }
    }
    return 0;
}

class myReaddir {
    DIR * d;
    bool isOk;
public:
    myReaddir(const char *dirname) {
        d = ::opendir(dirname);
        if (d == NULL)
            isOk = false;
        else
            isOk = true;
    }
    ~myReaddir(void) {
        if (d != NULL)
            closedir(d);
    }
    const bool ok(void) const { return isOk; }
    dirent * read(void) {
        // readdir_r was officially DEPRECATED in POSIX.1-2008 for
        // several reasons, which can easily be explained in an
        // internet search. Not duplicating the explanation here.
        return readdir(d);
    }
    void rewind(void) {
        rewinddir(d);
    }
};

void
process_directory(const std::string &dirname, int pass)
{
    (void) chmod(dirname.c_str(), 0700);
    {
        myReaddir d(dirname.c_str());
        if (!d.ok())
        {
            printf("couldn't open %s\n", dirname.c_str());
            return;
        }
        printf("wiping dir '%s'\n", dirname.c_str());
        dirent * de;
        while ((de = d.read()) != NULL)
        {
            const std::string fname(de->d_name);
            if (fname == "." || fname == "..")
                continue;
            const std::string fullname = dirname + "/" + fname;
            switch (de->d_type)
            {
            case DT_DIR:
                process_directory(fullname, pass);
                break;
            case DT_REG:
                wipe_file(fullname, pass);
                break;
            default:
                unlink(fullname.c_str());
                break;
            }
        }
    }
    if (pass == 4)
        (void) rmdir(dirname.c_str());
}

char buffer_zeros[16384];
char buffer_ones [16384];
char buffer_55   [16384];
char buffer_aa   [16384];

void init_buffers(void)
{
    memset(buffer_zeros, 0x00, 16384);
    memset(buffer_ones , 0xff, 16384);
    memset(buffer_55   , 0x55, 16384);
    memset(buffer_aa   , 0xaa, 16384);
}

char * buffers[4] = {
    buffer_zeros,
    buffer_ones,
    buffer_55,
    buffer_aa
};

void wipe_file(const std::string &fname, int pass)
{
    if (pass == 4)
    {
        printf("deleting %s\n", fname.c_str());
        (void) unlink(fname.c_str());
    }
    else
    {
        struct stat sb;
        if (stat(fname.c_str(), &sb) < 0)
        {
            printf("couldn't stat %s : %s\n",
                   fname.c_str(),  strerror(errno));
            return;
        }
        printf("wiping file %s pass %d (size %ld)\n",
               fname.c_str(), pass, sb.st_size);
        int numbufs = (sb.st_size / 16384) + 1;
        write_buffer(fname, numbufs, buffers[pass]);
    }
}

void write_buffer(const std::string &fname, int numbufs, char *buf)
{
    int ctr, fd;
    (void) chmod(fname.c_str(), 0600);
    fd = open(fname.c_str(), O_WRONLY);
    if ( fd < 0 )
    {
        printf("cannot open '%s' : %s\n", fname.c_str(), strerror(errno));
        return;
    }
    for (ctr = 0; ctr < numbufs; ctr++)
        (void) write(fd, buf, 16384);
    close(fd);
}
