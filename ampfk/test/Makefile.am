
CC=gcc
CFLAGS=-Wall
CXX=g++
CXXFLAGS=-Wall
CXXFLAGS+=-pedantic
CPPFLAGS=
AR=ar
LEX=flex
YACC=yacc # or bison

bin_PROGRAMS = one
noinst_PROGRAMS = two
noinst_LIBRARIES = libthree.a libfour.a

one_SOURCES = one.c
one_HEADERS = one.h
one_LDADD = libfour.a
one_CPPFLAGS = 
one_CXXFLAGS = 
one_CFLAGS =
two_SOURCES = two.cc
two_LDADD = three.a
two_LDADD+= four.a
two_LDFLAGS = -L/usr/lib
two_LDFLAGS+= -lm

libthree_a_SOURCES = three.cc four_lex.L four_yacc.Y
libthree_a_HEADERS = three.h
libthree_a_YFLAGS = -d
libfour_a_SOURCES = four.c 
libfour_a_HEADERS = four.h

eight: four
	nine
	two

clean:
	thing1
	thing2
