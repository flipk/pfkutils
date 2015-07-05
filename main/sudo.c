/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
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
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
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
        chdir("/");
        initgroups(usern, pw->pw_gid);
    }

    if ((uid != MY_UID) && (uid != 0))
    {
        printf("sorry, permission denied.\n");
        exit(1);
    }

    setgid(newgid);
    setuid(newuid);

    if (getuid() != newuid)
    {
	printf("sorry, binary not installed setuid?\n");
	exit(1);
    }

    if (homedir)
    {
        chdir(homedir);
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
