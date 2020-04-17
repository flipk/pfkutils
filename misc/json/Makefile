
OBJDIR= obj

PROG_TARGETS = test

test_TARGET = $(OBJDIR)/test
test_CXXSRCS = main.cc simple_json.cc json_tokenizer.cc json_parser.cc

# pfkutils/Makefile.inc can actually do these for you, but i'm not doing
# that here to show you how to use flexbison.sh
#test_LLSRCS = tokenizer.ll
#test_YYSRCS = parser.yy
#test_DEFS = -DPARSER_YY_HDR=\"test-parser.hh\"

test_INCS = -I.

EXTRA_CLEAN = my_clean

include $(HOME)/proj/pfkutils/Makefile.inc

my_clean:
	rm -f json_parser.cc json_parser.hh json_tokenizer.cc json_tokenizer.h
