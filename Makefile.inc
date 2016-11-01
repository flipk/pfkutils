# -*- Mode:makefile-gmake; tab-width:8 -*-

# TODO strip relpaths and handle files outside this tree;
# TODO prepend target-name to product artifacts?

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
OBJDIR := obj.$(shell uname -m).$(shell uname -s).$(shell uname -r)
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
PROTOC_PATH ?= protoc

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

# if you have more recipes you want to run after the "all" rule in this
# makefile as run (i.e. after all PREPROC, LIB, and PROG) then this is
# your opportunity to name more recipe rules to invoke.
POSTALL ?= 

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
#
# lib targets can also have:
#   $(target)_INSTALL_HDRS = file.h # (only if library is to be installed)
#
# prog targets can also have:
#   $(target)_DEPLIBS= $($(somelibtarget)_TARGET) # to ref a LIB_TARGET
#   $(target)_LIBS=-lpthread        # only for -l arguments.
#   $(target)_LDFLAGS=-rdynamic     # passed to linker
#   $(target)_INSTALL=1   # only if you want this prog installed
#
##############################################

ifeq ($(VERBOSE),1)
Q=
else
Q=@
endif
export VERBOSE

all:
	$(Q)+make objdirs
	$(Q)+make preprocs
	$(Q)+make deps
	$(Q)+make __INCLUDE_DEPS=1 _all

include $(INCLUDE_MAKEFILES)

echoconfig:
	@echo ''
	@echo '**************** CURRENT CONFIGURATION ************************'
	@echo CONFIG=$(CONFIG)
	@echo PFKARCH=$(PFKARCH)
	@echo OBJDIR=$(OBJDIR)
	@echo PREPROC_TARGETS=$(PREPROC_TARGETS)
	@echo LIB_TARGETS=$(LIB_TARGETS)
	@echo PROG_TARGETS=$(PROG_TARGETS)
	@echo '**************** CURRENT CONFIGURATION ************************'
	@echo ''

objdirs:
	$(Q)mkdir -p $(OBJDIR) $(foreach d,$(OBJDIRS_TOMAKE),$(OBJDIR)/$(d))

##############################################

define TARGET_VARS
$(target)_COBJS    = $(patsubst %.c,  $(OBJDIR)/%.o,    $($(target)_CSRCS))
$(target)_CDEPS    = $(patsubst %.c,  $(OBJDIR)/%.c.d,  $($(target)_CSRCS))

$(target)_CXXOBJS  = $(patsubst %.cc, $(OBJDIR)/%.o,    $($(target)_CXXSRCS))
$(target)_CXXDEPS  = $(patsubst %.cc, $(OBJDIR)/%.cc.d, $($(target)_CXXSRCS))

$(target)_YGENSRCS = $(patsubst %.y,  $(OBJDIR)/%.c,    $($(target)_YSRCS))
$(target)_YGENDEPS = $(patsubst %.y,  $(OBJDIR)/%.c.d,  $($(target)_YSRCS))
$(target)_YGENOBJS = $(patsubst %.y,  $(OBJDIR)/%.o,    $($(target)_YSRCS))

$(target)_LGENSRCS = $(patsubst %.l,  $(OBJDIR)/%.c,    $($(target)_LSRCS))
$(target)_LGENDEPS = $(patsubst %.l,  $(OBJDIR)/%.c.d,  $($(target)_LSRCS))
$(target)_LGENOBJS = $(patsubst %.l,  $(OBJDIR)/%.o,    $($(target)_LSRCS))

$(target)_YYGENSRCS= $(patsubst %.yy, $(OBJDIR)/%.cc,   $($(target)_YYSRCS))
$(target)_YYGENDEPS= $(patsubst %.yy, $(OBJDIR)/%.cc.d, $($(target)_YYSRCS))
$(target)_YYGENOBJS= $(patsubst %.yy, $(OBJDIR)/%.o,    $($(target)_YYSRCS))

$(target)_LLGENSRCS= $(patsubst %.ll, $(OBJDIR)/%.cc,   $($(target)_LLSRCS))
$(target)_LLGENDEPS= $(patsubst %.ll, $(OBJDIR)/%.cc.d, $($(target)_LLSRCS))
$(target)_LLGENOBJS= $(patsubst %.ll, $(OBJDIR)/%.o,    $($(target)_LLSRCS))

$(target)_PROTOGENSRCS = $(patsubst %.proto, $(OBJDIR)/%.pb.cc, \
				$($(target)_PROTOSRCS))
$(target)_PROTOGENHDRS = $(patsubst %.proto, $(OBJDIR)/%.pb.h, \
				$($(target)_PROTOSRCS))
$(target)_PROTOGENDEPS = $(patsubst %.proto, $(OBJDIR)/%.pb.cc.d, \
				$($(target)_PROTOSRCS))
$(target)_PROTOGENOBJS = $(patsubst %.proto, $(OBJDIR)/%.pb.o, \
				$($(target)_PROTOSRCS))

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

$($(target)_COBJS) : $(OBJDIR)/%.o: %.c
	@echo compiling $$<
	$(Q)$(CC) -c -I$(OBJDIR) $($(target)_CFLAGS) \
		$($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) $(CFLAGS) $$< -o $$@

$($(target)_CXXOBJS) : $(OBJDIR)/%.o: %.cc
	@echo compiling $$<
	$(Q)$(CXX) -c -I$(OBJDIR) $($(target)_CXXFLAGS) \
		$($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) \
		$(CXXFLAGS) $$< -o $$@

$($(target)_YGENOBJS) $($(target)_LGENOBJS) : %.o: %.c
	@echo compiling $$<
	$(Q)$(CC) -c -I$(OBJDIR) $($(target)_CFLAGS) \
		$($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) \
		$(CFLAGS) $$< -o $$@

$($(target)_PROTOGENOBJS) : %.o: %.cc
	@echo compiling $$<
	$(Q)$(CXX) -c -I$(OBJDIR) $($(target)_CXXFLAGS) \
		$($(target)_INCS) -I$(PROTOBUF_INC) \
		$(INCS) $(DEFS) $($(target)_DEFS) $(CXXFLAGS) $$< -o $$@

$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) : %.o: %.cc
	@echo compiling $$<
	$(Q)$(CXX) -c -I$(OBJDIR) $($(target)_CXXFLAGS) \
		$($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) $(CXXFLAGS) $$< -o $$@

$($(target)_YGENSRCS) : $(OBJDIR)/%.c : %.y
	@echo making $$@
	$(Q)$(BISON) -d $$< -o $$@

$($(target)_YYGENSRCS) : $(OBJDIR)/%.cc : %.yy
	@echo making $$@
	$(Q)$(BISON) -d $$< -o $$@

# flex ignores the -o arg !
$($(target)_LGENSRCS) : $(OBJDIR)/%.c : %.l
	@echo making $$@
	$(Q)LLFILE=$$(PWD)/$$< && \
		cd `dirname $$@` && \
		$(FLEX) $$$$LLFILE && \
		mv lex.yy.c `basename $$@`

$($(target)_LLGENSRCS) : $(OBJDIR)/%.cc : %.ll
	@echo making $$@
	$(Q)LLFILE=$$(PWD)/$$< && \
		cd `dirname $$@` && \
		$(FLEX) $$$$LLFILE && \
		mv lex.yy.c `basename $$@`

$($(target)_PROTOGENSRCS): $(OBJDIR)/%.pb.cc : %.proto
	@echo making $$@
	@cd $$(dir $$<) && $(PROTOC_PATH) \
		--cpp_out=$$(dir $(PWD)/$$@) $$(notdir $$<)

$($(target)_CDEPS) : $(OBJDIR)/%.c.d: %.c
	@echo depending $$<
	$(Q)$(CC) -I$(OBJDIR) $($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) \
		-M $$< -MT $$(<:%.c=$(OBJDIR)/%.o) -MF $$@

$($(target)_CXXDEPS) : $(OBJDIR)/%.cc.d: %.cc
	@echo depending $$<
	$(Q)$(CXX) -I$(OBJDIR) $($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) \
		-M $$< -MT $$(<:%.cc=$(OBJDIR)/%.o) -MF $$@

$($(target)_YGENDEPS) $($(target)_LGENDEPS) : %.c.d: %.c
	@echo depending $$<
	$(Q)$(CC) -I$(OBJDIR) $($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) \
		-M $$< -MT $$(<:%.c=%.o) -MF $$@

$($(target)_YYGENDEPS) $($(target)_LLGENDEPS) : %.cc.d: %.cc
	@echo depending $$<
	$(Q)$(CXX) -I$(OBJDIR) $($(target)_INCS) $(INCS) \
		$(DEFS) $($(target)_DEFS) \
		-M $$< -MT $$(<:%.cc=%.o) -MF $$@

$($(target)_PROTOGENDEPS) : %.cc.d: %.cc
	@echo depending $$<
	$(Q)$(CXX) -I$(OBJDIR) $($(target)_INCS) $(PROTOBUF_INC) \
		$(INCS) $(DEFS) $($(target)_DEFS) \
		-M $$< -MT $$(<:%.cc=%.o) -MF $$@

endef

define LIB_TARGET_RULES

$($(target)_TARGET): $($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YGENOBJS) $($(target)_LGENOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_PROTOGENOBJS) \
		$($(target)_EXTRAOBJS)
	@echo linking $($(target)_TARGET)
	$(Q)rm -f $($(target)_TARGET)
	$(Q)$(AR) cq $($(target)_TARGET) \
		$($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YGENOBJS) $($(target)_LGENOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_PROTOGENOBJS) \
		$($(target)_EXTRAOBJS)
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

$($(target)_TARGET): $($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YGENOBJS) $($(target)_LGENOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_PROTOGENOBJS) \
		$($(target)_DEPLIBS) $($(target)_EXTRAOBJS)
	@echo linking $($(target)_TARGET)
	$(Q)$(CXX) -o $($(target)_TARGET) \
		$($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YGENOBJS) $($(target)_LGENOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_PROTOGENOBJS) $($(target)_DEPLIBS) \
		$($(target)_EXTRAOBJS) $($(target)_LIBS) \
		$(LDFLAGS) $($(target)_LDFLAGS)

$(target)_install: $($(target)_TARGET)
	$(Q)set -e ; if [ "x$($(target)_INSTALL)" == "x1" ] ; then \
		echo installing $($(target)_TARGET) ; \
		FNAME="$(notdir $($(target)_TARGET))" ; \
		if [ -f $(INSTALL_BIN_DIR)/$$$$FNAME ] ; then \
			mv -f $(INSTALL_BIN_DIR)/$$$$FNAME \
				$(INSTALL_BIN_DIR)/$$$$FNAME.old ;\
		fi ; \
		cp $($(target)_TARGET) $(INSTALL_BIN_DIR)/$$$$FNAME ; \
	fi

endef

$(eval $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),$(TARGET_RULES)))
$(eval $(foreach target,$(LIB_TARGETS),$(LIB_TARGET_RULES)))
$(eval $(foreach target,$(PROG_TARGETS),$(PROG_TARGET_RULES)))

##############################################

preprocs: $(CONFIG_H) $(PREPROC_TARGETS)

##############################################

deps: $(CGENDEPS) $(CXXGENDEPS) $(CDEPS) $(CXXDEPS)

ifeq ($(__INCLUDE_DEPS),1)
include $(CDEPS) $(CXXDEPS) $(CGENDEPS) $(CXXGENDEPS)
endif

##############################################

_all: $(foreach target,$(LIB_TARGETS) \
	$(PROG_TARGETS),$($(target)_TARGET)) $(POSTALL)

cscope.files: Makefile config/always config/$(CONFIG)
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

clean:
	rm -rf $(OBJDIR)

