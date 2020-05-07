
OBJDIR= obj

LIB_TARGETS = protoparser

# if you don't have a Makefile.inc that can do this for you,
# you can do these by hand:
# bison -d json_parser.yy -o json_parser.cc
# flex json_tokenizer.ll

protoparser_TARGET = $(OBJDIR)/libprotoparser.a
protoparser_LLSRCS = protobuf_tokenizer.ll
protoparser_YYSRCS = protobuf_parser.yy
protoparser_INCS = -I.
protoparser_DEFS = -DPARSER_YY_HDR=\"protoparser-protobuf_parser.hh\"

PROG_TARGETS = test

test_TARGET = $(OBJDIR)/test
test_CXXSRCS = main.cc
test_DEPLIBS = $(protoparser_TARGET)

include $(HOME)/proj/pfkutils/Makefile.inc
