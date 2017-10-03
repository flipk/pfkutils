
TARG= i2

I2SRCS= ipipe_acceptor.C ipipe_connector.C ipipe_factories.C \
	ipipe_forwarder.C ipipe_main.C ipipe_rollover.C ipipe_stats.C
I2HDRS= ipipe_acceptor.H ipipe_connector.H ipipe_factories.H \
	ipipe_forwarder.H ipipe_main.H ipipe_rollover.H ipipe_stats.H
MISCSRCS= pk-md5.c m.c fd_mgr.C
MISCHDRS= pk-md5.h m.h fd_mgr.H
COMHDRS= types.H circular_buffer.H
DLL2SRCS= dll2_hash.C
DLL2HDRS= dll2.H
ROOTHDR= version.h

SRCS= $(I2SRCS) $(MISCSRCS)            $(DLL2SRCS)
HDRS= $(I2HDRS) $(MISCSRCS) $(COMHDRS) $(DLL2HDRS) $(ROOTHDR)

_OBJS= $(SRCS:.C=.o)
OBJS= $(_OBJS:.c=.o)
INCS= -I.
DEFS= -DI2_MD5 -Di2_main=main
OPTS= -O3
LIBS=
CC= g++

all: xmakefile
	@make -f xmakefile $(TARG)

copy:
	@./copy-if-changed ../../util      $(MISCSRCS) $(MISCHDRS)
	@./copy-if-changed ../../dll2      $(DLL2SRCS) $(DLL2HDRS)
	@./copy-if-changed ../../h         $(COMHDRS)
	@./copy-if-changed ../..           $(ROOTHDR)
	@./copy-if-changed ..              $(I2SRCS) $(I2HDRS)

xmakefile: Makefile $(CPPSRCS) $(CSRCS) $(HDRS)
	rm -f xmakefile tmp
	cat Makefile > tmp
	echo '' >> tmp
	$(CC) $(INCS) $(DEFS) -M $(SRCS) >> tmp
	mv tmp xmakefile

$(TARG): $(OBJS)
	rm -f $(TARG)
	$(CC) $(OBJS) $(LIBS) -o $(TARG)

.C.o:
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.C

.c.o:
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.c

depend: xmakefile

clean:
	rm -f *.C *.H *.c *.h $(OBJS) $(TARG) xmakefile tmp

