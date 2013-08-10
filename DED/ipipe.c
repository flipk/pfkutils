
#define BUFSIZE 16384

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include <netinet/in.h>

#include "m.h"

static const char *helpmsg =
"\n"
"ipipe [-i infile] [-o outfile] [-pqsve] port     (passive)\n"
"ipipe [-i infile] [-o outfile] [-pqsve] host port    (active)\n"
"ipipe [-i infile] [-o outfile] [-psve] host port pty (redirect to pty)\n"
"    -i: redirect fd 0 to input file\n"
"    -o: redirect fd 1 to output file\n"
"    -q: set raw qcomm mode\n"
"    -s: display stats of transfer at end\n"
"    -v: display stats live, during transfer (1 second updates) implies -s\n"
"    -e: echo a hex dump of the transfer in each dir to stderr\n"
"    -d: delete 0x00's and 0x0D's from the input stream\n"
"    -n: do not read from stdin\n"
"    -p: use ping-ack method to lower network impact (must use on both ends)\n"
"\n";

static int options;

#define OPT_FLOOD    ( 2 << 0 )
#define OPT_STAT     ( 2 << 1 )
#define OPT_VERBOSE  ( 2 << 2 )
#define OPT_ECHO     ( 2 << 3 )
#define OPT_RAWQ     ( 2 << 4 )
#define OPT_PINGACK  ( 2 << 5 )
#define OPT_NOSTDIN  ( 2 << 6 )
#define OPT_DELNULS  ( 2 << 7 )

#define SET(x)    options |= x
#define ISSET(x) (options &  x)

int do_connect(char *, int);
static void read_write(int);
static void setrawmode(int);
static void dumphex(int, char *, int);

static void
usage( void )
{
    fprintf(stderr, helpmsg);
    exit(1);
}

int
ipipe_main( int argc, char ** argv )
{
    int fd, ch;
    char *  input_file = NULL;
    char * output_file = NULL;

    extern int optind;
    extern char * optarg;

    srandom(getpid() * time(NULL));

    signal( SIGTTIN, SIG_IGN );
    options = 0;

    while (( ch = getopt( argc, argv, "qfsndvepl:i:o:" )) != -1 )
    {
        switch ( ch )
        {
        case 'f': SET(OPT_FLOOD);   break;
        case 's': SET(OPT_STAT);    break;
        case 'v': SET(OPT_VERBOSE); break;
        case 'e': SET(OPT_ECHO);    break;
        case 'q': SET(OPT_RAWQ);    break;
        case 'n': SET(OPT_NOSTDIN); break;
        case 'd': SET(OPT_DELNULS); break;
        case 'p': SET(OPT_PINGACK); break;
        case 'i':   input_file = optarg;  break;
        case 'o':  output_file = optarg;  break;
        default:  usage();
        }
    }

    argc -= optind;
    argv += optind;

    switch ( argc ) {
    case 1:
        fd = do_connect( NULL, atoi( argv[0] ));
        break;

    case 3:
        /* redirect port to a pty; same as case 2, except
           that local file descriptors 0 and 1 are redefined. */
        fd = open( argv[2], O_RDWR );
        if ( fd < 0 )
        {
            fprintf( stderr, "error %d opening pty\n", errno );
            return -1;
        }
        dup2( fd, 0 );
        dup2( fd, 1 );
        setrawmode( fd );
        /* fall through */

    case 2:
        if ( ISSET(OPT_RAWQ) )
        {
            setrawmode( 0 );
        }
        fd = do_connect( argv[0], atoi( argv[1] ));
        break;

    default:
        usage();
    }

#ifndef O_BINARY
#define O_BINARY 0
#endif

    if ( input_file )
    {
        int fd2 = open( input_file, O_RDONLY | O_BINARY );
        if ( fd2 < 0 )
        {
            fprintf( stderr, "error %d opening input file\n", errno );
            return -1;
        }
        dup2( fd2, 0 );
        close( fd2 );
    }

    if ( output_file )
    {
        int fd2 = open( output_file, O_WRONLY | O_CREAT | O_BINARY, 0600 );
        if ( fd2 < 0 )
        {
            fprintf( stderr, "error %d opening output file\n", errno );
            return -1;
        }
        dup2( fd2, 1 );
        close( fd2 );
    }

    read_write( fd );

    return 0;
}

void
read_write( int fd )
{
    fd_set fds;
    char buf[BUFSIZE];
    int cc, infd = 0, outfd = 1;

    M_INT64 total_bytes;
    time_t start_time, last_time, time_now;

    int total_1s;
    int total_5s;
    int each_sec[5];
    int each_ind;

    FD_ZERO(&fds);

    if ( ISSET(OPT_FLOOD) )
    {
        /* if you want to test a link that compresses data,
           like PPP with built-in libz support, send random
           data, since it doesn't compress well. */
        for ( cc = 0; cc < BUFSIZE; cc++ )
            buf[cc] = random() & 0xff;
        outfd = fd;
    }

    last_time = time_now = time( &start_time );

    total_bytes = 0;
    total_1s = 0;
    total_5s = 0;
    each_sec[0] = 0;
    each_sec[1] = 0;
    each_sec[2] = 0;
    each_sec[3] = 0;
    each_sec[4] = 0;
    each_ind = 0;

    while (1)
    {
        if ( !ISSET(OPT_FLOOD) )
        {
#define setup_fds() do { \
        FD_ZERO( &fds ); \
        if ( !ISSET(OPT_NOSTDIN)) \
        FD_SET( 0, &fds ); \
        FD_SET( fd, &fds ); } while(0)

            setup_fds();
            cc = select( fd+1, &fds, NULL, NULL, NULL );

            if (FD_ISSET(0, &fds))
            {
                infd = 0;
                outfd = fd;
            }
            else
            {
                infd = fd;
                outfd = 1;
            }

            if (infd == fd && ISSET(OPT_PINGACK))
            {
                int in_size, temp_cc;

                cc = read(infd, &in_size, 4);

                if (cc == 4)
                {
                    cc = 0;
                    in_size = ntohl(in_size);

                    while (cc < in_size)
                    {
                        temp_cc = read(infd, buf+cc, in_size-cc);
                        cc += temp_cc;
                    }

                    write(fd,buf,1);
                }
                else
                {
                    cc = 0;
                }
            }
            else
            {
                cc = read(infd, buf, BUFSIZE);
            }
        }
        else
        {
            cc = BUFSIZE;
        }

        if (ISSET(OPT_RAWQ) && (buf[0] == 3))
        {
            printf( "\n" );
            break;
        }

        total_bytes += cc;
        total_1s += cc;

        if (ISSET(OPT_VERBOSE) && time(&time_now) != last_time)
        {
            char rate[30];

            total_5s -= each_sec[each_ind];
            each_sec[each_ind] = total_1s;
            total_1s = 0;
            total_5s += each_sec[each_ind];

            each_ind = (each_ind+1)%5;

            last_time = time_now;
            strcpy( rate, m_dump_number( total_bytes /
                                         (time_now-start_time), 10 ));

            if ( (time_now-start_time) < 5 )
                fprintf( stderr,
                         " %s bytes in %d seconds (%s bps)    \r",
                         m_dump_number( total_bytes, 10 ),
                         time_now-start_time, rate );
            else
                fprintf( stderr,
                         " %s bytes in %d seconds (%s bps) "
                         "(5s avg %d bps)  \r",
                         m_dump_number( total_bytes, 10 ),
                         time_now-start_time, rate, total_5s / 5 );

        }

        if (cc > 0)
        {
            if (ISSET(OPT_RAWQ) && (infd == 0))
            {
                int x;
                for (x = 0; x < cc; x++)
                    if (buf[x] == 10)
                        buf[x] = 13;
            }

            if (outfd == fd && ISSET(OPT_PINGACK))
            {
                int outsize;
                outsize = htonl(cc);
                write(fd, &outsize, 4);
            }

            if ( ISSET(OPT_DELNULS))
            {
                int a,b;
                char outbuf[ BUFSIZE ];

                for ( a = b = 0; a < cc; a++ )
                    if ( buf[a] != 0 && buf[a] != 13 )
                        outbuf[b++] = buf[a];

                write( outfd, outbuf, b );
            }
            else
            {
                write(outfd, buf, cc);
            }

            if (ISSET(OPT_ECHO))
                dumphex(infd, buf, cc);

            if (outfd == fd && ISSET(OPT_PINGACK))
            {
                read(fd, buf, 1);
            }
        }
        else
        {
            break;
        }
    }

    if (ISSET(OPT_VERBOSE))
        fprintf( stderr, "\n" );

    if (ISSET(OPT_VERBOSE) || ISSET(OPT_STAT))
    {
        char rate[30];
        /* avoid a divide-by-zero if the transfer takes
           less than a second. */
        if (time(&time_now) == start_time)
            time_now++;

        strcpy( rate, m_dump_number( total_bytes /
                                     (time_now-start_time), 10 ));

        fprintf( stderr,
                 " %s bytes in %d seconds (%s bps)    \n",
                 m_dump_number( total_bytes, 10 ),
                 time_now-start_time, rate );
    }

    close(fd);
}

void
setrawmode(fd)
    int fd;
{
    struct termios tio;

    tcgetattr(fd, &tio);

    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;

    tio.c_iflag &= ~(ISTRIP | IXON | IXANY | IXOFF | IMAXBEL );

    tio.c_lflag &= ~(ISIG | ICANON | ECHO);
    tio.c_lflag |= NOFLSH;

    tio.c_cflag &= ~(CSIZE);
    tio.c_cflag |= CS8;

    tio.c_cc[ VDISCARD ] = 0;

    tcsetattr(fd, TCSANOW, &tio);
}

void
dumphex(fd, buf, size)
    int fd, size;
    char *buf;
{
    int i, j;

    j = 0;
    while (1)
    {
        fprintf(stderr, "%d: ", fd);

        /* first, dump hex version */
        for (i=0; i < 16; i++)
        {
            if ((j+i) >= size)
                fprintf(stderr, "   ");
            else
                fprintf(stderr, "%02x ", (unsigned char)buf[j + i]);
        }

        fprintf(stderr, "   ");
        /* then, dump ascii version */
        for (i=0; i < 16; i++)
        {
            if ((j+i) < size)
            {
                char x = buf[j + i];
                if (x >= 32)
                    fputc(x, stderr);
                else
                    fputc('.', stderr);
            }
        }

        j += i;

        fprintf(stderr, "\n");
        if (j >= size)
            break;
    }
}
