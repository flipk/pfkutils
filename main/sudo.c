/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <grp.h>

#include "sudo.h"

int
sudo_main( int argc, char ** argv )
{
    int uid = getuid();
    int newuid = 0;
    int newgid = 0;
    char *usern = "root";
    char *homedir = NULL;
    struct passwd *pw;
    char envstr[ 80 ];

    if ((argc > 2) && (strcmp(argv[1], "-u") == 0))
    {
        if ((pw = getpwnam(argv[2])) == NULL)
        {
            printf("unknown user %s\n", argv[2]);
            exit(1);
        }
        newuid = pw->pw_uid;
        newgid = pw->pw_gid;
        printf("switching to uid %d gid %d\n", 
               pw->pw_uid, pw->pw_gid);
        usern = argv[2];
        argv += 2;
        argc -= 2;
        homedir = pw->pw_dir;
        if (chdir("/") < 0)
        { /* quiet compiler */ }
        initgroups(usern, pw->pw_gid);
    }

    if ((uid != MY_UID) && (uid != 0))
    {
        printf("sorry, permission denied.\n");
        exit(1);
    }

    if (setgid(newgid) < 0)
        printf("setgid failed\n");
    if (setuid(newuid) < 0)
        printf("setuid failed\n");

    if (getuid() != newuid)
    {
	printf("sorry, binary not installed setuid?\n");
	exit(1);
    }

    if (homedir)
    {
        if (chdir(homedir) < 0)
            printf("cant chdir to homedir: %s\n",
                   strerror(errno));
        sprintf( envstr, "HOME=%s", homedir );
        putenv( envstr );
    }

    sprintf( envstr, "USER=%s", usern );
    putenv( envstr );
    sprintf( envstr, "LOGNAME=%s", usern );
    putenv( envstr );

    if (argc == 1) 
        execl(getenv("SHELL"), getenv("SHELL"), NULL);
    else
        execvp(argv[1], argv+1);

    perror("exec failed");
    return 1;
}
