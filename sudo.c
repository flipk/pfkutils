
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

    setuid(newuid);
    setgid(newgid);

    if (getuid() != newuid  ||
	getgid() != newgid)
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
        execl(getenv("SHELL"), "sh", NULL);
    else
        execvp(argv[1], argv+1);

    perror("exec failed");
    return 1;
}
