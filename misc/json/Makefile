
DEBUG_H_MACRO_GENERATION=y

OBJDIR= obj

LIB_TARGETS = simplejson

# if you don't have a Makefile.inc that can do this for you,
# you can do these by hand:
# bison -d json_parser.yy -o json_parser.cc
# flex json_tokenizer.ll

simplejson_TARGET = $(OBJDIR)/libsimplejson.a
simplejson_CXXSRCS = simple_json.cc
simplejson_LLSRCS = json_tokenizer.ll
simplejson_YYSRCS = json_parser.yy
simplejson_INCS = -I.
simplejson_DEFS = -DPARSER_YY_HDR=\"simplejson-json_parser.hh\"

PROG_TARGETS = test nbtopy

test_TARGET = $(OBJDIR)/test
test_CXXSRCS = main.cc
test_DEPLIBS = $(simplejson_TARGET)

nbtopy_TARGET = $(OBJDIR)/nbtopy
nbtopy_CXXSRCS = nbtopy.cc
nbtopy_DEPLIBS = $(simplejson_TARGET)

EXTRA_CLEAN = my_clean

include $(HOME)/proj/pfkutils/Makefile.inc

my_clean:
	rm -f json_parser.cc json_parser.hh json_tokenizer.cc json_tokenizer.h
