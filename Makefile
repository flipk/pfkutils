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

$(KNOWN_CONFIGS):
	@+make CONFIG=$@

clean:
	rm -rf obj.*

##############################################

else # $CONFIG

##############################################

TOP= $(PWD)
OBJDIR= obj.$(PFKARCH).$(CONFIG)

CC=gcc
CXX=g++
CCOPTS=
CXXOPTS=
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
$(target)_COBJS    = $(patsubst %.c, $(OBJDIR)/%.o, $($(target)_CSRCS))
$(target)_CDEPS    = $(patsubst %.c, $(OBJDIR)/%.c.d, $($(target)_CSRCS))
$(target)_CXXOBJS  = $(patsubst %.cc,$(OBJDIR)/%.o, $($(target)_CXXSRCS))
$(target)_CXXDEPS  = $(patsubst %.cc,$(OBJDIR)/%.cc.d, $($(target)_CXXSRCS))
$(target)_YYGENSRCS= $(patsubst %.yy,$(OBJDIR)/%.cc,$($(target)_YYSRCS))
$(target)_YYGENDEPS= $(patsubst %.yy,$(OBJDIR)/%.cc.d, $($(target)_YYSRCS))
$(target)_YYGENHDRS= $(patsubst %.yy,$(OBJDIR)/%.hh,$($(target)_YYSRCS))
$(target)_YYGENOBJS= $(patsubst %.yy,$(OBJDIR)/%.o, $($(target)_YYSRCS))
$(target)_LLGENSRCS= $(patsubst %.ll,$(OBJDIR)/%.cc,$($(target)_LLSRCS))
$(target)_LLGENDEPS= $(patsubst %.ll,$(OBJDIR)/%.cc.d, $($(target)_LLSRCS))
$(target)_LLGENOBJS= $(patsubst %.ll,$(OBJDIR)/%.o, $($(target)_LLSRCS))

endef

$(eval $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),$(TARGET_VARS)))

##############################################

define TARGET_RULES
$($(target)_LLGENSRCS): $($(target)_YYGENSRCS)
$($(target)_LLGENSRCS): $($(target)_LLSRCS)
$($(target)_YYGENSRCS): $($(target)_YYSRCS)

CXXGENSRCS += $($(target)_YYGENSRCS) $($(target)_LLGENSRCS)
CDEPS += $($(target)_CDEPS)
CXXDEPS += $($(target)_CXXDEPS)
CXXGENDEPS += $($(target)_YYGENDEPS) $($(target)_LLGENDEPS)

$($(target)_COBJS) : $(OBJDIR)/%.o: %.c
	@echo compiling $$<
	$(Q)gcc -c -I$(OBJDIR) $($(target)_INCS) $(CXXFLAGS) $$< -o $$@

$($(target)_CXXOBJS) : $(OBJDIR)/%.o: %.cc
	@echo compiling $$<
	$(Q)g++ -c -I$(OBJDIR) $($(target)_INCS) $(CXXFLAGS) $$< -o $$@

$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) : %.o: %.cc
	@echo compiling $$<
	$(Q)g++ -c -I$(OBJDIR) $($(target)_INCS) $(CXXFLAGS) $$< -o $$@

$($(target)_YYGENSRCS) : $(OBJDIR)/%.cc : %.yy
	@echo making $$@
	$(Q)bison -d $$< -o $$@

# flex ignores the -o arg !
$($(target)_LLGENSRCS) : $(OBJDIR)/%.cc : %.ll
	@echo making $$@
	$(Q)LLFILE=$$(PWD)/$$< && \
		cd `dirname $$@` && \
		flex $$$$LLFILE && \
		mv lex.yy.c `basename $$@`

$($(target)_CDEPS) : $(OBJDIR)/%.c.d: %.c
	@echo depending $$<
	$(Q)$(CC) -I$(OBJDIR) $($(target)_INCS) \
		-M $$< -MT $$(<:%.c=$(OBJDIR)/%.o) -MF $$@

$($(target)_CXXDEPS) : $(OBJDIR)/%.cc.d: %.cc
	@echo depending $$<
	$(Q)$(CC) -I$(OBJDIR) $($(target)_INCS) \
		-M $$< -MT $$(<:%.cc=$(OBJDIR)/%.o) -MF $$@

$($(target)_YYGENDEPS) $($(target)_LLGENDEPS) : %.cc.d: %.cc
	@echo depending $$<
	$(Q)$(CC) -I$(OBJDIR) $($(target)_INCS) \
		-M $$< -MT $$(<:%.cc=%.o) -MF $$@

endef

define LIB_TARGET_RULES

$($(target)_TARGET): $($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS)
	@echo linking $($(target)_TARGET)
	$(Q)rm -f $($(target)_TARGET)
	$(Q)$(AR) cq $($(target)_TARGET) \
		$($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS)
	$(Q)$(RANLIB) $($(target)_TARGET)

endef

define PROG_TARGET_RULES

$($(target)_TARGET): $($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_DEPLIBS)
	@echo linking $($(target)_TARGET)
	$(Q)g++ -o $($(target)_TARGET) \
		$($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_LIBS) $($(target)_DEPLIBS)

endef

$(eval $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),$(TARGET_RULES)))
$(eval $(foreach target,$(LIB_TARGETS),$(LIB_TARGET_RULES)))
$(eval $(foreach target,$(PROG_TARGETS),$(PROG_TARGET_RULES)))

##############################################

preprocs: $(CONFIG_H) $(PREPROC_TARGETS)

##############################################

deps: $(CDEPS) $(CXXDEPS) $(CXXGENDEPS)

ifeq ($(PFKUTILS_INCLUDE_DEPS),1)
include $(CDEPS) $(CXXDEPS) $(CXXGENDEPS)
endif

##############################################

_all: $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),$($(target)_TARGET))

install:

clean:
	rm -rf $(OBJDIR)

##############################################

endif # $CONFIG
