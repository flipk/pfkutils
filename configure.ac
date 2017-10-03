
m4_define([PFKUTILS_MAJOR_VERSION], 3)
m4_define([PFKUTILS_MINOR_VERSION], 8)

AC_INIT([pfkutils],
        [PFKUTILS_MAJOR_VERSION.PFKUTILS_MINOR_VERSION],
        [pknaack1@netscape.net])
AM_INIT_AUTOMAKE([-Wall -Werror])

AC_CONFIG_SRCDIR([main/main.C])
AC_CONFIG_MACRO_DIR([m4])

AC_SUBST([PFKUTILS_MAJOR_VERSION_NUM], [PFKUTILS_MAJOR_VERSION])
AC_SUBST([PFKUTILS_MINOR_VERSION_NUM], [PFKUTILS_MINOR_VERSION])

AC_ARG_ENABLE(environ,
    [  --enable-environ=ENV   set ENV to blade, isd, atd, or none],
    [ case $enableval in
        blade)
	  ;;
	isd)
	  ;;
	atd)
	  ;;
	none)
	  ;;
	*)
	  AC_MSG_ERROR([please specify --enable-environ=ENV])
	  ;;
      esac
      AC_SUBST([PFK_ENVIRON], [$enableval])
      AM_CONDITIONAL([PFK_ENVIRON], [test $enableval != none])
    ], [AC_MSG_ERROR([please specify --enable-environ=ENV])])

m4_define([_m4_divert(PFK_NOTICES_DIVERSION)],      20555)

# args : $1 = option name, $2 = help string
AC_DEFUN([PFK_ARG_ENABLE],
[
  AC_ARG_ENABLE($1,
    [  --enable-$1            $2],
    [
     pfk_build_desc_$1="$2"
     case $enableval in
      yes) 
        pfk_build_$1=yes
        PFK_BUILD=1
        ;;
      no)
        pfk_build_$1=no
	PFK_BUILD=0
        ;;
      *)
        AC_MSG_ERROR([valid args: --enable-$1 or --disable-$1])
        ;;
     esac],
    [
     pfk_build_$1=no
     PFK_BUILD=0
    ])
  AC_SUBST([PFK_BUILD_$1], [$PFK_BUILD])
  AM_CONDITIONAL([PFK_AM_BUILD_$1], [test $pfk_build_$1 = "yes"])
  m4_divert_text(PFK_NOTICES_DIVERSION, AC_MSG_NOTICE([$2: $pfk_build_$1]))
])

PFK_ARG_ENABLE([etg],        [build encrypted telnet gate tools])
PFK_ARG_ENABLE([pfkbak],     [build backup server])
PFK_ARG_ENABLE([syslog],     [build my syslog tools])
PFK_ARG_ENABLE([checksize],  [build checksize tool])
PFK_ARG_ENABLE([treescan],   [build treescan tool])
PFK_ARG_ENABLE([sudo],       [build my version of sudo])
PFK_ARG_ENABLE([runemacs],   [build cygwin runemacs])
PFK_ARG_ENABLE([runrxvt],    [build cygwin runrxvt])
PFK_ARG_ENABLE([ctwm],       [build claude's tab window manager])
PFK_ARG_ENABLE([pdksh],      [build public domain korn shell])
PFK_ARG_ENABLE([cscope],     [build cscope])
PFK_ARG_ENABLE([doxygen],    [build doxygen documentation])

# AC_USE_SYSTEM_EXTENSIONS

# AC_DEFINE(variable,value,description) -> $ac_cv_func_variable=value
#    @DEFS@

AC_PREFIX_DEFAULT([$HOME/pfk])

exec_prefix=$HOME/pfk/`$srcdir/scripts/architecture`

# Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_LEX
AC_PROG_YACC

AC_C_BIGENDIAN

# if ctwm building is enabled...
if test $pfk_build_ctwm = "yes" ; then
  AC_CHECK_PROG(pfk_have_xmkmf, xmkmf, yes, no)
  if test $pfk_have_xmkmf = "no" ; then
    AC_MSG_ERROR([xmkmf is required to build ctwm], [1])
  fi
fi

AC_PATH_PROG(pfk_shell_path1, bash, no)
if test $pfk_shell_path1 = "no" ; then
  AC_PATH_PROG(pfk_shell_path2, sh, no)
  pfk_shell_path=$pfk_shell_path2
else
  pfk_shell_path=$pfk_shell_path1
fi

AC_SUBST([PFK_SHELL_PATH], [$pfk_shell_path])

# Checks for libraries.
AC_SEARCH_LIBS([pthread_create], [pthread], [],
   [AC_MSG_ERROR([pthreads is required for pfkutils])], [])
AC_SEARCH_LIBS([clock_gettime], [rt], [],
   [AC_MSG_ERROR([clock_gettime is required for pfkutils])], [])
AC_SEARCH_LIBS([compress2], [z], [],
   [AC_MSG_ERROR([libz is required for pfkutils])], [])
AC_SEARCH_LIBS([wrefresh], [ncurses curses], [],
   [AC_MSG_ERROR([ncurses is required for pfkutils])], [])
AC_SEARCH_LIBS([accept], [nsl socket], [], [], [])
AC_SEARCH_LIBS([listen], [nsl socket], [], [], [])
AC_SEARCH_LIBS([setsockopt], [nsl socket], [], [], [])
AC_SEARCH_LIBS([bind], [nsl socket], [], [], [])
AC_SEARCH_LIBS([gethostbyname], [nsl], [], [], [])

# Checks for header files.

AC_CHECK_HEADERS([curses.h ncurses.h ncurses/curses.h])

# xxx update this whole area

#    AH_TEMPLATE(key, description) ?
# AC_PATH_X
# AC_FUNC_ALLOCA
# AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h langinfo.h libintl.h limits.h malloc.h netdb.h netinet/in.h stddef.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/mount.h sys/param.h sys/socket.h sys/time.h unistd.h wchar.h wctype.h])

if test $pfk_build_ctwm = "yes" ; then
  AC_CHECK_HEADERS([X11/Intrinsic.h], [],
    [AC_MSG_ERROR([ctwm requires X11 header files (libX11-devel libICE-devel libSM-devel libXau-devel libXaw-devel)])])
fi

# Checks for typedefs, structures, and compiler characteristics.
# AC_HEADER_STDBOOL
# AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
# AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
# AC_CHECK_MEMBERS([struct stat.st_blksize])
# AC_CHECK_MEMBERS([struct stat.st_rdev])
# AC_STRUCT_ST_BLOCKS
# AC_STRUCT_DIRENT_D_INO
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_OFF_T
AC_STRUCT_DIRENT_D_TYPE
AC_TYPE_UINTPTR_T

AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(int)

# Checks for library functions.
# AC_FUNC_CHOWN
# AC_FUNC_ERROR_AT_LINE
# AC_FUNC_FORK
# AC_FUNC_FSEEKO
# AC_FUNC_MMAP
# AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
# AC_FUNC_MALLOC
# AC_FUNC_MBRTOWC
# AC_FUNC_REALLOC
AC_CHECK_FUNCS([btowc bzero clock_gettime dup2 ftruncate gethostbyaddr gethostbyname gethostname gettimeofday isascii localtime_r memmove memset mkdir pow putenv re_comp regcomp rmdir select setlocale socket sqrt strchr strdup strerror strncasecmp strrchr strstr strtoul inet_aton lseek64 _lseek64 lseek])

AC_CHECK_FUNC([regexec], [pfk_need_regex=false], [pfk_need_regex=true])

AM_CONDITIONAL([PFK_NEED_REGEX], [test $pfk_need_regex = "true"])

# checks for system services
AC_SYS_LARGEFILE

AC_CONFIG_HEADERS([pfkutils_config.h])

if test -f /usr/xpg4/bin/id ; then
    AC_SUBST([PFK_USER_ID], [`/usr/xpg4/bin/id -u`])
    AC_SUBST([PFK_GROUP_ID], [`/usr/xpg4/bin/id -g`])
else
    AC_SUBST([PFK_USER_ID], [`id -u`])
    AC_SUBST([PFK_GROUP_ID], [`id -g`])
fi

AC_SUBST([PFK_USER_NAME], [`whoami`])

AC_CONFIG_FILES([ 
    pfkutils_version.h
    Makefile
    libpfkdll2/Makefile
    libpfkdll2/Doxyfile
    libpfkfb/Makefile
    libpfkfb/Doxyfile
    libpfkthread/Makefile
    libpfkthread/Doxyfile
    libpfkutil/Makefile
    libpfkutil/Doxyfile
    i2/Makefile
    syslog/Makefile
    backup/Makefile
    teld/Makefile
    enctelgate/Makefile
    main/Makefile
    main/programs.h
    main/sudo.h
    contrib/Makefile
    environ/profile
    environ/kshrc
    environ/Makefile
    scripts/emacs-lone
    scripts/myshell-launcher
    scripts/cs
    scripts/myemacs-cscope-helper
    scripts/myshell
    scripts/pf_frontend
    scripts/Makefile
])

#  AC_CONFIG_SUBDIRS([contrib/cscope-15.7a])
#  AC_CONFIG_SUBDIRS([contrib/pdksh-5.2.14])

m4_undivert(PFK_NOTICES_DIVERSION)

# custom commands added to config.status
# AC_CONFIG_COMMANDS([ ? ])

AC_OUTPUT
