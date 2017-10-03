
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = \
	libpfkdll2 \
	libpfkutil \
	libpfkfb \
	libpfkthread

SUBDIRS += i2

if PFK_AM_BUILD_pfkbak
SUBDIRS += backup
endif

if PFK_AM_BUILD_syslog
SUBDIRS += syslog
endif

if PFK_AM_BUILD_etg
SUBDIRS += teld enctelgate
endif

SUBDIRS += main contrib environ scripts

install-data-hook:
	cp $(builddir)/$(CONFIG_HEADER) $(DESTDIR)$(includedir)/$(CONFIG_HEADER)
	chmod 644 $(DESTDIR)$(includedir)/$(CONFIG_HEADER)
if PFK_AM_BUILD_sudo
	@echo ""
	@echo "*** also, if not already done, perform the following as root:"
	@echo "     chown root $(bindir)/pfkutils"
	@echo "     chmod 4755 $(bindir)/pfkutils"
	@echo ""
endif
