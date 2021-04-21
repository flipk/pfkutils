
DEBUG_H_MACRO_GENERATION=y

OBJDIR = obj
PROG_TARGETS = test

test_TARGET = $(OBJDIR)/test
test_CXXSRCS = main.cc
test_LLSRCS = tokenizer.ll
test_YYSRCS = parser.yy
test_DEFS = -DPARSER_YY_HDR=\"$(test_parser.yy_HDR)\" 

include $(HOME)/proj/pfkutils/Makefile.inc

