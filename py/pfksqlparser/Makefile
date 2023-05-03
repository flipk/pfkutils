
DEBUG_H_MACRO_GENERATION=y

OBJDIR = obj

LIB_TARGETS = pfksqlparser

pfksqlparser_TARGET = $(OBJDIR)/pfksqlparser.so
pfksqlparser_LLSRCS = pfksql_tokenizer.ll
pfksqlparser_YYSRCS = pfksql_parser.yy
pfksqlparser_INCS = -I.
pfksqlparser_DEFS = \
	-DPARSER_YY_HDR=\"$(pfksqlparser_pfksql_parser.yy_HDR)\" \
	-DTOKENIZER_LL_HDR=\"$(pfksqlparser_pfksql_tokenizer.ll_HDR)\"
pfksqlparser_CXXFLAGS = -fPIC -fasynchronous-unwind-tables -fexceptions
pfksqlparser_CXXSRCS = pfksql_tokenize_and_parse.cc pfksqlparsermodule.cc
pfksqlparser_AR = g++
pfksqlparser_ARCMD = \
	-shared \
	-Wl,-z,relro \
	-Wl,--as-needed \
	-Wl,-z,now \
	-Wl,--build-id=sha1 -o
pfksqlparser_RANLIB = true

PROG_TARGETS = test

test_TARGET = $(OBJDIR)/test
test_CXXSRCS = main.cc
test_LIBS = $(OBJDIR)/pfksqlparser.so -lpython3

include ../../Makefile.inc
