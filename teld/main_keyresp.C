/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
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
