
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = \
	contrib \
	libpfkdll2 \
	libpfkutil \
	libpfkfb \
	libpfkthread

SUBDIRS += i2

if PFK_AM_BUILD_pfkbak
SUBDIRS += backup
endif

if PFK_AM_BUILD_pfksh
SUBDIRS += pfksh
endif

if PFK_AM_BUILD_syslog
SUBDIRS += syslog
endif

if PFK_AM_BUILD_ampfk
SUBDIRS += ampfk
endif

if PFK_AM_BUILD_webappserver
SUBDIRS += libWebAppServer
endif

SUBDIRS += main environ scripts

CONFIG_HDR_DEST = $(DESTDIR)$(exec_prefix)/include
CONFIG_LOG_DEST = $(DESTDIR)$(exec_prefix)/share/pfkutils

install-data-hook:
	mkdir -p $(CONFIG_HDR_DEST) $(CONFIG_LOG_DEST) || true
	cp $(CONFIG_HEADER) $(CONFIG_HDR_DEST)/$(CONFIG_HEADER)
	chmod 644 $(CONFIG_HDR_DEST)/$(CONFIG_HEADER)
	cp pfkutils_version.h $(CONFIG_HDR_DEST)/pfkutils_version.h
	chmod 644 $(CONFIG_HDR_DEST)/pfkutils_version.h
	cp config.log config.status $(CONFIG_LOG_DEST)/
	chmod 644 $(CONFIG_LOG_DEST)/config.log
	chmod 644 $(CONFIG_LOG_DEST)/config.status
if PFK_AM_BUILD_sudo
	@echo ""
	@echo "*** also, if not already done, perform the following as root:"
	@echo "     chown root $(bindir)/pfkutils"
	@echo "     chmod 4755 $(bindir)/pfkutils"
	@echo ""
endif

diffdotfiles:
	@cd environ && make diffdotfiles
	@cd scripts && make diffdotfiles

CVSTAR= pfkutils_cvs-`date +%Y-%m%d`.tar.bz2
cvstar:
	cd /home/flipk/Dropbox/Public && tar jcf $(abs_builddir)/$(CVSTAR) philcvs/CVSROOT philcvs/pfkutils
	cd $(abs_builddir) && ls -l $(CVSTAR)
