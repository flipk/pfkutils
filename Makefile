# Local Variables:
# mode: makefile-gmake
# tab-width: 8
# End:

ifeq ($(CONFIG),)

KNOWN_CONFIGS= blade adler bohr droid sunlogin

all:
	@echo please specify CONFIG= from config/ subdir
	@echo or do 'make known_config' where known_config is
	@echo one of: $(KNOWN_CONFIGS)

$(KNOWN_CONFIGS):
	@+make CONFIG=$@

clean:
	rm -rf obj.*

else # $CONFIG

TOP= $(PWD)
OBJDIR= obj.$(PFKARCH).$(CONFIG)

CC=gcc
CXX=g++
CCOPTS=
CXXOPTS=
AR=ar
RANLIB=ranlib

CSRCS=
CXXSRCS=
HDRS=
INCS= -I$(OBJDIR) -I$(OBJDIR)/main
DEFS=

PREPROC_TARGETS=
LIB_TARGETS=
PROG_TARGETS= 

CONFIG_VALUES= \
	HAVE_NCURSES_CURSES_H HAVE_NCURSES_H HAVE_CURSES_H HAVE_DIRENT_H \
	HAVE_STRUCT_DIRENT_D_TYPE \
	HAVE_LSEEK64 HAVE_LSEEK HAVE_STRINGS_H HAVE_STRING_H \
	HAVE_STRUCT_STAT_ST_RDEV HAVE_STRUCT_STAT_ST_BLOCKS \
	HAVE_STRUCT_STAT_ST_BLKSIZE HAVE_STRUCT_STAT_ST_FLAGS \
	HAVE_STRUCT_STAT_ST_GEN HAVE_INET_ATON HAVE_INTTYPES_H \
	HAVE_PTHREAD_MUTEX_CONSISTENT_NP HAVE_PTHREAD_MUTEXATTR_SETPSHARED \
	HAVE_PTHREAD_MUTEXATTR_SETROBUST_NP \
	HAVE_PTHREAD_CONDATTR_SETPSHARED

ifeq ($(VERBOSE),1)
Q=
else
Q=@
endif

# this is named with a "2" to catch all those places
# i haven't converted yet. when i'm done converting,
# this should change back to pfkutils_config.h.
CONFIG_H= $(OBJDIR)/pfkutils_config2.h

all:
	@+make objdirs
	@+make preprocs
	@+make deps
	@+make PFKUTILS_INCLUDE_DEPS=1 _all

include config/$(CONFIG)
include config/always

preprocs: $(CONFIG_H) $(PREPROC_TARGETS)

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
	@echo CSRCS=$(CSRCS)
	@echo CXXSRCS=$(CXXSRCS)
	@echo HDRS=$(HDRS)
	@echo OBJS=$(OBJS)
	@echo '**************** CURRENT CONFIGURATION **************************'
	@echo ''

$(OBJDIR)/%.o: %.c
	@echo compiling $<
	$(Q)gcc -c $(INCS) $(DEFS) $(CXXFLAGS) $< -o $@

$(OBJDIR)/%.o: %.cc
	@echo compiling $<
	$(Q)g++ -c $(INCS) $(DEFS) $(CXXFLAGS) $< -o $@

%.o: %.cc
	@echo compiling $<
	$(Q)g++ -c $(INCS) $(DEFS) $(CXXFLAGS) $< -o $@

$(OBJDIR)/%.cc : %.yy
	@echo making $@
	$(Q)bison -d $< -o $@

# flex ignores the -o arg !
$(OBJDIR)/%.cc : %.ll
	@echo making $@
	$(Q)LLFILE=$$PWD/$< && \
		cd `dirname $@` && \
		flex $$LLFILE && \
		mv lex.yy.c `basename $@`

$(OBJDIR)/%.c.d: %.c
	@echo depending $<
	$(Q)$(CC) $(INCS) $(DEFS) -M $< -MT $(<:%.c=%.o) -MF $@

%.cc.d: %.cc
	@echo depending $<
	$(Q)$(CC) $(INCS) $(DEFS) -M $< -MT $(<:%.cc=%.o) -MF $@

$(OBJDIR)/%.cc.d: %.cc
	@echo depending $<
	$(Q)$(CC) $(INCS) $(DEFS) -M $< -MT $(<:%.cc=$(OBJDIR)/%.o) -MF $@

##############################################

# TODO: add .y and .l support some day

define TARGET_VARS
$(target)_COBJS    = $(patsubst %.c, $(OBJDIR)/%.o, $($(target)_CSRCS))
$(target)_CXXOBJS  = $(patsubst %.cc,$(OBJDIR)/%.o, $($(target)_CXXSRCS))
$(target)_YYGENSRCS= $(patsubst %.yy,$(OBJDIR)/%.cc,$($(target)_YYSRCS))
$(target)_YYGENHDRS= $(patsubst %.yy,$(OBJDIR)/%.hh,$($(target)_YYSRCS))
$(target)_YYGENOBJS= $(patsubst %.yy,$(OBJDIR)/%.o, $($(target)_YYSRCS))
$(target)_LLGENSRCS= $(patsubst %.ll,$(OBJDIR)/%.cc,$($(target)_LLSRCS))
$(target)_LLGENOBJS= $(patsubst %.ll,$(OBJDIR)/%.o, $($(target)_LLSRCS))

CSRCS   += $($(target)_CSRCS)
CXXSRCS += $($(target)_CXXSRCS)
HDRS    += $($(target)_HDRS)
YYSRCS  += $($(target)_YYSRCS)
LLSRCS  += $($(target)_LLSRCS)

endef

define LIB_TARGET_RULES

$($(target)_LLGENSRCS): $($(target)_YYGENSRCS)
$($(target)_LLGENSRCS): $($(target)_LLSRCS)
$($(target)_YYGENSRCS): $($(target)_YYSRCS)

CXXGENSRCS += $($(target)_YYGENSRCS) $($(target)_LLGENSRCS)

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

$($(target)_LLGENSRCS): $($(target)_YYGENSRCS)
$($(target)_LLGENSRCS): $($(target)_LLSRCS)
$($(target)_YYGENSRCS): $($(target)_YYSRCS)

CXXGENSRCS += $($(target)_YYGENSRCS) $($(target)_LLGENSRCS)

$($(target)_TARGET): $($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_DEPLIBS)
	@echo linking $($(target)_TARGET)
	$(Q)g++ -o $($(target)_TARGET) \
		$($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_LIBS) $($(target)_DEPLIBS)

endef

$(eval $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),$(TARGET_VARS)))
$(eval $(foreach target,$(PROG_TARGETS),$(PROG_TARGET_RULES)))
$(eval $(foreach target,$(LIB_TARGETS),$(LIB_TARGET_RULES)))

##############################################

CDEPS = $(CSRCS:%.c=$(OBJDIR)/%.c.d)
CXXDEPS = $(CXXSRCS:%.cc=$(OBJDIR)/%.cc.d)
CXXGENDEPS = $(CXXGENSRCS:%.cc=%.cc.d)

deps: $(CDEPS) $(CXXDEPS) $(CXXGENDEPS)

ifeq ($(PFKUTILS_INCLUDE_DEPS),1)
include $(CDEPS) $(CXXDEPS) # $(CXXGENDEPS)
endif

##############################################

_all: $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),$($(target)_TARGET))

install:

clean:
	rm -rf $(OBJDIR)

define PFK_CONFIG_LINE
echo \#define $(value) $(PFK_CONFIG_$(value)) ;
endef

OBJDIRS_TOMAKE= \
	libWebAppServer libpfkdll2 libpfkfb libpfkthread libpfkutil \
	ampfk backup bglog diskloader environ i2 main misc \
	pfkscript pfksh scripts syslog

objdirs:
	$(Q)mkdir -p $(OBJDIR) $(foreach d,$(OBJDIRS_TOMAKE),$(OBJDIR)/$(d))

$(CONFIG_H): Makefile config/always config/$(CONFIG)
	$(Q)echo \#define PACKAGE_NAME \"pfkutils\" > $(CONFIG_H).tmp
	$(Q)echo \#define PACKAGE_STRING \"pfkutils\" >> $(CONFIG_H).tmp
	$(Q)($(foreach value,$(CONFIG_VALUES), \
		$(PFK_CONFIG_LINE))) >> $(CONFIG_H).tmp
	$(Q)if [ -f $(CONFIG_H) ] ; then \
		if ! cmp -s $(CONFIG_H).tmp $(CONFIG_H) ; then \
			echo making $(CONFIG_H) ; \
			mv $(CONFIG_H).tmp $(CONFIG_H) ; \
		fi ; \
	else \
		echo making $(CONFIG_H) ; \
		mv $(CONFIG_H).tmp $(CONFIG_H) ; \
	fi

endif # $CONFIG
