
/*
    This file is part of the "pkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>

#define uint16 unsigned short
#define uint32 unsigned int
#define uint8  unsigned char

struct pcap_file_header {
    uint32 magic;           // a1b2c3d4
    uint16 version_major;   // 2
    uint16 version_minor;   // 4
    uint32 thiszone;        // 0 
    uint32 sigfigs;         // 0 
    uint32 snaplen;         // 1500 
    uint32 linktype;        // 1 
};
struct pcap_pkthdr {
    struct timeval ts;  
    uint32 caplen;      
    uint32 len;
};

int
main(int argc, char ** argv)
{
    struct pcap_file_header pfh;
    struct pcap_pkthdr ph;
    char body[2000];
    char outfile[50];
    int filenum = 1;
    int filesize = 0;
    int outfd = -1;
    int len;

    read(0, &pfh, sizeof(pfh));

    while (1)
    {
        if (read(0, &ph, sizeof(ph)) != sizeof(ph))
            break;
        if (ph.caplen > 2000)
        {
            fprintf(stderr,"caplen %d\n", ph.caplen);
            exit(1);
        }
        len = read(0, body, ph.caplen);
        if (len != ph.caplen)
            exit(2);
        if (outfd == -1)
        {
            sprintf(outfile,"out%03d.cap",filenum++);
            unlink(outfile);
            outfd = open(outfile,O_WRONLY | O_CREAT,0644);
            if (outfd <= 0)
                exit(3);
            write(outfd, &pfh, sizeof(pfh));
            fprintf(stderr, "opened file %s\n", outfile);
        }
        write(outfd, &ph, sizeof(ph));
        write(outfd, body, len);
        filesize += (len + sizeof(ph));
        if (filesize > 10000000)
        {
            filesize = 0;
            close(outfd);
            outfd = -1;
        }
    }
    if (outfd != -1)
    {
        close(outfd);
    }
}
