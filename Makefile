
ifeq ($(CONFIG),)

KNOWN_CONFIGS= blade adler bohr droid sunlogin

all:
	@echo please specify CONFIG= from config/ subdir
	@echo or do 'make known_config' where known_config is
	@echo one of: $(KNOWN_CONFIGS)

$(KNOWN_CONFIGS):
	@+make CONFIG=$@

clean:
	rm -rf obj.$(PFKARCH).*

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
	HAVE_STRUCT_STAT_ST_GEN HAVE_INET_ATON \
	HAVE_PTHREAD_MUTEX_CONSISTENT_NP HAVE_PTHREAD_MUTEXATTR_SETPSHARED \
	HAVE_PTHREAD_MUTEXATTR_SETROBUST_NP \
	HAVE_PTHREAD_CONDATTR_SETPSHARED

ifeq ($(VERBOSE),1)
Q=
else
Q=@
endif

all: objdirs $(OBJDIR)/xmakefile
	$(Q)+make -f $(OBJDIR)/xmakefile _all

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
	@echo CSRCS=$(CSRCS)
	@echo CXXSRCS=$(CXXSRCS)
	@echo HDRS=$(HDRS)
	@echo OBJS=$(OBJS)
	@echo '**************** CURRENT CONFIGURATION **************************'
	@echo ''

OBJDIRS_TOMAKE= \
	libWebAppServer libpfkdll2 libpfkfb libpfkthread libpfkutil \
	ampfk backup bglog diskloader environ i2 main misc \
	pfkscript pfksh scripts syslog

objdirs:
	$(Q)mkdir -p $(OBJDIR) $(foreach d,$(OBJDIRS_TOMAKE),$(OBJDIR)/$(d))

$(OBJDIR)/%.o: %.c
	@echo compiling $<
	$(Q)gcc -c $(INCS) $(DEFS) $(CXXFLAGS) $< -o $@

$(OBJDIR)/%.o: %.cc
	@echo compiling $<
	$(Q)g++ -c $(INCS) $(DEFS) $(CXXFLAGS) $< -o $@

CONFIG_H= $(OBJDIR)/pfkutils_config2.h

##############################################

define LIB_TARGET_VARS
$(target)_COBJS= $(patsubst %.c,$(OBJDIR)/%.o,$($(target)_CSRCS))
$(target)_CXXOBJS= $(patsubst %.cc,$(OBJDIR)/%.o,$($(target)_CXXSRCS))
CSRCS += $($(target)_CSRCS)
CXXSRCS += $($(target)_CXXSRCS)
HDRS += $($(target)_HDRS)

endef

define LIB_TARGET_RULES
$($(target)_TARGET): $($(target)_COBJS) $($(target)_CXXOBJS)
	@echo linking $($(target)_TARGET)
	$(Q)rm -f $($(target)_TARGET)
	$(Q)$(AR) cq $($(target)_TARGET) $($(target)_COBJS) $($(target)_CXXOBJS)
	$(Q)$(RANLIB) $($(target)_TARGET)

endef

define PROG_TARGET_VARS
$(target)_COBJS= $(patsubst %.c,$(OBJDIR)/%.o,$($(target)_CSRCS))
$(target)_CXXOBJS= $(patsubst %.cc,$(OBJDIR)/%.o,$($(target)_CXXSRCS))
CSRCS += $($(target)_CSRCS)
CXXSRCS += $($(target)_CXXSRCS)
HDRS += $($(target)_HDRS)

endef

define PROG_TARGET_RULES
$($(target)_TARGET): $($(target)_COBJS) $($(target)_CXXOBJS) $($(target)_DEPLIBS)
	@echo linking $($(target)_TARGET)
	$(Q)g++ -o $($(target)_TARGET) $($(target)_COBJS) $($(target)_CXXOBJS) $($(target)_LIBS) $($(target)_DEPLIBS)

endef

$(eval $(foreach target,$(PROG_TARGETS),$(PROG_TARGET_VARS)))
$(eval $(foreach target,$(LIB_TARGETS),$(LIB_TARGET_VARS)))

$(eval $(foreach target,$(PROG_TARGETS),$(PROG_TARGET_RULES)))
$(eval $(foreach target,$(LIB_TARGETS),$(LIB_TARGET_RULES)))

##############################################

$(OBJDIR)/xmakefile: $(CONFIG_H) $(PREPROC_TARGETS) Makefile $(CSRCS) $(CXXSRCS) $(HDRS)
	@echo depending
	$(Q)cat Makefile > $(OBJDIR)/x
	$(Q)set -e ; for f in $(CSRCS) ; do \
		$(CC) $(INCS) $(DEFS) -M $$f \
			-MT $(OBJDIR)/$${f%.c}.o -MF $(OBJDIR)/xdeps ; \
		cat $(OBJDIR)/xdeps >> $(OBJDIR)/x ; \
		rm -f $(OBJDIR)/xdeps ; \
	done
	$(Q)set -e ; for f in $(CXXSRCS) ; do \
		$(CC) $(INCS) $(DEFS) -M $$f \
			-MT $(OBJDIR)/$${f%.cc}.o -MF $(OBJDIR)/xdeps ; \
		cat $(OBJDIR)/xdeps >> $(OBJDIR)/x ; \
		rm -f $(OBJDIR)/xdeps ; \
	done
	$(Q)mv $(OBJDIR)/x $(OBJDIR)/xmakefile

_all: $(foreach target,$(LIB_TARGETS) $(PROG_TARGETS),$($(target)_TARGET))

install:

clean:
	rm -rf $(OBJDIR)

define PFK_CONFIG_LINE
echo \#define $(value) $(PFK_CONFIG_$(value)) ;
endef

$(CONFIG_H): Makefile config/always config/$(CONFIG)
	@echo making $(CONFIG_H).tmp
	$(Q)echo \#define PACKAGE_NAME \"pfkutils\" > $(CONFIG_H).tmp
	$(Q)echo \#define PACKAGE_STRING \"pfkutils\" >> $(CONFIG_H).tmp
	$(Q)($(foreach value,$(CONFIG_VALUES),$(PFK_CONFIG_LINE))) >> $(CONFIG_H).tmp
	$(Q)if [ -f $(CONFIG_H) ] ; then \
		if ! cmp -s $(CONFIG_H).tmp $(CONFIG_H) ; then \
			mv $(CONFIG_H).tmp $(CONFIG_H) ; \
		fi ; \
	else \
		mv $(CONFIG_H).tmp $(CONFIG_H) ; \
	fi

endif # $CONFIG

# contrib/cscope
