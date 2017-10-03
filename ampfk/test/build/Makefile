
CC=gcc
CFLAGS=-Wall
CXX=g++
CXXFLAGS=-Wall
CPPFLAGS=
AR=ar
one_LDADD = libfour.a
two_LDADD = libfour.a libthree.a
two_LDFLAGS = -L/usr/lib -lm
one_CPPFLAGS= -DCRAP

srcdir=..

all: xmakefile
	make -f xmakefile libthree.a libfour.a two one

xmakefile: Makefile libthree_a-three.d libfour_a-four.d two-two.d one-one.d
	rm -f xmakefile xmakefile-tmp
	cat Makefile > xmakefile-tmp
	echo '' >> xmakefile-tmp
	cat libthree_a-three.d libfour_a-four.d two-two.d one-one.d >> xmakefile-tmp
	mv xmakefile-tmp xmakefile

libthree_a-three.d: $(srcdir)/three.cc $(srcdir)/three.h
	$(CXX) $(AM_INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(three_CPPFLAGS) $(three_CXXFLAGS) -MT libthree_a-three.o -M -MF libthree_a-three.d $(srcdir)/three.cc

libfour_a-four.d: $(srcdir)/four.c $(srcdir)/four.h
	$(CC) $(AM_INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(four_CPPFLAGS) $(four_CFLAGS) -MT libfour_a-four.o -M -MF libfour_a-four.d $(srcdir)/four.c

two-two.d: $(srcdir)/two.cc 
	$(CXX) $(AM_INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(two_CPPFLAGS) $(two_CPPFLAGS) -MT two-two.o -M -MF two-two.d $(srcdir)/two.cc

one-one.d: $(srcdir)/one.c $(srcdir)/one.h
	$(CC) $(AM_INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(one_CPPFLAGS) $(one_CFLAGS) -MT one-one.o -M -MF one-one.d $(srcdir)/one.c

libthree.a: libthree_a-three.o
	rm -f libthree.a
	$(AR) cq libthree.a libthree_a-three.o

libfour.a: libfour_a-four.o
	rm -f libfour.a
	$(AR) cq libfour.a libfour_a-four.o

two: two-two.o libfour.a libthree.a
	$(CXX) -o two $(two_LDFLAGS) two-two.o $(two_LDADD)

one: one-one.o libfour.a
	$(CXX) -o one $(one_LDFLAGS) one-one.o $(one_LDADD)

libthree_a-three.o: $(srcdir)/three.cc
	$(CXX) $(AM_INCLUDES) $(AM_CPPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(three_CPPFLAGS) $(three_CXXFLAGS) -c $(srcdir)/three.cc -o libthree_a-three.o

libfour_a-four.o: $(srcdir)/four.c
	$(CC) $(AM_INCLUDES) $(AM_CPPFLAGS) $(CFLAGS) $(CPPFLAGS) $(four_CPPFLAGS) $(four_CFLAGS) -c $(srcdir)/four.c -o libfour_a-four.o

two-two.o: $(srcdir)/two.cc
	$(CXX) $(AM_INCLUDES) $(AM_CPPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(two_CPPFLAGS) $(two_CPPFLAGS) -c $(srcdir)/two.cc -o two-two.o

one-one.o: $(srcdir)/one.c $(srcdir)/one.h
	$(CC) $(AM_INCLUDES) $(AM_CPPFLAGS) $(CFLAGS) $(CPPFLAGS) $(one_CPPFLAGS) $(one_CFLAGS) -c $(srcdir)/one.c -o one-one.o

clean:
	rm -f xmakefile *.d *.o libthree.a libfour.a two one
