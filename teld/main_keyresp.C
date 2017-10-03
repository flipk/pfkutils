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
#include <string.h>
#include <unistd.h>
#include "keys.H"

extern "C" int
pfktel_keyresp_main()
{
    PfkKeyPairs pairs;
    PfkKeyPair p;
    char buf[512];
    int len;
    char * chal;

    memset(buf,0,sizeof(buf));
    len = read(0, buf, sizeof(buf)-1);
    if (len <= 0)
    {
        fprintf(stderr,"error reading challenge\n");
        return 1;
    }
    buf[sizeof(buf)-1] = 0;

    chal = strstr(buf, "challenge: ");
    if (!chal)
    {
        fprintf(stderr,"challenge header not found\n");
        return 1;
    }
    if (strlen(chal+11) < (PFK_KEYLEN-1))
    {
        fprintf(stderr,"challenge header incomplete\n");
        return 1;
    }
    memcpy(p.challenge, chal+11, PFK_KEYLEN-1);
    p.challenge[PFK_KEYLEN-1]=0;

    if (pairs.find_response( &p ) == false)
    {
        fprintf(stderr,"unknown challenge '%s'\n", p.challenge);
        return 1;
    }

    printf("response: %s\n", p.response);

    return 0;
}
