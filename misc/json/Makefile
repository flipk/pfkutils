
OBJDIR= obj

LIB_TARGETS = simplejson

simplejson_TARGET = $(OBJDIR)/libsimplejson.a
simplejson_CXXSRCS = simple_json.cc json_tokenizer.cc json_parser.cc

# pfkutils/Makefile.inc can actually do these for you, but i'm not doing
# that here to show you how to use flexbison.sh
#simplejson_LLSRCS = tokenizer.ll
#simplejson_YYSRCS = parser.yy
#simplejson_DEFS = -DPARSER_YY_HDR=\"simplejson-parser.hh\"

PROG_TARGETS = test

test_TARGET = $(OBJDIR)/test
test_CXXSRCS = main.cc
test_DEPLIBS = $(simplejson_TARGET)

EXTRA_CLEAN = my_clean

include $(HOME)/proj/pfkutils/Makefile.inc

my_clean:
	rm -f json_parser.cc json_parser.hh json_tokenizer.cc json_tokenizer.h
