#include <stdio.h>

extern char *progname;

#define DEBUG(x) \
    fprintf(stderr, "%s (pid %d): %s (line %d): %s\n", progname, getpid(), \
	    __FILE__, __LINE__, x)
