
PROG= t
HDRS= automake_parser.H condition.H parser.H tokenizer.H
SRCS= automake_parser.cc automake_ostream.cc automake_findtargets.cc \
	automake_rules.cc automake_variables.cc automake_utilities.cc \
	automake_output.cc condition.cc parser.cc tokenizer.cc main.cc
OBJS= $(SRCS:.cc=.o)
DEPS= $(SRCS:.cc=.d)
ARCH= `$(HOME)/pfk/bin/architecture`
LIBS= $(HOME)/pfk/$(ARCH)/lib/libpfkdll2.a
LEX= flex
YACC= yacc
CC= gcc
CFLAGS= -g3 -Wall
CXX= g++
CXXFLAGS= -g3 -Wall
CPPFLAGS= -I$(HOME)/pfk/include/dll2

.SUFFIXES: .d

all: xmakefile
	+make -f xmakefile $(PROG)

xmakefile: Makefile $(DEPS)
	rm -f xmakefile xmakefile-tmp
	cat Makefile > xmakefile-tmp
	echo '' >> xmakefile-tmp
	cat $(DEPS) >> xmakefile-tmp
	mv xmakefile-tmp xmakefile

$(PROG): $(OBJS)
	$(CXX) -o $(PROG) $(OBJS) $(LIBS)

parser.cc parser.h : parser.Y
	rm -f parser.cc parser.h
	$(YACC) -d parser.Y
	mv y.tab.c parser.cc
	mv y.tab.h parser.h

tokenizer.cc : parser.h tokenizer.L
	$(LEX) tokenizer.L
	mv lex.yy.c tokenizer.cc

.cc.d:
	$(CXX) $(CPPFLAGS) -MT $*.o -M -MF $*.d $*.cc

.cc.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f *~ $(OBJS) $(DEPS) $(PROG) parser.cc parser.h tokenizer.cc
	rm -f xmakefile xmakefile-tmp
