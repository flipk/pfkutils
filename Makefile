# Local Variables:
# mode: makefile-gmake
# tab-width: 8
# End:

ifeq ($(VERBOSE),1)
Q=
else
Q=@
endif

export VERBOSE

ifeq ($(CONFIG),)

##############################################

KNOWN_CONFIGS= blade adler droid

all:
	@echo please specify CONFIG= from config/ subdir
	@echo or do 'make known_config' where known_config is
	@echo one of: $(KNOWN_CONFIGS)

define PER_CONFIG_RULES
$(config):
	$(Q)+make -s CONFIG=$(config)

$(config)-cscope:
	$(Q)make -s CONFIG=$(config) cscope

$(config)-install:
	$(Q)make -s CONFIG=$(config) install

$(config)-clean:
	$(Q)make -s CONFIG=$(config) clean

endef

$(eval $(foreach config,$(KNOWN_CONFIGS),$(PER_CONFIG_RULES)))

clean:
	rm -rf obj.*

##############################################

else # $CONFIG

##############################################

TOP= $(PWD)
PFKARCH := $(shell ./scripts/architecture)
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

##############################################

all:
	$(Q)+make objdirs
	$(Q)+make preprocs
	$(Q)+make deps
	$(Q)+make __INCLUDE_DEPS=1 _all

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
CXXGENSRCS += $($(target)_YYGENSRCS) $($(target)_LLGENSRCS) $($(target)_PROTOGENSRCS)

CDEPS += $($(target)_CDEPS)
CXXDEPS += $($(target)_CXXDEPS)

CGENDEPS += $($(target)_YGENDEPS) $($(target)_LGENDEPS)
CXXGENDEPS += $($(target)_YYGENDEPS) $($(target)_LLGENDEPS) $($(target)_PROTOGENDEPS)

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

$($(target)_PROTOGENOBJS) : %.o: %.cc
	@echo compiling $$<
	$(Q)gcc -c -I$(OBJDIR) -I$(PROTOBUF_INC) $($(target)_INCS) \
		$($(target)_DEFS) $(CXXFLAGS) $$< -o $$@

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

$($(target)_PROTOGENSRCS): $(OBJDIR)/%.pb.cc : %.proto
	@echo making $$@
	@cd $$(dir $$<) && $(PROTOC_PATH) \
		--cpp_out=$$(dir $(PWD)/$$@) $$(notdir $$<)

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

$($(target)_PROTOGENDEPS) : %.cc.d: %.cc
	@echo depending $$<
	$(Q)$(CC) -I$(OBJDIR) $($(target)_INCS) $(PROTOBUF_INC) \
		$($(target)_DEFS) -M $$< -MT $$(<:%.cc=%.o) -MF $$@

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

$(target)_install: $($(target)_TARGET)
	$(Q)set -e ; if [ "x$($(target)_INSTALL_HDRS)" != "x" ] ; then \
		echo installing $($(target)_TARGET) and headers ; \
		cp $($(target)_TARGET) $(PFK_LIB_DIR) ; \
		tar cf - $($(target)_INSTALL_HDRS) | \
			tar -C $(PFK_INC_DIR) -xf - ; \
	fi

endef

define PROG_TARGET_RULES

$($(target)_TARGET): $($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YGENOBJS) $($(target)_LGENOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_PROTOGENOBJS) \
		$($(target)_DEPLIBS) $($(target)_EXTRAOBJS)
	@echo linking $($(target)_TARGET)
	$(Q)g++ -o $($(target)_TARGET) \
		$($(target)_COBJS) $($(target)_CXXOBJS) \
		$($(target)_YGENOBJS) $($(target)_LGENOBJS) \
		$($(target)_YYGENOBJS) $($(target)_LLGENOBJS) \
		$($(target)_PROTOGENOBJS) $($(target)_DEPLIBS) \
		$($(target)_EXTRAOBJS) $($(target)_LIBS)

$(target)_install: $($(target)_TARGET)
	$(Q)set -e ; if [ "x$($(target)_INSTALL)" == "x1" ] ; then \
		echo installing $($(target)_TARGET) ; \
		FNAME="$(notdir $($(target)_TARGET))" ; \
		echo FNAME is $$$$FNAME ; \
		if [ -f $(PFK_BIN_DIR)/$$$$FNAME ] ; then \
			mv -f $(PFK_BIN_DIR)/$$$$FNAME \
				$(PFK_BIN_DIR)/$$$$FNAME.old ;\
		fi ; \
		cp $($(target)_TARGET) $(PFK_BIN_DIR)/$$$$FNAME ; \
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

##############################################

endif # $CONFIG
