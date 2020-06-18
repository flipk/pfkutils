# -*- Mode:makefile-gmake; tab-width:8 -*-

# see below the licence for usage instructions.

# This is free and unencumbered software released into the public domain.
# 
# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.
# 
# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
# 
# For more information, please refer to <http://unlicense.org>

##############################################
#
#    first, set some variables before including Makefile.inc.
#
# OBJDIR= <subdir, such as "obj">
#    this specifies where all build products will go. it is assumed
#    to be the name of a subdirectory relative to the current directory.
#
#    you can set CC, CXX, AR, RANLIB, BISON, FLEX, PROTOC_PATH
#    CFLAGS, CXXFLAGS, LDFLAGS, INCS, DEFS.
#    these variables affect every build target of this makefile.
#    Makefile.inc supplies reasonable defaults for these but you
#    can override them if you need to.
#
#    you MUST tell Makefile.inc what you want to build. you can build
#    libraries, or programs. make a list of target names. put those
#    target names into a variable LIB_TARGETS or PROG_TARGETS (depending
#    on whether each target is a library or program). don't worry about
#    the actual filenames produced yet, these target names are just used
#    for specifying other makefile variable names specific to that target.
#
# LIB_TARGETS = example somelib someotherlib
# PROG_TARGETS = prog1 phils_prog testprog
#
#    each target name is used as the prefix for another set of
#    variables, e.g.  if you have a library target named "thingy",
#    Makefile.inc will looking for a variable called "thingy_CXXSRCS"
#    to see if that library includes C++ sources.  the list of
#    $(target)_xxx variables you can set is listed below. for this
#    example, we're specifying variables for the "somelib" and "prog1"
#    target examples we specified above.
#
# somelib_TARGET = $(OBJDIR)/libSOMELIB.a    # required
# somelib_CXXSRCS = one_file.cc  another_file.cc
# prog1_TARGET = $(OBJDIR)/program_one       # required
# prog1_CSRCS = a_c_file.c
#
#    the full list variables you can set (replace "$(target)" with
#    the name of the target you want to build):
#
# $(target)_TARGET = <library or executable filename to build, required>
# $(target)_CSRCS = <a list of "c" files to pas to $(CC)>
# $(target)_CXXSRCS = <a list of "cc" files to pass to $(CXX)>
# $(target)_YSRCS = <a list of "y" files to pass to $(BISON)>
# $(target)_LSRCS = <a list of "l" files to pass to $(FLEX)>
# $(target)_YYSRCS = <a list of "yy" files to pass to $(BISON) (C++)>
# $(target)_LLSRCS = <a list of "ll" files to pass to $(FLEX) (C++)>
# $(target)_PROTOSRCS = <a list of "proto" files to pass to $(PROTOC_PATH)>
# $(target)_CFLAGS = <compiler options to $(CC)>
# $(target)_CXXFLAGS = <compiler options to $(CXX)>
# $(target)_INCS = -I/path <a list of include dir directives>
# $(target)_DEFS = -DVAR=VALUE <a list of macro defines>
# $(target)_EXTRAOBJS = <a list of ".o" files built by other recipes>
# $(target)_POSTINSTALL = <a recipe name to run after installing>
# $(target)_PREMAKE = <a recipe name to run before making _TARGET>
# $(target)_POSTMAKE = <a recipe name to run after making _TARGET>
#
#    PROG_TARGETS can also have:
#
# $(targt)_DEPLIBS = $(somelib_TARGET) <a list of _TARGET dependencies>
# $(targt)_LIBS = -L/path -lpthread <a list of libraries to link against>
# $(targt)_LDFLAGS = -rdynamic <a list of options passed to linker>
#
#    now that you have set all these variables, you can include
#    this file.
#
# include /path/to/Makefile.inc
#
#    now, you can specify any additional make recipes you need.
#    NOTE you can't put any make recipes before the include, because
#    you want the "all" target in Makefile.inc to be the first one,
#    the default one run if you type "make" with no args.
#
#    happy hacking!
#
##############################################

# all artifacts produced by this makefile go under
# an objects tree rooted at this directory.
# default: specific to this os and processor.
ifeq ($(OBJDIR),)
# neat bug: if you use OBJDIR = $(shell xxx) then the shell cmd is
#           executed EVERY time $(OBJDIR) is evaluated; on complex
#           targets, this could be hundreds or thousands of invocations
#           of "uname".
#           if you use OBJDIR := $(shell xxx) then the shell cmd is
#           executed only once and the output reused for every $(OBJDIR) ref.
# ugh. cygwin puts slashes and parens in uname -r !!

UNAMER := $(shell uname -r | sed -e 's,/,_,g' -e 's,[(|)],_,g')
OBJDIR := obj.$(shell uname -m).$(shell uname -s).$(UNAMER)
endif

# additional subdirs of $(OBJDIR) to create, if your source tree
# has has a tree of relpath "dir/file.c" sources.
OBJDIRS_TOMAKE ?=

# common tools, reasonable defaults.
CC ?= gcc
CXX ?= g++
AR ?= ar
RANLIB ?= ranlib
BISON ?= bison
FLEX ?= flex

# on platforms that have protoc as a managed package,
# it is in one of three places.
POSSIBLE_PROTOC_PATHS = /bin/protoc /usr/bin/protoc /usr/local/bin/protoc
PROTOC_PATH ?= $(firstword $(wildcard $(POSSIBLE_PROTOC_PATHS)))

# cflags and cxxflags should not include defines or includes;
# this should be options to the compiler like optimization, debug,
# pedantics, warnflags, and the like (i.e. stuff that only cc1 or cc1plus
# would consume, but not stuff cpp would consume).
ifeq ($(DBG),1)
CFLAGS  += -O0 -g3
CXXFLAGS += -O0 -g3
OBJDIR := $(OBJDIR).dbg
else
CFLAGS ?= -O2
CXXFLAGS ?= -O2
endif

# if you want assembler listings from your C or CC files, add
#    -Wa,-ahls=$$(@:.o=.L)
# to CFLAGS, CXXFLAGS, $(target)_CFLAGS, and/or $(target)_CXXFLAGS
# (assuming your assembler is gnu gas, of course)

# flags applied to program linker. (i.e. -rdynamic). applied to
# all targets in PROG_TARGETS, applied before $(target)_LDFLAGS.
LDFLAGS ?= 

# cpp type arguments. INCS should be -I's, DEFS should be -D's.
# note these are global for all LIB and PROG targets! if different
# LIB and PROG need different values, use $(target)_INCS and $(target)_DEFS
# below.
INCS ?=
# add -D_FORTIFY_SOURCE to test warn_unused stuff is okay
DEFS ?=

# if you have files to preprocess before computing dependencies, here's
# your hook to add target recipes. of course this makefile doesn't do any
# work for you, you have to write the target recipes. this is invoked
# after objdirs but before deps.
PREPROC_TARGETS ?=

# if you have any libraries to make, list them out in this variable.
# for each value, there are a set of $(target)_XXX vars (described below)
# that should be set.  for instance, if you do
#    LIB_TARGETS += bloopy
# then you should also define
#    bloopy_TARGET = $(OBJDIR)/libbloopy.a
#    bloopy_CSRCS = some_file.c
#    [etc]
LIB_TARGETS ?=

# if you have any programs to make, list them out in this variable.
# this works just like LIB_TARGETS--see below for target-specifc vars
# you can set to configure a prog.
# note this makefile builds all $(LIB_TARGETS) before all $(PROG_TARGETS).
PROG_TARGETS ?=

# you can autogenerate doxygen documentation. list all doxygen targets
# you want to make with 'make doxygen' or installs with 'make doxygen-install'
# with this variable.
DOXYGEN_TARGETS ?=

# if you have more recipes you want to run after the "all" rule in this
# makefile as run (i.e. after all PREPROC, LIB, and PROG) then this is
# your opportunity to name more recipe rules to invoke.
POSTALL ?= 

# 'make clean' will blow away the $(OBJDIR) and dox directories, but
# if you have more you'd like to clean, make another recipe and add
# the recipe name(s) to $(EXTRA_CLEAN).
EXTRA_CLEAN ?=

##############################################
#
# all targets can define:
#   $(target)_TARGET = $(OBJDIR)/something   # required for all targets
#   $(target)_CSRCS = file.c    # only if you have C files
#   $(target)_CXXSRCS = file.cc  # only if you have C++ files
#   $(target)_YSRCS = file.y   # only if you have yacc (bison) inputs
#   $(target)_LSRCS = file.l   # only if you have lex (Flex) inputs
#   $(target)_YYSRCS = file.yy  # yacc (bison) inputs with C++ code
#   $(target)_LLSRCS = file.ll  # lex (flex) inputs with C++ code
#   $(target)_PROTOSRCS = file.proto # google protobuf definition files
#   $(target)_CFLAGS =               # to add cc1 type options
#   $(target)_CXXFLAGS =             # to add cc1plus type options
#   $(target)_INCS = -Isome/path
#   $(target)_DEFS = -DSOME_VAR=SOME_VALUE
#   $(target)_EXTRAOBJS = some_object.o # only if you want custom
#                                         objects built by your own recipes.
#   $(target)_POSTINSTALL = some_recipe # only if you have additional rules
#                                         you want triggered during a
#                                         'make install'
#   $(target)_PREMAKE = recipe  # rules run before making $(target)
#   $(target)_POSTMAKE = recipe # rules run after making $(target)

# lib targets can also have:
#   $(target)_INSTALL_HDRS = file.h # (only if library is to be installed)
#
# prog targets can also have:
#   $(target)_DEPLIBS= $($(somelibtarget)_TARGET) # to ref a LIB_TARGET
#   $(target)_LIBS=-lpthread        # only for -l arguments.
#   $(target)_LDFLAGS=-rdynamic     # passed to linker
#   $(target)_INSTALL=1   # only if you want this prog installed
#
# doxyen targets can define:
#   $(target)_DOXYFILE = Doxyfile
#
# NOTE if you define $(target)_PROTOSRCS with a list of proto files,
# there will be an autodefined variable produced for each proto source
# of the form  "$(target)_$(proto_filename)_HDR"  which contains the
# path to the autogenerated header file created from that proto file.
# example:
#         LIB_TARGETS += testProto
#         testProto_TARGET = $(OBJDIR)/libtestProto.a
#         testProto_PROTOSRCS = test1.proto test2.proto
#
# this will result in the following variables being defined:
#         testProto_test1.proto_HDR = $(OBJDIR)/testProto-test1.pb.h
#         testProto_test2.proto_HDR = $(OBJDIR)/testProto-test2.pb.h
#
# these macros are then suitable to put in a C file like this:
#         testProto_CXXSRCS = test1.cc
#         testProto_DEFS = -DTEST_PROTO_HDR=\"$(testProto_test1.proto_HDR)\"
# and then in the file test1.cc:
#         #include TEST_PROTO_HDR
#
##############################################

ifeq ($(VERBOSE),1)
Q=
else
Q=@
endif
export VERBOSE

all:
	$(Q)+make echoconfig objdirs preprocs protobufs
	$(Q)make deps
	$(Q)+make $(MAKEJOBS) __INCLUDE_DEPS=1 _all

include $(INCLUDE_MAKEFILES)

echoconfig:
	@echo ''
	@echo '**************** CURRENT CONFIGURATION ************************'
ifneq ($(CONFIG),)
	@echo CONFIG=$(CONFIG)
endif
ifneq ($(PFKARCH),)
	@echo PFKARCH=$(PFKARCH)
endif
	@echo OBJDIR=$(OBJDIR)
ifneq ($(PREPROC_TARGETS),)
	@echo PREPROC_TARGETS=$(PREPROC_TARGETS)
endif
ifneq ($(LIB_TARGETS),)
	@echo LIB_TARGETS=$(LIB_TARGETS)
endif
ifneq ($(PROG_TARGETS),)
	@echo PROG_TARGETS=$(PROG_TARGETS)
endif
ifneq ($(MAKEJOBS),)
	@echo MAKEJOBS=$(MAKEJOBS)
endif
	@echo '**************** CURRENT CONFIGURATION ************************'
	@echo ''

objdirs:
	$(Q)mkdir -p $(OBJDIR) $(foreach d,$(OBJDIRS_TOMAKE),$(OBJDIR)/$(d))

##############################################

# $(call FROM_X_TO_Y,cc,o,dirA/dirB/file.cc) == objdir/target-dirA_dirB_file.o

FROM_X_TO_Y = $(patsubst %.$(1), $(OBJDIR)/$(target)-%.$(2), $(subst /,_,$(3)))

FROM_C_TO_O      = $(call FROM_X_TO_Y,c,o,$(1))
FROM_C_TO_D      = $(call FROM_X_TO_Y,c,c.d,$(1))
FROM_CC_TO_O     = $(call FROM_X_TO_Y,cc,o,$(1))
FROM_CC_TO_D     = $(call FROM_X_TO_Y,cc,cc.d,$(1))
FROM_Y_TO_C      = $(call FROM_X_TO_Y,y,c,$(1))
FROM_Y_TO_H      = $(call FROM_X_TO_Y,y,h,$(1))
FROM_Y_TO_D      = $(call FROM_X_TO_Y,y,c.d,$(1))
FROM_Y_TO_O      = $(call FROM_X_TO_Y,y,o,$(1))
FROM_YY_TO_CC    = $(call FROM_X_TO_Y,yy,cc,$(1))
FROM_YY_TO_HH    = $(call FROM_X_TO_Y,yy,hh,$(1))
FROM_YY_TO_D     = $(call FROM_X_TO_Y,yy,cc.d,$(1))
FROM_YY_TO_O     = $(call FROM_X_TO_Y,yy,o,$(1))
FROM_L_TO_C      = $(call FROM_X_TO_Y,l,c,$(1))
FROM_L_TO_D      = $(call FROM_X_TO_Y,l,c.d,$(1))
FROM_L_TO_O      = $(call FROM_X_TO_Y,l,o,$(1))
FROM_LL_TO_CC    = $(call FROM_X_TO_Y,ll,cc,$(1))
FROM_LL_TO_D     = $(call FROM_X_TO_Y,ll,cc.d,$(1))
FROM_LL_TO_O     = $(call FROM_X_TO_Y,ll,o,$(1))
FROM_PROTO_TO_CC = $(call FROM_X_TO_Y,proto,pb.cc,$(1))
FROM_PROTO_TO_H  = $(call FROM_X_TO_Y,proto,pb.h,$(1))
FROM_PROTO_TO_D  = $(call FROM_X_TO_Y,proto,pb.cc.d,$(1))
FROM_PROTO_TO_O  = $(call FROM_X_TO_Y,proto,pb.o,$(1))

define TARGET_VARS

$(target)_COBJS        = $(call FROM_C_TO_O,$($(target)_CSRCS))
$(target)_CDEPS        = $(call FROM_C_TO_D,$($(target)_CSRCS))
$(target)_CXXOBJS      = $(call FROM_CC_TO_O,$($(target)_CXXSRCS))
$(target)_CXXDEPS      = $(call FROM_CC_TO_D,$($(target)_CXXSRCS))
$(target)_YGENSRCS     = $(call FROM_Y_TO_C,$($(target)_YSRCS))
$(target)_YGENDEPS     = $(call FROM_Y_TO_D,$($(target)_YSRCS))
$(target)_YGENOBJS     = $(call FROM_Y_TO_O,$($(target)_YSRCS))
$(target)_LGENSRCS     = $(call FROM_L_TO_C,$($(target)_LSRCS))
$(target)_LGENDEPS     = $(call FROM_L_TO_D,$($(target)_LSRCS))
$(target)_LGENOBJS     = $(call FROM_L_TO_O,$($(target)_LSRCS))
$(target)_YYGENSRCS    = $(call FROM_YY_TO_CC,$($(target)_YYSRCS))
$(target)_YYGENDEPS    = $(call FROM_YY_TO_D,$($(target)_YYSRCS))
$(target)_YYGENOBJS    = $(call FROM_YY_TO_O,$($(target)_YYSRCS))
$(target)_LLGENSRCS    = $(call FROM_LL_TO_CC,$($(target)_LLSRCS))
$(target)_LLGENDEPS    = $(call FROM_LL_TO_D,$($(target)_LLSRCS))
$(target)_LLGENOBJS    = $(call FROM_LL_TO_O,$($(target)_LLSRCS))
$(target)_PROTOGENSRCS = $(call FROM_PROTO_TO_CC,$($(target)_PROTOSRCS))
$(target)_PROTOGENHDRS = $(call FROM_PROTO_TO_H,$($(target)_PROTOSRCS))
$(target)_PROTOGENDEPS = $(call FROM_PROTO_TO_D,$($(target)_PROTOSRCS))
$(target)_PROTOGENOBJS = $(call FROM_PROTO_TO_O,$($(target)_PROTOSRCS))

$(foreach src,$($(target)_YSRCS),
$(target)_$(src)_HDR := $(notdir $(call FROM_Y_TO_H, $(src)))
)

$(foreach src,$($(target)_YYSRCS),
$(target)_$(src)_HDR := $(notdir $(call FROM_YY_TO_HH, $(src)))
)

$(foreach src,$($(target)_PROTOSRCS),
$(target)_$(src)_HDR := $(notdir $(call FROM_PROTO_TO_H, $(src))))

endef

$(eval $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),$(TARGET_VARS)))

##############################################

define TARGET_RULES
$($(target)_LLGENSRCS): $($(target)_YYGENSRCS)
$($(target)_LLGENSRCS): $($(target)_LLSRCS)
$($(target)_YYGENSRCS): $($(target)_YYSRCS)

$($(target)_LGENSRCS): $($(target)_YGENSRCS)
$($(target)_LGENSRCS): $($(target)_LSRCS)
$($(target)_YGENSRCS): $($(target)_YSRCS)

CGENSRCS += $($(target)_YGENSRCS) $($(target)_LGENSRCS)
CXXGENSRCS += $($(target)_YYGENSRCS) $($(target)_LLGENSRCS) \
		$($(target)_PROTOGENSRCS)

CDEPS += $($(target)_CDEPS)
CXXDEPS += $($(target)_CXXDEPS)

CGENDEPS += $($(target)_YGENDEPS) $($(target)_LGENDEPS)
CXXGENDEPS += $($(target)_YYGENDEPS) $($(target)_LLGENDEPS) \
		$($(target)_PROTOGENDEPS)

# for cscope
HDRS += $($(target)_HDRS)
CSRCS += $($(target)_CSRCS) $($(target)_YGENSRCS) $($(target)_LGENSRCS)
CXXSRCS += $($(target)_CXXSRCS) $($(target)_YYGENSRCS) $($(target)_LLGENSRCS)


# handle C files

$(foreach src,$($(target)_CSRCS),$(call FROM_C_TO_O, $(src)): $(src)
	@echo compiling $$<
	$(Q)$(CC) -c -I$(OBJDIR) $(CFLAGS) $($(target)_CFLAGS) \
		$($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) $$< -o $$@
)

$(foreach src,$($(target)_CSRCS),$(call FROM_C_TO_D, $(src)): $(src)
	@echo depending $$<
	$(Q)$(CC) -I$(OBJDIR) $($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) -DDEPENDING=1 \
		-M $$< -MT $(call FROM_C_TO_O,$(src)) -MF $$@.tmp
	$(Q)mv $$@.tmp $$@
)

# handle CC files

$(foreach src,$($(target)_CXXSRCS),$(call FROM_CC_TO_O, $(src)): $(src)
	@echo compiling $$<
	$(Q)$(CXX) -c -I$(OBJDIR) $(CXXFLAGS) $($(target)_CXXFLAGS) \
		$($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) \
		$$< -o $$@
)


$(foreach src,$($(target)_CXXSRCS),$(call FROM_CC_TO_D, $(src)): $(src)
	@echo depending $$<
	$(Q)$(CXX) -I$(OBJDIR) $($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) -DDEPENDING=1 \
		-M $$< -MT $(call FROM_CC_TO_O,$(src)) -MF $$@.tmp
	$(Q)mv $$@.tmp $$@
)

# handle Y files

$(foreach src,$($(target)_YSRCS),$(call FROM_Y_TO_O, $(src)): $(call FROM_Y_TO_C, $(src))
	@echo compiling $$<
	$(Q)$(CC) -c -I$(OBJDIR) $(CFLAGS) $($(target)_CFLAGS) \
		$($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) \
		$$< -o $$@
)

$(foreach src,$($(target)_YSRCS),$(call FROM_Y_TO_C, $(src)) $(call FROM_Y_TO_H, $(src)): $(src)
	@echo making $$@
	$(Q)$(BISON) -d $$< -o $$@
)

$(foreach src,$($(target)_YSRCS),$(call FROM_Y_TO_D, $(src)): $(call FROM_Y_TO_C, $(src)) $(call FROM_Y_TO_H, $(src))
	@echo depending $$<
	$(Q)$(CC) -I$(OBJDIR) $($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) -DDEPENDING=1 \
		-M $$< -MT $(call FROM_Y_TO_O,$(src)) -MF $$@.tmp
	$(Q)mv $$@.tmp $$@
)

# handle YY files

$(foreach src,$($(target)_YYSRCS),$(call FROM_YY_TO_CC, $(src)) $(call FROM_YY_TO_HH, $(src)): $(src)
	@echo making $$@
	$(Q)$(BISON) -d $$< -o $$@
)

$(foreach src,$($(target)_YYSRCS),$(call FROM_YY_TO_D, $(src)): $(call FROM_YY_TO_CC, $(src))
	@echo depending $$<
	$(Q)$(CXX) -I$(OBJDIR) $($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) -DDEPENDING=1 \
		-M $$< -MT $(call FROM_YY_TO_O,$(src)) -MF $$@.tmp
	$(Q)mv $$@.tmp $$@
)

$(foreach src,$($(target)_YYSRCS),$(call FROM_YY_TO_O, $(src)): $(call FROM_YY_TO_CC, $(src)) $(call FROM_YY_TO_HH, $(src))
	@echo compiling $$<
	$(Q)$(CXX) -c -I$(OBJDIR) $(CXXFLAGS) $($(target)_CXXFLAGS) \
		$($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) $$< -o $$@
)

# handle L files

$(foreach src,$($(target)_LSRCS),$(call FROM_L_TO_C, $(src)): $(src)
	@echo making $$@
	$(Q)$(FLEX) --stdout $$<  > $$@
)

$(foreach src,$($(target)_LSRCS),$(call FROM_L_TO_D, $(src)): $(call FROM_L_TO_C, $(src))
	@echo depending $$<
	$(Q)$(CXX) -I$(OBJDIR) $($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) -DDEPENDING=1 \
		-M $$< -MT $(call FROM_L_TO_O,$(src)) -MF $$@.tmp
	$(Q)mv $$@.tmp $$@
)

$(foreach src,$($(target)_LSRCS),$(call FROM_L_TO_O, $(src)): $(call FROM_L_TO_C, $(src))
	@echo compiling $$<
	$(Q)$(CC) -c -I$(OBJDIR) $(CFLAGS) $($(target)_CFLAGS) \
		$($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) \
		$$< -o $$@
)

# handle LL files

$(foreach src,$($(target)_LLSRCS),$(call FROM_LL_TO_CC, $(src)): $(src)
	@echo making $$@
	$(Q)$(FLEX) --stdout $$<  > $$@
)

$(foreach src,$($(target)_LLSRCS),$(call FROM_LL_TO_O, $(src)): $(call FROM_LL_TO_CC, $(src))
	@echo compiling $$<
	$(Q)$(CXX) -c -I$(OBJDIR) $(CXXFLAGS) $($(target)_CXXFLAGS) \
		$($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) $$< -o $$@
)

$(foreach src,$($(target)_LLSRCS),$(call FROM_LL_TO_D, $(src)): $(call FROM_LL_TO_CC, $(src))
	@echo depending $$<
	$(Q)$(CXX) -I$(OBJDIR) $($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) -DDEPENDING=1 \
		-M $$< -MT $(call FROM_LL_TO_O,$(src)) -MF $$@.tmp
	$(Q)mv $$@.tmp $$@
)

# handle PROTO files

# note that protoc turns file.proto into file.pb.h and file.pb.cc,
# but we really want obj/$(target)-directory_directory_file.pb.cc and .h
# but also note protoc spits out the .pb.cc file with a #include looking for file.pb.h,
# so we use a sed-expression to replace that with $(target)-dir_dir_file_pb.h

$(foreach src,$($(target)_PROTOSRCS),

$(call FROM_PROTO_TO_CC, $(src)): $(src)
	@echo making $$@
	$(Q)cd $$(dir $$<) && $(PROTOC_PATH) \
		--cpp_out=$$(dir $(PWD)/$$@) $$(notdir $$<)
	$(Q)mv $(OBJDIR)/$$(notdir $(src:.proto=.pb.h)) $$(@:.pb.cc=.pb.h)
	$(Q)sed -e s,$$(notdir $(src:.proto=.pb.h)),$$(notdir $$(@:.pb.cc=.pb.h)), \
		<  $(OBJDIR)/$$(notdir $(src:.proto=.pb.cc)) \
		> $$(@).TEMP
	$(Q)mv $$(@).TEMP $$(@)
)

$(foreach src,$($(target)_PROTOSRCS),$(call FROM_PROTO_TO_O, $(src)): $(call FROM_PROTO_TO_CC, $(src))
	@echo compiling $$<
	$(Q)$(CXX) -c -I$(OBJDIR) $(CXXFLAGS) $($(target)_CXXFLAGS) \
		$($(target)_INCS) $(PROTOBUF_INC) \
		$(INCS) $(DEFS) $($(target)_DEFS) $$< -o $$@
)

$(foreach src,$($(target)_PROTOSRCS),$(call FROM_PROTO_TO_D, $(src)): $(call FROM_PROTO_TO_CC, $(src))
	@echo depending $$<
	$(Q)$(CXX) -I$(OBJDIR) $($(target)_INCS) $(PROTOBUF_INC) \
		$(INCS) $(DEFS) $($(target)_DEFS) -DDEPENDING=1 \
		-M $$< -MT $(call FROM_PROTO_TO_O,$(src)) -MF $$@.tmp
	$(Q)mv $$@.tmp $$@
)

# here for reference
#$($(target)_PROTOGENSRCS): $(OBJDIR)/%.pb.cc : %.proto
#	@echo making $$@
#	@cd $$(dir $$<) && $(PROTOC_PATH) \
#		--cpp_out=$$(dir $(PWD)/$$@) $$(notdir $$<)

endef

define TARGET_OBJ_LIST

$(target)_OBJS = \
	$($(target)_COBJS)        $($(target)_CXXOBJS) \
	$($(target)_YGENOBJS)     $($(target)_LGENOBJS) \
	$($(target)_YYGENOBJS)    $($(target)_LLGENOBJS) \
	$($(target)_PROTOGENOBJS) $($(target)_EXTRAOBJS)

endef

define LIB_TARGET_RULES

$($(target)_TARGET): $($(target)_OBJS)
	@echo linking $($(target)_TARGET)
	$(Q)rm -f $($(target)_TARGET)
	$(Q)$(AR) cq $($(target)_TARGET) $($(target)_OBJS)
	$(Q)$(RANLIB) $($(target)_TARGET)

$(target)_install: $($(target)_TARGET)
	$(Q)set -e ; if [ "x$($(target)_INSTALL_HDRS)" != "x" ] ; then \
		echo installing $($(target)_TARGET) and headers ; \
		cp $($(target)_TARGET) $(INSTALL_LIB_DIR) ; \
		tar cf - $($(target)_INSTALL_HDRS) | \
			tar -C $(INSTALL_INC_DIR) -xf - ; \
	fi

endef

define PROG_TARGET_RULES

$($(target)_TARGET): $($(target)_OBJS) $($(target)_DEPLIBS)
	@echo linking $($(target)_TARGET)
	$(Q)$(CXX) -o $($(target)_TARGET) $($(target)_OBJS) \
		$($(target)_DEPLIBS) \
		$(LDFLAGS) $($(target)_LDFLAGS) \
		$($(target)_LIBS)

$(target)_install: $($(target)_TARGET)
	$(Q)set -e ; if [ "x$($(target)_INSTALL)" = "x1" ] ; then \
		echo installing $($(target)_TARGET) ; \
		FNAME="$(notdir $($(target)_TARGET))" ; \
		if [ -f $(INSTALL_BIN_DIR)/$$$$FNAME ] ; then \
			mv -f $(INSTALL_BIN_DIR)/$$$$FNAME \
				$(INSTALL_BIN_DIR)/$$$$FNAME.old ;\
		fi ; \
		cp $($(target)_TARGET) $(INSTALL_BIN_DIR)/$$$$FNAME ; \
	fi

endef

PROTOGENSRCS := $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS), \
	$($(target)_PROTOGENSRCS))

$(eval $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),$(TARGET_OBJ_LIST)))
$(eval $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),$(TARGET_RULES)))
$(eval $(foreach target,$(LIB_TARGETS),$(LIB_TARGET_RULES)))
$(eval $(foreach target,$(PROG_TARGETS),$(PROG_TARGET_RULES)))

##############################################

define DOXYGEN_TARGET_RULES
$(target)_doxygen:
	$(foreach dox,$($(target)_DOXYFILE),doxygen $(dox) &&) true

$(target)_doxygen-install:
	@echo making $(target)_doxygen-install is not yet supported

endef

$(eval $(foreach target,$(DOXYGEN_TARGETS),$(DOXYGEN_TARGET_RULES)))

##############################################

preprocs: $(CONFIG_H) $(PREPROC_TARGETS)

##############################################

protobufs: $(PROTOGENSRCS)

##############################################

deps: $(CGENDEPS) $(CXXGENDEPS) $(CDEPS) $(CXXDEPS)

ifeq ($(__INCLUDE_DEPS),1)
include $(CDEPS) $(CXXDEPS) $(CGENDEPS) $(CXXGENDEPS)
endif

##############################################

_all: $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),$($(target)_PREMAKE) $($(target)_TARGET) $($(target)_POSTMAKE)) $(POSTALL)

cscope.files: Makefile $(INCLUDE_MAKEFILES)
	@echo making cscope.files
	@rm -f cscope.files ; touch cscope.files
	@$(foreach f,$(HDRS) $(CSRCS) $(CXXSRCS),echo $(f) >> cscope.files ;)

cscope: cscope.files $(HDRS) $(CSRCS) $(CXXSRCS)
	@echo making cscope.out
	@cscope -bk

install: installdirs $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),\
		$(target)_install) $(foreach target,$(LIB_TARGETS) \
		$(PROG_TARGETS),$($(target)_POSTINSTALL)) \
		$(POSTINSTALL)

doxygen: $(foreach target,$(DOXYGEN_TARGETS),$(target)_doxygen)

doxygen-install: $(foreach target,$(DOXYGEN_TARGETS),$(target)_doxygen-install)

clean: $(EXTRA_CLEAN)
	rm -rf $(OBJDIR) dox/*
