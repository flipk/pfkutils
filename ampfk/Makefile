ARCH = `$(HOME)/pfk/bin/architecture`
noinst_PROGRAMS = ampfk-debug ampfk-final

CC = gcc
CFLAGS = 
CXX = g++
CXXFLAGS = 
CPPFLAGS = -I$(srcdir)
AR = ar
LEX = flex
YACC = yacc
LFLAGS = 
YFLAGS = 
ampfk-debug_CC = $(CC)
ampfk-debug_CXX = $(CXX)
ampfk-debug_LDADD = 
ampfk-debug_LDFLAGS = $(HOME)/pfk/$(ARCH)/lib/libpfkdll2.a -g3
ampfk-debug_CFLAGS = -g3
ampfk-debug_CXXFLAGS = -g3
ampfk-debug_CPPFLAGS = -I$(HOME)/pfk/include/dll2
ampfk-final_CC = $(CC)
ampfk-final_CXX = $(CXX)
ampfk-final_LDADD = 
ampfk-final_LDFLAGS = $(HOME)/pfk/$(ARCH)/lib/libpfkdll2.a
ampfk-final_CFLAGS = -O6
ampfk-final_CXXFLAGS = -O6
ampfk-final_CPPFLAGS = -I$(HOME)/pfk/include/dll2
srcdir = /home/flipk/proj/pfkutils/ampfk
builddir = /home/flipk/proj/pfkutils/ampfk

all: xmakefile
	+make $(MAKEJOBS) -f xmakefile ampfk-debug ampfk-final

xmakefile: Makefile ampfk-debug-parser.d ampfk-debug-tokenizer.d ampfk-debug-automake_parser.d ampfk-debug-automake_ostream.d ampfk-debug-automake_findtargets.d ampfk-debug-automake_rules.d ampfk-debug-automake_variables.d ampfk-debug-automake_utilities.d ampfk-debug-automake_output.d ampfk-debug-condition.d ampfk-debug-main.d ampfk-final-parser.d ampfk-final-tokenizer.d ampfk-final-automake_parser.d ampfk-final-automake_ostream.d ampfk-final-automake_findtargets.d ampfk-final-automake_rules.d ampfk-final-automake_variables.d ampfk-final-automake_utilities.d ampfk-final-automake_output.d ampfk-final-condition.d ampfk-final-main.d
	rm -f xmakefile xmakefile-tmp
	cat Makefile > xmakefile-tmp
	echo '' >> xmakefile-tmp
	cat ampfk-debug-parser.d ampfk-debug-tokenizer.d ampfk-debug-automake_parser.d ampfk-debug-automake_ostream.d ampfk-debug-automake_findtargets.d ampfk-debug-automake_rules.d ampfk-debug-automake_variables.d ampfk-debug-automake_utilities.d ampfk-debug-automake_output.d ampfk-debug-condition.d ampfk-debug-main.d ampfk-final-parser.d ampfk-final-tokenizer.d ampfk-final-automake_parser.d ampfk-final-automake_ostream.d ampfk-final-automake_findtargets.d ampfk-final-automake_rules.d ampfk-final-automake_variables.d ampfk-final-automake_utilities.d ampfk-final-automake_output.d ampfk-final-condition.d ampfk-final-main.d >> xmakefile-tmp
	mv xmakefile-tmp xmakefile

Makefile: /home/flipk/proj/pfkutils/ampfk/Makefile-test.am
	ampfk /home/flipk/proj/pfkutils/ampfk/Makefile-test.am
	make clean

ampfk-debug-parser.d: ampfk-debug-parser.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -MT ' ampfk-debug-parser.o ampfk-debug-parser.d ' -M -MF ampfk-debug-parser.d ampfk-debug-parser.cc

ampfk-debug-tokenizer.d: ampfk-debug-tokenizer.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -MT ' ampfk-debug-tokenizer.o ampfk-debug-tokenizer.d ' -M -MF ampfk-debug-tokenizer.d ampfk-debug-tokenizer.cc

ampfk-debug-automake_parser.d: $(srcdir)/automake_parser.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -MT ' ampfk-debug-automake_parser.o ampfk-debug-automake_parser.d ' -M -MF ampfk-debug-automake_parser.d $(srcdir)/automake_parser.cc

ampfk-debug-automake_ostream.d: $(srcdir)/automake_ostream.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -MT ' ampfk-debug-automake_ostream.o ampfk-debug-automake_ostream.d ' -M -MF ampfk-debug-automake_ostream.d $(srcdir)/automake_ostream.cc

ampfk-debug-automake_findtargets.d: $(srcdir)/automake_findtargets.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -MT ' ampfk-debug-automake_findtargets.o ampfk-debug-automake_findtargets.d ' -M -MF ampfk-debug-automake_findtargets.d $(srcdir)/automake_findtargets.cc

ampfk-debug-automake_rules.d: $(srcdir)/automake_rules.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -MT ' ampfk-debug-automake_rules.o ampfk-debug-automake_rules.d ' -M -MF ampfk-debug-automake_rules.d $(srcdir)/automake_rules.cc

ampfk-debug-automake_variables.d: $(srcdir)/automake_variables.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -MT ' ampfk-debug-automake_variables.o ampfk-debug-automake_variables.d ' -M -MF ampfk-debug-automake_variables.d $(srcdir)/automake_variables.cc

ampfk-debug-automake_utilities.d: $(srcdir)/automake_utilities.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -MT ' ampfk-debug-automake_utilities.o ampfk-debug-automake_utilities.d ' -M -MF ampfk-debug-automake_utilities.d $(srcdir)/automake_utilities.cc

ampfk-debug-automake_output.d: $(srcdir)/automake_output.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -MT ' ampfk-debug-automake_output.o ampfk-debug-automake_output.d ' -M -MF ampfk-debug-automake_output.d $(srcdir)/automake_output.cc

ampfk-debug-condition.d: $(srcdir)/condition.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -MT ' ampfk-debug-condition.o ampfk-debug-condition.d ' -M -MF ampfk-debug-condition.d $(srcdir)/condition.cc

ampfk-debug-main.d: $(srcdir)/main.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -MT ' ampfk-debug-main.o ampfk-debug-main.d ' -M -MF ampfk-debug-main.d $(srcdir)/main.cc

ampfk-final-parser.d: ampfk-final-parser.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -MT ' ampfk-final-parser.o ampfk-final-parser.d ' -M -MF ampfk-final-parser.d ampfk-final-parser.cc

ampfk-final-tokenizer.d: ampfk-final-tokenizer.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -MT ' ampfk-final-tokenizer.o ampfk-final-tokenizer.d ' -M -MF ampfk-final-tokenizer.d ampfk-final-tokenizer.cc

ampfk-final-automake_parser.d: $(srcdir)/automake_parser.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -MT ' ampfk-final-automake_parser.o ampfk-final-automake_parser.d ' -M -MF ampfk-final-automake_parser.d $(srcdir)/automake_parser.cc

ampfk-final-automake_ostream.d: $(srcdir)/automake_ostream.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -MT ' ampfk-final-automake_ostream.o ampfk-final-automake_ostream.d ' -M -MF ampfk-final-automake_ostream.d $(srcdir)/automake_ostream.cc

ampfk-final-automake_findtargets.d: $(srcdir)/automake_findtargets.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -MT ' ampfk-final-automake_findtargets.o ampfk-final-automake_findtargets.d ' -M -MF ampfk-final-automake_findtargets.d $(srcdir)/automake_findtargets.cc

ampfk-final-automake_rules.d: $(srcdir)/automake_rules.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -MT ' ampfk-final-automake_rules.o ampfk-final-automake_rules.d ' -M -MF ampfk-final-automake_rules.d $(srcdir)/automake_rules.cc

ampfk-final-automake_variables.d: $(srcdir)/automake_variables.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -MT ' ampfk-final-automake_variables.o ampfk-final-automake_variables.d ' -M -MF ampfk-final-automake_variables.d $(srcdir)/automake_variables.cc

ampfk-final-automake_utilities.d: $(srcdir)/automake_utilities.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -MT ' ampfk-final-automake_utilities.o ampfk-final-automake_utilities.d ' -M -MF ampfk-final-automake_utilities.d $(srcdir)/automake_utilities.cc

ampfk-final-automake_output.d: $(srcdir)/automake_output.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -MT ' ampfk-final-automake_output.o ampfk-final-automake_output.d ' -M -MF ampfk-final-automake_output.d $(srcdir)/automake_output.cc

ampfk-final-condition.d: $(srcdir)/condition.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -MT ' ampfk-final-condition.o ampfk-final-condition.d ' -M -MF ampfk-final-condition.d $(srcdir)/condition.cc

ampfk-final-main.d: $(srcdir)/main.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -MT ' ampfk-final-main.o ampfk-final-main.d ' -M -MF ampfk-final-main.d $(srcdir)/main.cc

ampfk-debug: ampfk-debug-parser.o ampfk-debug-tokenizer.o ampfk-debug-automake_parser.o ampfk-debug-automake_ostream.o ampfk-debug-automake_findtargets.o ampfk-debug-automake_rules.o ampfk-debug-automake_variables.o ampfk-debug-automake_utilities.o ampfk-debug-automake_output.o ampfk-debug-condition.o ampfk-debug-main.o $(ampfk-debug_LDADD)
	rm -f ampfk-debug
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) -o ampfk-debug  ampfk-debug-parser.o ampfk-debug-tokenizer.o ampfk-debug-automake_parser.o ampfk-debug-automake_ostream.o ampfk-debug-automake_findtargets.o ampfk-debug-automake_rules.o ampfk-debug-automake_variables.o ampfk-debug-automake_utilities.o ampfk-debug-automake_output.o ampfk-debug-condition.o ampfk-debug-main.o $(ampfk-debug_LDADD) $(ampfk-debug_LDFLAGS)

ampfk-final: ampfk-final-parser.o ampfk-final-tokenizer.o ampfk-final-automake_parser.o ampfk-final-automake_ostream.o ampfk-final-automake_findtargets.o ampfk-final-automake_rules.o ampfk-final-automake_variables.o ampfk-final-automake_utilities.o ampfk-final-automake_output.o ampfk-final-condition.o ampfk-final-main.o $(ampfk-final_LDADD)
	rm -f ampfk-final
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) -o ampfk-final  ampfk-final-parser.o ampfk-final-tokenizer.o ampfk-final-automake_parser.o ampfk-final-automake_ostream.o ampfk-final-automake_findtargets.o ampfk-final-automake_rules.o ampfk-final-automake_variables.o ampfk-final-automake_utilities.o ampfk-final-automake_output.o ampfk-final-condition.o ampfk-final-main.o $(ampfk-final_LDADD) $(ampfk-final_LDFLAGS)

ampfk-debug-parser.o: ampfk-debug-parser.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -c ampfk-debug-parser.cc -o ampfk-debug-parser.o

ampfk-debug-tokenizer.o: ampfk-debug-tokenizer.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -c ampfk-debug-tokenizer.cc -o ampfk-debug-tokenizer.o

ampfk-debug-automake_parser.o: $(srcdir)/automake_parser.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -c $(srcdir)/automake_parser.cc -o ampfk-debug-automake_parser.o

ampfk-debug-automake_ostream.o: $(srcdir)/automake_ostream.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -c $(srcdir)/automake_ostream.cc -o ampfk-debug-automake_ostream.o

ampfk-debug-automake_findtargets.o: $(srcdir)/automake_findtargets.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -c $(srcdir)/automake_findtargets.cc -o ampfk-debug-automake_findtargets.o

ampfk-debug-automake_rules.o: $(srcdir)/automake_rules.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -c $(srcdir)/automake_rules.cc -o ampfk-debug-automake_rules.o

ampfk-debug-automake_variables.o: $(srcdir)/automake_variables.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -c $(srcdir)/automake_variables.cc -o ampfk-debug-automake_variables.o

ampfk-debug-automake_utilities.o: $(srcdir)/automake_utilities.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -c $(srcdir)/automake_utilities.cc -o ampfk-debug-automake_utilities.o

ampfk-debug-automake_output.o: $(srcdir)/automake_output.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -c $(srcdir)/automake_output.cc -o ampfk-debug-automake_output.o

ampfk-debug-condition.o: $(srcdir)/condition.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -c $(srcdir)/condition.cc -o ampfk-debug-condition.o

ampfk-debug-main.o: $(srcdir)/main.cc
	$(ampfk-debug_CXX) $(CXXFLAGS) $(ampfk-debug_CXXFLAGS) $(CPPFLAGS) $(ampfk-debug_CPPFLAGS) -c $(srcdir)/main.cc -o ampfk-debug-main.o

ampfk-final-parser.o: ampfk-final-parser.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -c ampfk-final-parser.cc -o ampfk-final-parser.o

ampfk-final-tokenizer.o: ampfk-final-tokenizer.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -c ampfk-final-tokenizer.cc -o ampfk-final-tokenizer.o

ampfk-final-automake_parser.o: $(srcdir)/automake_parser.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -c $(srcdir)/automake_parser.cc -o ampfk-final-automake_parser.o

ampfk-final-automake_ostream.o: $(srcdir)/automake_ostream.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -c $(srcdir)/automake_ostream.cc -o ampfk-final-automake_ostream.o

ampfk-final-automake_findtargets.o: $(srcdir)/automake_findtargets.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -c $(srcdir)/automake_findtargets.cc -o ampfk-final-automake_findtargets.o

ampfk-final-automake_rules.o: $(srcdir)/automake_rules.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -c $(srcdir)/automake_rules.cc -o ampfk-final-automake_rules.o

ampfk-final-automake_variables.o: $(srcdir)/automake_variables.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -c $(srcdir)/automake_variables.cc -o ampfk-final-automake_variables.o

ampfk-final-automake_utilities.o: $(srcdir)/automake_utilities.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -c $(srcdir)/automake_utilities.cc -o ampfk-final-automake_utilities.o

ampfk-final-automake_output.o: $(srcdir)/automake_output.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -c $(srcdir)/automake_output.cc -o ampfk-final-automake_output.o

ampfk-final-condition.o: $(srcdir)/condition.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -c $(srcdir)/condition.cc -o ampfk-final-condition.o

ampfk-final-main.o: $(srcdir)/main.cc
	$(ampfk-final_CXX) $(CXXFLAGS) $(ampfk-final_CXXFLAGS) $(CPPFLAGS) $(ampfk-final_CPPFLAGS) -c $(srcdir)/main.cc -o ampfk-final-main.o

ampfk-debug-parser.cc parser.h: $(srcdir)/parser.Y
	rm -f ampfk-debug-parser.cc parser.h
	$(YACC) $(ampfk-debug_YFLAGS) -d $(srcdir)/parser.Y
	mv y.tab.c ampfk-debug-parser.cc
	mv y.tab.h parser.h

ampfk-debug-tokenizer.cc: $(srcdir)/tokenizer.L
	rm -f ampfk-debug-tokenizer.cc
	$(LEX) $(ampfk-debug_LFLAGS) $(srcdir)/tokenizer.L
	mv lex.yy.c ampfk-debug-tokenizer.cc

ampfk-final-parser.cc parser.h: $(srcdir)/parser.Y
	rm -f ampfk-final-parser.cc parser.h
	$(YACC) $(ampfk-final_YFLAGS) -d $(srcdir)/parser.Y
	mv y.tab.c ampfk-final-parser.cc
	mv y.tab.h parser.h

ampfk-final-tokenizer.cc: $(srcdir)/tokenizer.L
	rm -f ampfk-final-tokenizer.cc
	$(LEX) $(ampfk-final_LFLAGS) $(srcdir)/tokenizer.L
	mv lex.yy.c ampfk-final-tokenizer.cc

clean: 
	rm -f xmakefile *.d *.o ampfk-debug ampfk-debug-parser.cc parser.h ampfk-debug-tokenizer.cc ampfk-final ampfk-final-parser.cc parser.h ampfk-final-tokenizer.cc

