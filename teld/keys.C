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

#include "keys.H"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/file.h>

// file format:
//   4 bytes: number of keys
//   array of PfkKeyPair

static char *
get_keyfile(void)
{
    char * ret;
    ret = getenv( "PFK_TEL_KEYS" );
    if (ret)
    {
        ret = strdup(ret);
        return ret;
    }
    // else
    ret = (char*)malloc(256);
    sprintf(ret, "%s/.pfktelkeys", getenv( "HOME" ));
    fprintf(stderr, "PFK_TEL_KEYS not set; using %s\n", ret);
    return ret;
}

PfkKeyPairs :: PfkKeyPairs( void )
{
    char * keyfile = get_keyfile();
    fd = open(keyfile, O_RDWR);
    free(keyfile);

    if (fd < 0)
    {
        fprintf(stderr,"unable to open keyfile '%s'\n", keyfile);
        exit(1);
    }

    if (flock(fd, LOCK_EX) < 0)
    {
        fprintf(stderr,"unable to flock keyfile '%s'\n", keyfile);
        exit(1);
    }

    if (read(fd, &num_pairs, 4) != 4)
    {
        fprintf(stderr,"unable to read num_pairs\n");
        exit(1);
    }
    num_pairs = ntohl(num_pairs);
    pairs = new PfkKeyPair[num_pairs];
    int rdsize = sizeof(PfkKeyPair) * num_pairs;
    if (read(fd, pairs, rdsize) != rdsize)
    {
        fprintf(stderr,"unable to read keys\n");
        exit(1);
    }
}

PfkKeyPairs :: PfkKeyPairs( int _num )
{
    num_pairs = _num;

    char * keyfile = get_keyfile();
    fd = open(keyfile, O_RDWR | O_CREAT, 0600);
    free(keyfile);

    if (fd < 0)
    {
        fprintf(stderr,"cannot write keyfile\n");
        exit(1);
    }

    pairs = new PfkKeyPair[num_pairs];
    for (int i=0; i < num_pairs; i++)
    {
        for (int j=0; j < (PFK_KEYLEN-1); j++)
        {
            pairs[i].challenge[j] = (random() % 94) + 0x21;
            pairs[i].response [j] = (random() % 94) + 0x21;
        }
        pairs[i].challenge[PFK_KEYLEN-1] = 0;
        pairs[i].response [PFK_KEYLEN-1] = 0;
    }
}

PfkKeyPairs :: ~PfkKeyPairs( void )
{
    int wrsize = sizeof(PfkKeyPair) * num_pairs;
    num_pairs = htonl(num_pairs);
    lseek(fd, 0, SEEK_SET);
    ftruncate(fd, 0);
    write(fd, &num_pairs, 4);
    write(fd, pairs, wrsize);
    close(fd);
    delete[] pairs;
}

bool
PfkKeyPairs :: pick_key( PfkKeyPair * pair )
{
    if (num_pairs == 0)
        return false;

    int i = random() % num_pairs;
    *pair = pairs[i];
    num_pairs--;
    if (i != num_pairs)
        pairs[i] = pairs[num_pairs];

    return true;
}

bool
PfkKeyPairs :: find_response( PfkKeyPair * pair )
{
    for (int i=0; i < num_pairs; i++)
        if (memcmp(pair->challenge, pairs[i].challenge, PFK_KEYLEN) == 0)
        {
            memcpy(pair->response, pairs[i].response, PFK_KEYLEN);
            num_pairs--;
            if (i != num_pairs)
                pairs[i] = pairs[num_pairs];
            return true;
        }
    return false;
}
