
ifeq ($(CONFIG),)

KNOWN_CONFIGS= blade adler bohr droid sunlogin

all:
	@echo please specify CONFIG= from config/ subdir
	@echo or do 'make known_config' where known_config is
	@echo one of: $(KNOWN_CONFIGS)

$(KNOWN_CONFIGS):
	make CONFIG=$@

clean:
	rm -rf obj.$(PFKARCH).*

else # $CONFIG

CC=gcc
CXX=g++
CCOPTS=
CXXOPTS=
AR=ar
RANLIB=ranlib

CSRCS=
CXXSRCS=
HDRS=
INCS=
DEFS=

TOP= $(PWD)
OBJDIR= obj.$(PFKARCH).$(CONFIG)
CXXOBJS= $(CXXSRCS:%.cc=$(OBJDIR)/%.o)
COBJS= $(CSRCS:%.c=$(OBJDIR)/%.o)
OBJS= $(COBJS) $(CXXOBJS)
LIBS=

PREPROC_TARGETS=
LIB_TARGETS=
PROG_TARGETS= 

all: echoconfig $(OBJDIR)/xmakefile
	@make -f $(OBJDIR)/xmakefile _all

include config/$(CONFIG)
include config/always

echoconfig:
	@echo '******************************************************************************'
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
	@echo LIBS=$(LIBS)
	@echo '******************************************************************************'

$(OBJDIR)/xmakefile: Makefile $(xxx_OTHER_SHIT)
	@echo depending
	@mkdir -p $(OBJDIR)
	@cat Makefile > $(OBJDIR)/x
	@for f in $(CSRCS) ; do \
		$(CC) $(INCS) $(DEFS) -M $$f \
			-MT $(OBJDIR)/$${f%.c}.o -MF $(OBJDIR)/xdeps ; \
		cat $(OBJDIR)/xdeps >> $(OBJDIR)/x ; \
		rm -f $(OBJDIR)/xdeps ; \
	done
	@for f in $(CXXSRCS) ; do \
		$(CC) $(INCS) $(DEFS) -M $$f \
			-MT $(OBJDIR)/$${f%.cc}.o -MF $(OBJDIR)/xdeps ; \
		cat $(OBJDIR)/xdeps >> $(OBJDIR)/x ; \
		rm -f $(OBJDIR)/xdeps ; \
	done
	@mv $(OBJDIR)/x $(OBJDIR)/xmakefile

_all: $(PREPROC_TARGETS) $(LIB_TARGETS) $(PROG_TARGETS)

install:

clean:
	rm -rf $(OBJDIR)

#pfkutils: $(OBJS)
#	g++ -o pfkutils $(OBJS) $(LIBS)

#$(_LIB): $(OBJS)
#	rm -f $(_LIB)
#	$(AR) cq $(_LIB) $(OBJS)
#	$(RANLIB) $(_LIB)

#$(CXXOBJS): $(OBJDIR)/%.o: %.cc
#	$(CXX) $(DEFS) $(INCS) $(CXXOPTS) -c $< -o $@

#$(COBJS): $(OBJDIR)/%.o: %.c
#	$(CC) $(DEFS) $(INCS) $(CCOPTS) -c $< -o $@

endif # $CONFIG

# contrib/cscope
