# Local Variables:
# mode: makefile-gmake
# tab-width: 8
# End:

ifeq ($(CONFIG),)

##############################################

KNOWN_CONFIGS= blade adler bohr droid sunlogin

all:
	@echo please specify CONFIG= from config/ subdir
	@echo or do 'make known_config' where known_config is
	@echo one of: $(KNOWN_CONFIGS)

define PER_CONFIG_RULES
$(config):
	@+make CONFIG=$(config)

$(config)-cscope:
	@make CONFIG=$(config) cscope

endef

$(eval $(foreach config,$(KNOWN_CONFIGS),$(PER_CONFIG_RULES)))

clean:
	rm -rf obj.*

##############################################

else # $CONFIG

##############################################

TOP= $(PWD)
OBJDIR= obj.$(PFKARCH).$(CONFIG)

CC=gcc
CXX=g++
CFLAGS= -O3
CXXFLAGS= -O3
AR=ar
RANLIB=ranlib

PREPROC_TARGETS=
LIB_TARGETS=
PROG_TARGETS= 

ifeq ($(VERBOSE),1)
Q=
else
Q=@
endif

##############################################

all:
	@+make objdirs
	@+make preprocs
	@+make deps
	@+make PFKUTILS_INCLUDE_DEPS=1 _all
	@+make -C contrib \
		PROGS="$(CONTRIB_PROGS)" OBJDIR=$(PWD)/$(OBJDIR)/contrib

include config/$(CONFIG)
include config/always

echoconfig:
	@echo ''
	@echo '**************** CURRENT CONFIGURATION **************************'
	@echo CONFIG=$(CONFIG)
	@echo TOP=$(TOP)
	@echo PFKARCH=$(PFKARCH)
	@echo OBJDIR=$(OBJDIR)
	@echo PREPROC_TARGETS=$(PREPROC_TARGETS)
	@echo LIB_TARGETS=$(LIB_TARGETS)
	@echo PROG_TARGETS=$(PROG_TARGETS)
	@echo '**************** CURRENT CONFIGURATION **************************'
	@echo ''

##############################################

# TODO: add .y and .l support some day

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
CXXGENSRCS += $($(target)_YYGENSRCS) $($(target)_LLGENSRCS)

CDEPS += $($(target)_CDEPS)
CXXDEPS += $($(target)_CXXDEPS)

CGENDEPS += $($(target)_YGENDEPS) $($(target)_LGENDEPS)
CXXGENDEPS += $($(target)_YYGENDEPS) $($(target)_LLGENDEPS)

# for cscope
HDRS += $($(target)_HDRS)
CSRCS += $($(target)_CSRCS) $($(target)_YGENSRCS) $($(target)_LGENSRCS)
CXXSRCS += $($(target)_CXXSRCS) $($(target)_YYGENSRCS) $($(target)_LLGENSRCS)

$($(target)_COBJS) : $(OBJDIR)/%.o: %.c
	@echo compiling $$<
	$(Q)gcc -c -I$(OBJDIR) $($(target)_INCS) $($(target)_DEFS) \
		$(CFLAGS) $$< -o $$@

$($(target)_CXXOBJS) : $(OBJDIR)/%.o: %.cc
	@echo compiling $$<
	$(Q)g++ -c -I$(OBJDIR) $($(target)_INCS) $($(target)_DEFS) \
		$(CXXFLAGS) $$< -o $$@

$($(target)_YGENOBJS) $($(target)_LGENOBJS) : %.o: %.c
	@echo compiling $$<
	$(Q)gcc -c -I$(OBJDIR) $($(target)_INCS) $($(target)_DEFS) \
		$(CFLAGS) $$< -o $$@

$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) : %.o: %.cc
	@echo compiling $$<
	$(Q)g++ -c -I$(OBJDIR) $($(target)_INCS) $($(target)_DEFS) \
		$(CXXFLAGS) $$< -o $$@

$($(target)_YGENSRCS) : $(OBJDIR)/%.c : %.y
	@echo making $$@
	$(Q)bison -d $$< -o $$@

$($(target)_YYGENSRCS) : $(OBJDIR)/%.cc : %.yy
	@echo making $$@
	$(Q)bison -d $$< -o $$@

# flex ignores the -o arg !
$($(target)_LGENSRCS) : $(OBJDIR)/%.c : %.l
	@echo making $$@
	$(Q)LLFILE=$$(PWD)/$$< && \
		cd `dirname $$@` && \
		flex $$$$LLFILE && \
		mv lex.yy.c `basename $$@`

$($(target)_LLGENSRCS) : $(OBJDIR)/%.cc : %.ll
	@echo making $$@
	$(Q)LLFILE=$$(PWD)/$$< && \
		cd `dirname $$@` && \
		flex $$$$LLFILE && \
		mv lex.yy.c `basename $$@`

$($(target)_CDEPS) : $(OBJDIR)/%.c.d: %.c
	@echo depending $$<
	$(Q)$(CC) -I$(OBJDIR) $($(target)_INCS) $($(target)_DEFS) \
		-M $$< -MT $$(<:%.c=$(OBJDIR)/%.o) -MF $$@

$($(target)_CXXDEPS) : $(OBJDIR)/%.cc.d: %.cc
	@echo depending $$<
	$(Q)$(CC) -I$(OBJDIR) $($(target)_INCS) $($(target)_DEFS) \
		-M $$< -MT $$(<:%.cc=$(OBJDIR)/%.o) -MF $$@

$($(target)_YGENDEPS) $($(target)_LGENDEPS) : %.c.d: %.c
	@echo depending $$<
	$(Q)$(CC) -I$(OBJDIR) $($(target)_INCS) $($(target)_DEFS) \
		-M $$< -MT $$(<:%.c=%.o) -MF $$@

$($(target)_YYGENDEPS) $($(target)_LLGENDEPS) : %.cc.d: %.cc
	@echo depending $$<
	$(Q)$(CC) -I$(OBJDIR) $($(target)_INCS) $($(target)_DEFS) \
		-M $$< -MT $$(<:%.cc=%.o) -MF $$@

endef

define LIB_TARGET_RULES

$($(target)_TARGET): $($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YGENOBJS) $($(target)_LGENOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_EXTRAOBJS)
	@echo linking $($(target)_TARGET)
	$(Q)rm -f $($(target)_TARGET)
	$(Q)$(AR) cq $($(target)_TARGET) \
		$($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YGENOBJS) $($(target)_LGENOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_EXTRAOBJS)
	$(Q)$(RANLIB) $($(target)_TARGET)

endef

define PROG_TARGET_RULES

$($(target)_TARGET): $($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YGENOBJS) $($(target)_LGENOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_DEPLIBS) $($(target)_EXTRAOBJS)
	@echo linking $($(target)_TARGET)
	$(Q)g++ -o $($(target)_TARGET) \
		$($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YGENOBJS) $($(target)_LGENOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_LIBS) $($(target)_DEPLIBS) \
		$($(target)_EXTRAOBJS)

endef

$(eval $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),$(TARGET_RULES)))
$(eval $(foreach target,$(LIB_TARGETS),$(LIB_TARGET_RULES)))
$(eval $(foreach target,$(PROG_TARGETS),$(PROG_TARGET_RULES)))

##############################################

preprocs: $(CONFIG_H) $(PREPROC_TARGETS)

##############################################

deps: $(CDEPS) $(CXXDEPS) $(CGENDEPS) $(CXXGENDEPS)

ifeq ($(PFKUTILS_INCLUDE_DEPS),1)
include $(CDEPS) $(CXXDEPS) $(CGENDEPS) $(CXXGENDEPS)
endif

##############################################

_all: $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),$($(target)_TARGET))

cscope.files: Makefile config/always config/$(CONFIG)
	@echo making cscope.files
	@rm -f cscope.files ; touch cscope.files
	@$(foreach f,$(HDRS) $(CSRCS) $(CXXSRCS),echo $(f) >> cscope.files ;)

cscope: cscope.files $(HDRS) $(CSRCS) $(CXXSRCS)
	@echo making cscope.out
	@cscope -bk

install:

clean:
	rm -rf $(OBJDIR)

##############################################

endif # $CONFIG
