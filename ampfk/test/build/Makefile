bin_PROGRAMS = one
noinst_PROGRAMS = two
noinst_LIBRARIES = libthree.a libfour.a

CC = gcc
CFLAGS = -Wall
CXX = g++
CXXFLAGS = -Wall
CPPFLAGS = -DCRAP -I$(srcdir)
AR = ar
LEX = flex
YACC = yacc
YFLAGS = -d
two_LDADD = libthree.a libfour.a
two_LDFLAGS = -L/usr/lib -lm
one_LDADD = libfour.a
srcdir = ..

all: xmakefile
	make -f xmakefile libthree.a libfour.a two one

xmakefile: Makefile libthree_a-three.d libfour_a-four.d libfour_a-one.d libfour_a-two.d two-two.d one-one.d
	rm -f xmakefile xmakefile-tmp
	cat Makefile > xmakefile-tmp
	echo '' >> xmakefile-tmp
	cat libthree_a-three.d libfour_a-four.d libfour_a-one.d libfour_a-two.d two-two.d one-one.d >> xmakefile-tmp
	mv xmakefile-tmp xmakefile

libthree_a-three.d: $(srcdir)/three.cc
	$(CXX) $(CXXFLAGS) $(libthree_a_CXXFLAGS) $(CPPFLAGS) $(libthree_a_CPPFLAGS) -MT ' libthree_a-three.o libthree_a-three.d ' -M -MF libthree_a-three.d $(srcdir)/three.cc

libfour_a-four.d: $(srcdir)/four.c
	$(CC) $(CFLAGS) $(libfour_a_CFLAGS) $(CPPFLAGS) $(libfour_a_CPPFLAGS) -MT ' libfour_a-four.o libfour_a-four.d ' -M -MF libfour_a-four.d $(srcdir)/four.c

libfour_a-one.d: $(srcdir)/one.c
	$(CC) $(CFLAGS) $(libfour_a_CFLAGS) $(CPPFLAGS) $(libfour_a_CPPFLAGS) -MT ' libfour_a-one.o libfour_a-one.d ' -M -MF libfour_a-one.d $(srcdir)/one.c

libfour_a-two.d: $(srcdir)/two.cc
	$(CXX) $(CXXFLAGS) $(libfour_a_CXXFLAGS) $(CPPFLAGS) $(libfour_a_CPPFLAGS) -MT ' libfour_a-two.o libfour_a-two.d ' -M -MF libfour_a-two.d $(srcdir)/two.cc

two-two.d: $(srcdir)/two.cc
	$(CXX) $(CXXFLAGS) $(two_CXXFLAGS) $(CPPFLAGS) $(two_CPPFLAGS) -MT ' two-two.o two-two.d ' -M -MF two-two.d $(srcdir)/two.cc

one-one.d: $(srcdir)/one.c
	$(CC) $(CFLAGS) $(one_CFLAGS) $(CPPFLAGS) $(one_CPPFLAGS) -MT ' one-one.o one-one.d ' -M -MF one-one.d $(srcdir)/one.c

libthree.a: libthree_a-three.o $(libthree_a_LDADD)
	rm -f libthree.a
	$(AR) cq libthree.a  libthree_a-three.o $(libthree_a_LDADD) $(libthree_a_LDFLAGS)

libfour.a: libfour_a-four.o libfour_a-one.o libfour_a-two.o $(libfour_a_LDADD)
	rm -f libfour.a
	$(AR) cq libfour.a  libfour_a-four.o libfour_a-one.o libfour_a-two.o $(libfour_a_LDADD) $(libfour_a_LDFLAGS)

two: two-two.o $(two_LDADD)
	rm -f two
	$(CXX) $(CXXFLAGS) $(two_CXXFLAGS) -o two  two-two.o $(two_LDADD) $(two_LDFLAGS)

one: one-one.o $(one_LDADD)
	rm -f one
	$(CC) $(CFLAGS) $(one_CFLAGS) -o one  one-one.o $(one_LDADD) $(one_LDFLAGS)

libthree_a-three.o: $(srcdir)/three.cc
	$(CXX) $(CXXFLAGS) $(libthree_a_CXXFLAGS) $(CPPFLAGS) $(libthree_a_CPPFLAGS) -c $(srcdir)/three.cc -o libthree_a-three.o

libfour_a-four.o: $(srcdir)/four.c
	$(CC) $(CFLAGS) $(libfour_a_CFLAGS) $(CPPFLAGS) $(libfour_a_CPPFLAGS) -c $(srcdir)/four.c -o libfour_a-four.o

libfour_a-one.o: $(srcdir)/one.c
	$(CC) $(CFLAGS) $(libfour_a_CFLAGS) $(CPPFLAGS) $(libfour_a_CPPFLAGS) -c $(srcdir)/one.c -o libfour_a-one.o

libfour_a-two.o: $(srcdir)/two.cc
	$(CXX) $(CXXFLAGS) $(libfour_a_CXXFLAGS) $(CPPFLAGS) $(libfour_a_CPPFLAGS) -c $(srcdir)/two.cc -o libfour_a-two.o

two-two.o: $(srcdir)/two.cc
	$(CXX) $(CXXFLAGS) $(two_CXXFLAGS) $(CPPFLAGS) $(two_CPPFLAGS) -c $(srcdir)/two.cc -o two-two.o

one-one.o: $(srcdir)/one.c
	$(CC) $(CFLAGS) $(one_CFLAGS) $(CPPFLAGS) $(one_CPPFLAGS) -c $(srcdir)/one.c -o one-one.o

clean: 
	rm -f xmakefile *.d *.o libthree.a libfour.a two one

