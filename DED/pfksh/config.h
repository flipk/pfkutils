/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */
/*
 * This file, acconfig.h, which is a part of pdksh (the public domain ksh),
 * is placed in the public domain.  It comes with no licence, warranty
 * or guarantee of any kind (i.e., at your own risk).
 */

#ifndef CONFIG_H
#define CONFIG_H

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define as the return value of signal handlers (0 or ).  */
#define RETSIGVAL 

/* Define if `sys_siglist' is declared by <signal.h>.  */
/* #define SYS_SIGLIST_DECLARED 1 */

/* Define if sys_errlist[] and sys_nerr are in the C library */
#define HAVE_SYS_ERRLIST 1

/* Define if sys_siglist[] is in the C library */
/* #define HAVE_SYS_SIGLIST 1 */

/* Define if you have a sane <termios.h> header file */
#define HAVE_TERMIOS_H 1

/* Define if you have a sane <termio.h> header file */
/* #undef HAVE_TERMIO_H */

/* Define if the pgrp of setpgrp() can't be the pid of a zombie process */
/* #undef NEED_PGRP_SYNC */

/* Define if your OS maps references to /dev/fd/n to file descriptor n */
#define HAVE_DEV_FD 1

/* Default PATH (see comments in configure.in for more details) */
#define DEFAULT_PATH "/bin:/usr/bin:/usr/ucb"

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 8

/* Define if you have the _setjmp function.  */
#define HAVE__SETJMP 1

/* Define if you have the confstr function.  */
#define HAVE_CONFSTR 1

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD 1

/* Define if you have the killpg function.  */
#define HAVE_KILLPG 1

/* Define if you have the nice function.  */
#define HAVE_NICE 1

/* Define if you have the setrlimit function.  */
#define HAVE_SETRLIMIT 1

/* Define if you have the sigsetjmp function.  */
/* #undef HAVE_SIGSETJMP */

/* Define if you have the strcasecmp function.  */
#define HAVE_STRCASECMP 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the sysconf function.  */
#define HAVE_SYSCONF 1

/* Define if you have the tcsetpgrp function.  */
#define HAVE_TCSETPGRP 1

/* Define if you have the ulimit function.  */
#define HAVE_ULIMIT 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <paths.h> header file.  */
#define HAVE_PATHS_H 1

/* Define if you have the <stddef.h> header file.  */
#define HAVE_STDDEF_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <sys/resource.h> header file.  */
#define HAVE_SYS_RESOURCE_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/wait.h> header file.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have the <ulimit.h> header file.  */
#define HAVE_ULIMIT_H 1

#endif /* CONFIG_H */
