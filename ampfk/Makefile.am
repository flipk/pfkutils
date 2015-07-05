
noinst_LIBRARIES = libampfk.a

libampfk_a_SOURCES = parser.yy tokenizer.ll \
        automake_parser.cc automake_ostream.cc automake_findtargets.cc \
        automake_rules.cc automake_variables.cc automake_utilities.cc \
        automake_output.cc condition.cc   main.cc \
	automake_parser.h condition.h tokenizer.h

libampfk_a_CPPFLAGS = -I$(top_srcdir)/libpfkdll2

AM_YFLAGS = -d
