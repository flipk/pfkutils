
PROG= webproxy_all.o

HDRS= conn.H inputbuf.H proxy.H status.H connlog.H config.h
SRCS= conn.C inputbuf.C proxy.C status.C connlog.C main.C 
OBJS= conn.o inputbuf.o proxy.o status.o connlog.o main.o
LIBS= ../threads/libthreads.a
CCOPTS= -g3
#CCOPTS= -O3

all: xmakefile
	@make -f xmakefile _all

xmakefile: $(SRCS) $(HDRS) Makefile
	@rm -f xmakefile
	@cat Makefile > xmakefile.tmp
	@gcc -M $(SRCS) >> xmakefile.tmp
	@mv xmakefile.tmp xmakefile

_all: $(PROG) 

$(PROG): $(OBJS)
	ld -x -r $(OBJS) -o $(PROG)
#	crunchide -k webproxy_main $(PROG)

.C.o:
	gcc $(CCOPTS) -c $*.C

.c.o:
	gcc $(CCOPTS) -c $*.c

clean:
	rm -f xmakefile* $(PROG) *.o *~ *core

#
# dependencies appear below this line in the xmakefile
#
