
DEBUG_H_MACRO_GENERATION=y

OBJDIR= obj

PROG_TARGETS = test

test_TARGET = $(OBJDIR)/test
test_CXXSRCS = test.cc
test_LLSRCS = tokenizer.ll
test_INCS = -I.
test_DEFS = -DTOKENIZER_LL_HDR=\"$(test_tokenizer.ll_HDR)\"

include $(HOME)/proj/pfkutils/Makefile.inc

