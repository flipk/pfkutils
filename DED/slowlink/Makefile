
PROG= slowlink
SRCS= slowlink.cc
LIBSRCS= ../../libpfkutil/LockWait.cc ../../libpfkutil/dll3.cc ../../libpfkutil/thread_slinger.cc
#
LIBS= -lpthread -lrt
INCS= -I../../libpfkutil
CCFLAGS= -g3 -Wall -Werror
O= objs_$(PFKARCH)
LIBOBJS= $(LIBSRCS:../../libpfkutil/%.cc=$(O)/%.o)
PROGOBJS= $(SRCS:%.cc=$(O)/%.o)
OBJS= $(LIBOBJS) $(PROGOBJS)

ifeq ($(VERBOSE),)
Q=@
E=echo
else
Q=
E=true
endif

all: $(O)/xmakefile
	+$(Q)make -f $(O)/xmakefile $(O)/$(PROG)

clean:
	@$(E) cleaning
	$(Q)rm -rf $(O)

$(O)/xmakefile: Makefile $(LIBSRCS) $(SRCS)
	@$(E) depending
	$(Q)mkdir -p $(O)
	$(Q)rm -f $(O)/xmakefile $(O)/x
	$(Q)cat Makefile > $(O)/x
	$(Q)($(foreach f,$(LIBSRCS),g++ -M $(f) $(INCS) \
		-MT $(f:../../libpfkutil/%.cc=$(O)/%.o) &&) echo \#) >> $(O)/x
	$(Q)($(foreach f,$(SRCS),g++ -M $(f) $(INCS) \
		-MT $(f:%.cc=$(O)/%.o) &&) echo \#) >> $(O)/x
	$(Q)mv $(O)/x $(O)/xmakefile

$(O)/$(PROG): $(OBJS)
	@$(E) linking $(PROG)
	$(Q)g++ -o $@ $(OBJS) $(LIBS)

$(LIBOBJS): $(O)/%.o: ../../libpfkutil/%.cc
	@$(E) compiling $<
	$(Q)g++ -c $(CCFLAGS) $(INCS) $< -o $@

$(PROGOBJS): $(O)/%.o: %.cc
	@$(E) compiling $<
	$(Q)g++ -c $(CCFLAGS) $(INCS) $< -o $@


# dependencies below this line
