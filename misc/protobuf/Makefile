
OBJDIR= obj

LIB_TARGETS = protoparser

protoparser_TARGET = $(OBJDIR)/libprotoparser.a
protoparser_CXXSRCS = protobuf_tokenizer.cc protobuf_parser.cc

# pfkutils/Makefile.inc can actually do these for you, but i'm not doing
# that here to show you how to use flexbison.sh
#protoparser_LLSRCS = tokenizer.ll
#protoparser_YYSRCS = parser.yy
#protoparser_DEFS = -DPARSER_YY_HDR=\"protoparser-parser.hh\"

PROG_TARGETS = test

test_TARGET = $(OBJDIR)/test
test_CXXSRCS = main.cc
test_DEPLIBS = $(protoparser_TARGET)

#test_INCS = -I.

EXTRA_CLEAN = my_clean

include $(HOME)/proj/pfkutils/Makefile.inc

my_clean:
	rm -f protobuf_parser.cc protobuf_parser.hh protobuf_tokenizer.cc protobuf_tokenizer.h
