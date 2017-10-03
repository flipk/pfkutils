
CC=gcc
CFLAGS=-Wall
CXX=g++
CXXFLAGS=-Wall
CPPFLAGS= -DCRAP
AR=ar
LEX=flex
YACC=yacc # or bison
YFLAGS=-d

bin_PROGRAMS = one
noinst_PROGRAMS = two
noinst_LIBRARIES = libthree.a libfour.a

one_SOURCES = one.c
one_HEADERS = one.h
one_LDADD = libfour.a
two_SOURCES = two.cc
two_LDADD = libthree.a
two_LDADD+= libfour.a
two_LDFLAGS = -L/usr/lib
two_LDFLAGS+= -lm

libthree_a_SOURCES = three.cc # four_lex.L four_yacc.Y
libthree_a_HEADERS = three.h
libfour_a_SOURCES = four.c  one.c two.cc
libfour_a_HEADERS = four.h  one.h

eight: four
	nine
	two

clean:
	thing1
	thing2
