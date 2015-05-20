
bin_PROGRAMS = pfkutils.new

pfkutils_new_SOURCES = \
	crnl.c he.c random-hex.c \
	rmstar_new.cc fmtsdiff.c \
	genpw.c xtermbar.c waitpid.c \
	pfkstat.c \
	deduplifier.cc \
	main.C programs.h

# treescan.c

pfkutils_new_LDADD = 

if PFK_AM_BUILD_pfkbak
pfkutils_new_LDADD += ../backup/libbackup.a
endif

if PFK_AM_BUILD_pfksh
pfkutils_new_LDADD += ../pfksh/libpfksh.a
endif

if PFK_AM_BUILD_sudo
pfkutils_new_SOURCES += sudo.c sudo.h
endif

if PFK_AM_BUILD_checksize
pfkutils_new_SOURCES += checksize.c
endif

if PFK_AM_BUILD_treescan
pfkutils_new_SOURCES += treescan.c
endif

pfkutils_new_SOURCES += btdump.C fbcompact.C fbdump.C

if PFK_AM_BUILD_syslog
pfkutils_new_LDADD += ../syslog/libsyslog.a
endif

if PFK_AM_BUILD_ampfk
pfkutils_new_LDADD += ../ampfk/libampfk.a
endif

pfkutils_new_LDADD += ../i2/libi2.a

pfkutils_new_LDADD += \
	../libpfkutil/libpfkutil.la \
	../libpfkfb/libpfkfb.la \
	../libpfkdll2/libpfkdll2.la \
	../libpfkthread/libpfkthread.la

AM_CPPFLAGS = \
	-I$(top_srcdir)/libpfkutil \
	-I$(top_srcdir)/libpfkdll2 \
	-I$(top_srcdir)/libpfkfb


install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
		$(STRIP) pfkutils.new
if PFK_AM_BUILD_sudo
	cd $(DESTDIR)$(bindir) && \
		if [ -x sudo ] ; then \
			./sudo mv pfkutils pfkutils.old  && \
			rm -f sudo && \
			ln -s pfkutils.old sudo && \
			mv pfkutils.new pfkutils && \
			./sudo chown root pfkutils && \
			./sudo chmod 4755 pfkutils && \
			rm -f pfkutils.old ; \
		else \
			mv pfkutils.new pfkutils ;  \
		fi
else
	set -e ; cd $(DESTDIR)$(bindir) && \
		mv pfkutils.new pfkutils
endif
	cd $(DESTDIR)$(bindir) && \
		./pfkutils -dellinks && \
		./pfkutils -makelinks
