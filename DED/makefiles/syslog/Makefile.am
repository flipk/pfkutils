
pkgdatadir = $(datadir)/syslog

noinst_LIBRARIES = libsyslog.a

libsyslog_a_SOURCES = \
	config_file.c config_file.h \
	process_line.c process_line.h \
	rules.h \
	strip_chars.c strip_chars.h \
	syslog-postprocess.c syslogd.c

dist_pkgdata_DATA = \
	custom.ini \
	simple.ini \
	quiet.ini \
	sample.ini

AM_INCLUDES = -I../libpfkutil
