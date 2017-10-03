
CC=gcc
CFLAGS=
CXX=g++
CXXFLAGS=
CPPFLAGS= 
AR=ar
LEX=flex
YACC=yacc
LFLAGS=
YFLAGS=
ARCH= `$(HOME)/pfk/bin/architecture`

noinst_PROGRAMS = ampfk-debug ampfk-final

ampfk-final_SOURCES = parser.Y tokenizer.L \
	automake_parser.cc automake_ostream.cc automake_findtargets.cc \
	automake_rules.cc automake_variables.cc automake_utilities.cc \
	automake_output.cc condition.cc   main.cc
ampfk-final_HEADERS = automake_parser.H condition.H tokenizer.H
ampfk-final_CPPFLAGS = -I$(HOME)/pfk/include/dll2
ampfk-final_CFLAGS = -O6
ampfk-final_CXXFLAGS = -O6
ampfk-final_LDADD = 
ampfk-final_LDFLAGS = $(HOME)/pfk/$(ARCH)/lib/libpfkdll2.a

ampfk-debug_SOURCES = parser.Y tokenizer.L \
	automake_parser.cc automake_ostream.cc automake_findtargets.cc \
	automake_rules.cc automake_variables.cc automake_utilities.cc \
	automake_output.cc condition.cc   main.cc
ampfk-debug_HEADERS = automake_parser.H condition.H tokenizer.H
ampfk-debug_CPPFLAGS = -I$(HOME)/pfk/include/dll2
ampfk-debug_CFLAGS = -g3
ampfk-debug_CXXFLAGS = -g3
ampfk-debug_LDADD = 
ampfk-debug_LDFLAGS = $(HOME)/pfk/$(ARCH)/lib/libpfkdll2.a -g3
