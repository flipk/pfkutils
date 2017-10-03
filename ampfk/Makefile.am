
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

noinst_PROGRAMS = ampfk

ampfk_SOURCES = parser.Y tokenizer.L \
	automake_parser.cc automake_ostream.cc automake_findtargets.cc \
	automake_rules.cc automake_variables.cc automake_utilities.cc \
	condition.cc   main.cc

ampfk_HEADERS = automake_parser.H condition.H tokenizer.H

ampfk_CPPFLAGS = -I$(HOME)/pfk/include/dll2

ampfk_CFLAGS = -Wall
ampfk_CXXFLAGS = -Wall

ARCH= `/home/flipk/pfk/bin/architecture`
ampfk_LDFLAGS = $(HOME)/pfk/$(ARCH)/lib/libpfkdll2.a
