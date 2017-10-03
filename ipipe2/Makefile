
TARG= i2
SRCS= ipipe_acceptor.C ipipe_connector.C ipipe_factories.C ipipe_forwarder.C ipipe_main.C ipipe_stats.C ipipe_tick.C
HDRS= ipipe_acceptor.H ipipe_connector.H ipipe_factories.H ipipe_forwarder.H ipipe_stats.H ipipe_tick.H

OBJS= $(SRCS:.C=.o) fd_mgr.obj
CC=  g++
INCS= -I../dll2 -I..
DEFS= -DDLL2_INCLUDE_BTREE=0
LIBS= -lz -lpthread
OPTS= -g3 -Wall -Werror

all: xmakefile
	@make -f xmakefile $(TARG)

xmakefile: Makefile $(SRCS) $(HDRS)
	rm -f xmakefile tmp
	cat Makefile > tmp
	echo '' >> tmp
	$(CC) $(INCS) $(DEFS) -M $(SRCS) >> tmp
	mv tmp xmakefile

fd_mgr.obj: ../fd_mgr.C ../fd_mgr.H
	$(CC) $(INCS) $(DEFS) $(OPTS) -c ../fd_mgr.C -o fd_mgr.obj

$(TARG): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(TARG)

.C.o:
#	$(CC) $(INCS) $(DEFS) $(OPTS) -E $*.C > $*.i
	$(CC) $(INCS) $(DEFS) $(OPTS) -c $*.C

clean:
	rm -f xmakefile tmp *~ *.o *.obj *.i $(TARG)
