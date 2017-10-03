
TARG= libi2.a
SRCS= ipipe_acceptor.C ipipe_connector.C ipipe_factories.C ipipe_forwarder.C ipipe_main.C ipipe_stats.C
HDRS= ipipe_acceptor.H ipipe_connector.H ipipe_factories.H ipipe_forwarder.H ipipe_stats.H

OBJS= $(SRCS:.C=.o)
CC=  g++
INCS= -I../dll2 -I.. -I../util
DEFS= -DDLL2_INCLUDE_BTREE=0 -DI2_ZLIB -DI2_MD5
LIBS= -lz -lpthread
OPTS= -O3 -Wall -Werror

all: xmakefile
	@make -f xmakefile $(TARG)

xmakefile: Makefile $(SRCS) $(HDRS)
	rm -f xmakefile tmp
	cat Makefile > tmp
	echo '' >> tmp
	$(CC) $(INCS) $(DEFS) -M $(SRCS) >> tmp
	mv tmp xmakefile

$(TARG): $(OBJS)
	rm -f $(TARG)
	ar cq $(TARG) $(OBJS)

.C.o:
#	$(CC) $(INCS) $(DEFS) $(OPTS) -E $*.C > $*.i
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.C

depend: xmakefile

clean:
	rm -f xmakefile tmp *~ *.o *.a *.obj *.i
